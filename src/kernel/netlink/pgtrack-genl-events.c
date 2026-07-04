#include <linux/compiler.h>

#include <netlink/pgtrack/events.h>
#include <user/scid-netlink-defs.h>
#include <netlink.h>
#include <logging.h>
#include <pgsnap.h>
#include <pgtrack.h>

#include "pgtrack-genl-events.h"

struct kmem_cache *event_cachep;
struct kmem_cache *do_event_bcast_work_cachep;

/* can be called from atomic context */
static inline struct event *alloc_and_init_event(enum event_type type, const void *data)
{
	struct event *evt;

	evt = kmem_cache_alloc(event_cachep, GFP_ATOMIC);
	if(!evt)
		goto __failure0;

	evt->type = type;
	evt->data = data;

	return evt;

__failure0:
	scid_err("memory exhausted");
	return NULL;
}

static void free_event(struct event *evt)
{
	if(unlikely(!evt)) {
		scid_warn("evt is NULL, check your code");
		return;
	}

	if(likely(evt->data)) {
		if(evt->type == EVENT_TYPE_WXWARNING)
			del_page_wxwarn((struct page_wxwarn*) evt->data);
		else if(evt->type == EVENT_TYPE_SNAPSHOT)
			del_page_snap((struct page_snap*) evt->data);
		else
			scid_err("unrecognized event type to free!!");

		evt->data = NULL;
	} else
		scid_err("data is NULL??");

	kmem_cache_free(event_cachep, evt);
}

DECLARE_RWSEM(le_lock);
struct workqueue_struct *bcast_evt_wq;
struct kfifo le;

void free_le_kfifo(void)
{
	/* no locking takes place */

	struct event *evt;

	while(kfifo_out(&le, &evt, __fifo_bytes(1)))
		free_event(evt);

	kfifo_free(&le);
}

static void insert_event_le_kfifo(struct event *event)
{
	down_write(&le_lock);

	if(__fifo_len() == NR_QUEUED_EVENTS) {
		struct event *evt;

		if(unlikely(!kfifo_out(&le, &evt, __fifo_bytes(1)))) {
			scid_err("well well well... bug?");
			goto __unlock;
		} else
			free_event(evt);
	}

	kfifo_in(&le, &event, __fifo_bytes(1));

__unlock:
	up_write(&le_lock);
}

static bool __event_to_populate_skb_with_wxwarning(
		const struct page_wxwarn *wxw, struct sk_buff *skb, 
		__always_unused const void* args)
{
	if(unlikely(nla_put_s32(skb, SCID_GENL_ATTR_PID, wxw->pid))) {
		scid_err("unable to put pid in skb");
		return false;
	}

	if(unlikely(nla_put_u64_64bit(skb, SCID_GENL_ATTR_PFN, wxw->pfn, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put pfn in skb");
		return false;
	}

	if(unlikely(nla_put_u64_64bit(skb, SCID_GENL_ATTR_VA, wxw->va, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put va in skb");
		return false;
	}

	return true;
}

static bool __event_to_populate_skb_with_snapshot(
		const struct page_snap *snap, struct sk_buff *skb, const void* args)
{
	if(unlikely(nla_put_s32(skb, SCID_GENL_ATTR_PID, snap->pid))) {
		scid_err("unable to put pid in skb");
		return false;
	}

	if(unlikely(nla_put_u64_64bit(skb, SCID_GENL_ATTR_PFN, snap->pfn, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put pfn in skb");
		return false;
	}

	if(unlikely(nla_put_u64_64bit(skb, SCID_GENL_ATTR_VA, snap->va, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put va in skb");
		return false;
	}

	if(unlikely(nla_put_u64_64bit(skb, SCID_GENL_ATTR_PAGE_SNAPSHOT_DATETIME, 
					snap->datetime, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put datetime in skb");
		return false;
	}

	if(unlikely(nla_put_u64_64bit(skb, SCID_GENL_ATTR_PAGE_SNAPSHOT_SEQ, 
					snap->seq, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put seq in skb");
		return false;
	}

	if(unlikely(nla_put_u32(skb, SCID_GENL_ATTR_PAGE_SNAPSHOT_FAULT, 
					snap->fault))) {
		scid_err("unable to put fault in skb");
		return false;
	}

	if(args != EVENT_TO_SKB_GET_EVTS) {
		if(unlikely(nla_put(skb, SCID_GENL_ATTR_PAGE_SNAPSHOT, 
						PAGE_SIZE, snap->buffer))) {
			scid_errf(
					"unable to put snapshot in skb "
					"(skb_tailroom=%d < nla_total_size=%d)",
					skb_tailroom(skb), nla_total_size(PAGE_SIZE));
			return false;
		}
	}

	return true;
}

bool event_to_populate_skb_with(
		const struct event *event, struct sk_buff *skb, const void *args)
{
	if(event->type == EVENT_TYPE_WXWARNING)
		return __event_to_populate_skb_with_wxwarning(event->data, skb, args);
	else if(event->type == EVENT_TYPE_SNAPSHOT)
		return __event_to_populate_skb_with_snapshot(event->data, skb, args);
	else
		scid_warn("unknown event type!!");

	return false;
}

static inline int __event_nla_total_size(const struct event *event)
{
	int size = GENLMSG_DEFAULT_SIZE;

	/* 
	 * we may find better ways, but let's try to
	 * avoid specifying nla_total_size(sizeof(field))
	 */
	if(event->type == EVENT_TYPE_SNAPSHOT)
		size += nla_total_size(PAGE_SIZE);

	return size;
}

static struct sk_buff* event_to_skb_alloc_one(const struct event *event)
{
	int payld_size = __event_nla_total_size(event);

	struct sk_buff *skb = genlmsg_new(payld_size, GFP_KERNEL);
	if(!skb) {
		scid_err("unable to allocate skb");
		return NULL;
	}

	u8 cmd = SCID_GENL_CMD_EVENT_WXWARNING;
	if(event->type == EVENT_TYPE_SNAPSHOT)
		cmd = SCID_GENL_CMD_EVENT_SNAPSHOT;

	void *hdr = genlmsg_put(skb, 0, 0, &genl_fam, 0, cmd);
	if(!hdr) {
		scid_err("unable to put header");
		goto __failure_free;
	}

	if(!event_to_populate_skb_with(event, skb, EVENT_TO_SKB_BCAST)) {
		scid_err("unable to populate skb");
		goto __failure_cancel_and_free;
	}

	genlmsg_end(skb, hdr);
	return skb;

__failure_cancel_and_free:
	genlmsg_cancel(skb, hdr);
__failure_free:
	nlmsg_free(skb);
	return NULL;
}

static void __do_bcast(const struct event *event)
{
	int rv;
	struct sk_buff *skb;

	skb = event_to_skb_alloc_one(event);
	if(!skb) {
		scid_err("skb is NULL");
		return;
	}

	rv = genlmsg_multicast(&genl_fam, skb, 0, 0, GFP_KERNEL);
	if(rv && rv != -ESRCH)
		scid_err("unable to do multicast");
}

static void do_event_bcast(struct work_struct *work)
{
	struct do_event_bcast_work *bcast_work = 
		container_of(work, struct do_event_bcast_work, work);

	/* 
	 * first do bcast of the event, then record it in the
	 * kfifo to avoid a use-after-free bug 
	 */
	__do_bcast(bcast_work->event);
	insert_event_le_kfifo(bcast_work->event);

	kfree(bcast_work);
}

static bool __bcast_pgtrack_event_common(
		enum event_type type, const void *data)
{
	struct do_event_bcast_work *work;
	bool queued;

	work = kmem_cache_alloc(do_event_bcast_work_cachep, GFP_ATOMIC);
	if(!work)
		goto __failure0;

	work->event = alloc_and_init_event(type, data);
	if(!work->event)
		goto __failure1;

	INIT_WORK(&work->work, do_event_bcast);

	queued = queue_work(bcast_evt_wq, &work->work);
	if(!queued) {
		free_event(work->event);
		goto __failure1;
	}

	return queued;

__failure1:
	kmem_cache_free(do_event_bcast_work_cachep, work);
__failure0:
	scid_err("memory exhausted");
	return false;
}

/* events impl */

bool bcast_pgtrack_event_wxwarning(const struct page_wxwarn *wxw)
{
	bool done = __bcast_pgtrack_event_common(EVENT_TYPE_WXWARNING, wxw);
	if(!done)
		scid_err("unable to send out wxwarning event");
	
	return done;
}

bool bcast_pgtrack_event_snapshot(const struct page_snap *snap)
{
	bool done = __bcast_pgtrack_event_common(EVENT_TYPE_SNAPSHOT, snap);
	if(!done)
		scid_err("unable to send out snapshot event");
	
	return done;
}


