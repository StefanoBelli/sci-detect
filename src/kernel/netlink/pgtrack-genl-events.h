#ifndef SCID_PGTRACK_GENL_EVENTS_H
#define SCID_PGTRACK_GENL_EVENTS_H

#include <linux/kfifo.h>
#include <linux/workqueue.h>
#include <linux/rwsem.h>
#include <linux/skbuff.h>

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

extern struct workqueue_struct *bcast_evt_wq;
extern struct kfifo le;
extern struct rw_semaphore le_lock;

extern void free_le_kfifo(void);
extern bool event_to_populate_skb_with(
		const struct event *event, struct sk_buff *skb, const void *args);

/* internal event representation */

#define EVENT_TO_SKB_BCAST ((void*) 0)
#define EVENT_TO_SKB_GET_EVTS ((void*) 1)
#define EVENT_TO_SKB_PEEKONE ((void*) 2)

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

#endif
