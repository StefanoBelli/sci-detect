#include <linux/slab.h>
#include <linux/compiler.h>

#include <netlink/pgtrack/events.h>
#include <user/scid-netlink-defs.h>
#include <netlink.h>
#include <logging.h>

#include "pgtrack-genl-events.h"

/* can be called from atomic context */
static struct event *alloc_and_init_event(enum event_type type, size_t data_size)
{
	const gfp_t gfp = GFP_ATOMIC;

	struct event *evt;

	evt = kmalloc(sizeof(struct event), gfp);
	if(!evt)
		goto __failure0;

	evt->type = type;

	evt->data = kmalloc(data_size, gfp);
	if(!evt->data) {
		kfree(evt);
		goto __failure0;
	}
	
	return evt;

__failure0:
	scid_err("memory exhausted");
	return NULL;
}

static void free_event(struct event *evt)
{
	if(!evt) {
		scid_warn("evt is NULL, check your code");
		return;
	}

	if(evt->data) {
		kfree(evt->data);
		evt->data = NULL;
	} else
		scid_err("data is NULL??");

	kfree(evt);
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
		const struct wxwarning_event *wxw, struct sk_buff *skb, 
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

bool event_to_populate_skb_with(
		const struct event *event, struct sk_buff *skb, const void *args)
{
	if(event->type == EVENT_TYPE_WXWARNING)
		return __event_to_populate_skb_with_wxwarning(event->data, skb, args);

	return false;
}

static inline int __event_nla_total_size(const struct event *event)
{
	int size;
	
	if(event->type == EVENT_TYPE_WXWARNING)
		size = 
			nla_total_size(sizeof(pid_t)) +
			nla_total_size(sizeof(unsigned long)) + 
			nla_total_size(sizeof(unsigned long));

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

	u8 cmd;
	if(event->type == EVENT_TYPE_WXWARNING)
		cmd = SCID_GENL_CMD_EVENT_WXWARNING;

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

struct do_event_bcast_work {
	struct event *event;
	struct work_struct work;
};

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

typedef void(*init_event_data_cb)(void *event_data, const void* args);

static bool __bcast_pgtrack_event_common(
		enum event_type type, size_t size, init_event_data_cb init, const void *args)
{
	struct do_event_bcast_work *work;
	bool queued;

	work = kmalloc(sizeof(struct do_event_bcast_work), GFP_ATOMIC);
	if(!work)
		goto __failure0;

	work->event = alloc_and_init_event(type, size);
	if(!work->event)
		goto __failure1;

	init(work->event->data, args);

	INIT_WORK(&work->work, do_event_bcast);

	queued = queue_work(bcast_evt_wq, &work->work);
	if(!queued) {
		free_event(work->event);
		goto __failure1;
	}

	return queued;

__failure1:
	kfree(work);
__failure0:
	scid_err("memory exhausted");
	return false;
}

/* events impl */

struct init_event_wxwarning_args {
	unsigned long pfn;
	pid_t pid;
	unsigned long va;
};

static void init_wxwarning_event(void *event_data, const void *args)
{
	struct wxwarning_event *event = event_data;
	const struct init_event_wxwarning_args *myargs = args;

	event->pfn = myargs->pfn;
	event->pid = myargs->pid;
	event->va = myargs->va;
}

bool bcast_pgtrack_event_wxwarning(unsigned long pfn, pid_t pid, unsigned long va)
{
	bool done;
	struct init_event_wxwarning_args args;

	args.pfn = pfn;
	args.pid = pid;
	args.va = va;

	done = __bcast_pgtrack_event_common(
			EVENT_TYPE_WXWARNING,
			sizeof(struct wxwarning_event),
			init_wxwarning_event,
			&args);

	if(!done)
		scid_err("unable to send out wxwarning event");
	
	return done;
}

bool bcast_pgtrack_event_snapshot(const struct snapshot_event *snap)
{
	return true;
}


