#include <linux/workqueue.h>
#include <linux/rwsem.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/compiler.h>

#include <netlink/pgtrack/cmds.h>
#include <netlink/pgtrack/events.h>
#include <netlink/pgtrack/setup.h>
#include <user/scid-netlink-defs.h>
#include <netlink.h>
#include <logging.h>

/* nr of queued events in the kfifo */
#define NR_QUEUED_EVENTS 10

/* single event ptr element size in kfifo */
#define EVENT_PTR_SIZE sizeof(struct event*)

/* convert from number of elements to bytes */
#define __fifo_bytes(i) (i * EVENT_PTR_SIZE)

/* kfifo_len(&le) is a multiple of EVENT_PTR_SIZE */
#define __fifo_len() (kfifo_len(&le) / EVENT_PTR_SIZE)

/* internal event representation */
enum event_type {
	EVENT_TYPE_WXWARNING,

	NR_EVENT_TYPES
};

struct event {
	enum event_type type;
	void *data;
};

struct wxwarning_event {
	pid_t pid;
	unsigned long va;
	unsigned long pfn;
};

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

/* static declarations */

static DECLARE_RWSEM(le_lock);
static struct workqueue_struct *bcast_evt_wq;
static struct kfifo le;

static void free_le_kfifo(void)
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

#define EVENT_TO_SKB_BCAST ((void*) 0)
#define EVENT_TO_SKB_GET_EVTS ((void*) 1)
#define EVENT_TO_SKB_PEEKONE ((void*) 2)

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

static bool event_to_populate_skb_with(
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

/* exported */

int pgtrack_genl_get_last_events_doit(
		__always_unused struct sk_buff *in_skb, struct genl_info *info)
{
	struct sk_buff *skb;
	void *hdr;
	struct nlattr *nest;

	/* new genl skb */
	skb = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if(!skb) {
		scid_err("unable to get new skb");
		return -ENOMEM;
	}

	/* put header */
	hdr = genlmsg_put(skb, info->snd_portid, info->snd_seq, &genl_fam, 0, info->genlhdr->cmd);
    if (!hdr) {
        scid_err("unable to build hdr");
        goto __failure_nlfree;
    }

    /* take the fifo lock */
    down_read(&le_lock);

    /* always send out an array */
    unsigned int fifo_len = __fifo_len();

    /* put the nr elems of array prior */
    if(unlikely(nla_put_u32(skb, SCID_GENL_ATTR_ARRAY_NR_ELEMS, fifo_len))) {
    	scid_err("unable to put nr_elems");
    	goto __failure_unlock_cancel_nlfree;
    }

    /* put the nesting for the array, app will know how to 
     * rebuild everything */
    nest = nla_nest_start(skb, SCID_GENL_ATTR_ARRAY);
    if(!nest) {
    	scid_err("unable to create nesting");
    	goto __failure_unlock_cancel_nlfree;
    }

    /* if the fifo is empty... */
    if(!fifo_len)
    	goto __end_ok;

    struct event **evts = kmalloc(__fifo_bytes(fifo_len), GFP_KERNEL);
    if(!evts) {
    	scid_err("memory exhausted");
    	goto __failure_endnest_unlock_cancel_nlfree;
    }

    if(unlikely(!kfifo_out_peek(&le, evts, __fifo_bytes(fifo_len))))
    	scid_warn("why kfifo_out_peek is 0???");
    else {
    	for(unsigned int i = 0; i < fifo_len; i++) {
    		if(unlikely(nla_put_u32(skb, SCID_GENL_ATTR_EVT_TYPE, evts[i]->type))) {
    			scid_err("unable to put evt type in skb");
    			kfree(evts);
    			goto __failure_endnest_unlock_cancel_nlfree;
    		}

    		if(unlikely(!event_to_populate_skb_with(evts[i], skb, EVENT_TO_SKB_GET_EVTS))) {
    			scid_err("unable to populate skb");
    			kfree(evts);
    			goto __failure_endnest_unlock_cancel_nlfree;
    		}
    	}
    }

    kfree(evts);

    /* path taken when either the array is empty or not, in any case
     * everything ok! */
__end_ok:
	up_read(&le_lock);
    nla_nest_end(skb, nest);
    genlmsg_end(skb, hdr);
    return genlmsg_reply(skb, info);

    /* path taken when something goes wrong... (e.g. skb is full) */
__failure_endnest_unlock_cancel_nlfree:
	nla_nest_end(skb, nest);
__failure_unlock_cancel_nlfree:
	up_read(&le_lock);
	genlmsg_cancel(skb, hdr);
__failure_nlfree:
	nlmsg_free(skb);
	return -EMSGSIZE;
}

int pgtrack_genl_is_tracked_page_doit(struct sk_buff *, struct genl_info *)
{
	return 0;
}

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

int setup_pgtrack_netlink(void)
{
	int rv;

	/* first, setup the fifo, size will be ROUNDED UP TO the nearest 2^n !!! */
	rv = kfifo_alloc(&le, __fifo_bytes(NR_QUEUED_EVENTS), GFP_KERNEL);
	if(rv) {
		scid_err("unable to setup kfifo");
		return rv;
	}

	/* then setup the ordered wq */
	bcast_evt_wq = alloc_ordered_workqueue("scid-bcast-evt", 0);
	if(!bcast_evt_wq) {
		scid_err("unable to allocate wq");
		free_le_kfifo();
		return -1;
	}

	return 0;
}

void teardown_pgtrack_netlink(void)
{
	/* destroy the wq */
	drain_workqueue(bcast_evt_wq);
	destroy_workqueue(bcast_evt_wq);

	/* ... then the fifo */
	free_le_kfifo();
}
