#ifndef SCID_PGTRACK_GENL_EVENTS_H
#define SCID_PGTRACK_GENL_EVENTS_H

#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/workqueue.h>
#include <linux/rwsem.h>
#include <linux/skbuff.h>
#include <linux/time64.h>

/* nr of queued events in the kfifo */
#define NR_QUEUED_EVENTS 10

/* single event ptr element size in kfifo */
#define EVENT_PTR_SIZE sizeof(struct event*)

/* convert from number of elements to bytes */
#define __fifo_bytes(i) (i * EVENT_PTR_SIZE)

/* kfifo_len(&le) is a multiple of EVENT_PTR_SIZE */
#define __fifo_len() (kfifo_len(&le) / EVENT_PTR_SIZE)

/* see below */
struct event;

#ifndef DISABLE_PAGE_SNAPSHOT

/* fwd decl */
struct page_snap;

#endif /* DISABLE_PAGE_SNAPSHOT */

extern struct workqueue_struct *bcast_evt_wq;
extern struct kfifo le;
extern struct rw_semaphore le_lock;
extern struct kmem_cache *event_cachep;
extern struct kmem_cache *do_event_bcast_work_cachep;

struct do_event_bcast_work {
	struct event *event;
	struct work_struct work;
};

extern void free_le_kfifo(void);

/* these are passed to @args of 
 * event_to_poulate_skb_with and modify routine behaviour
 * based on event type + EVENT_TO_SKB_*
 *
 * ignored with WXWARNING, useful with SNAPSHOTs
 */
#define EVENT_TO_SKB_BCAST ((void*) 0)
#define EVENT_TO_SKB_GET_EVTS ((void*) 1)
#define EVENT_TO_SKB_PEEKONE ((void*) 2)

extern bool event_to_populate_skb_with(
		const struct event *event, struct sk_buff *skb, const void *args);

/* 
 * internal event representation 
 * to avoid the need to keep struct defs in-sync,
 * across the whole project limit this to event 
 * @type identification and private @data casting
 */
enum event_type {
	EVENT_TYPE_WXWARNING,
	EVENT_TYPE_SNAPSHOT,

	NR_EVENT_TYPES
};

struct event {
	enum event_type type;
	time64_t datetime;
	const void *data;
};

#endif
