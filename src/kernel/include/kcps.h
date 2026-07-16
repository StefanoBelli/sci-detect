#ifndef SCID_KCPS_H
#define SCID_KCPS_H

#include <linux/types.h>
#include <linux/spinlock_types.h>

struct kcp_entry {
	void *data;
	u64 key;

#ifdef CONFIG_SMP
	rwlock_t *list_lock;
#endif

	struct hlist_node node;
};


/**
 * setup_kcps_pcp_lists - setup per-cpu hashtables and whatever
 * is needed to use kcps
 */
int setup_kcps_pcp_lists(void);

/**
 * teardown_kcps_pcp_lists - teardown kcps
 */
void teardown_kcps_pcp_lists(void);

/**
 * User of kcps must provide the comparator when needed
 * to lookup
 */
typedef bool (*kcp_compare_fpt)(struct kcp_entry *kcpe, u64 key);

/**
 * got_this_kcp: check if object with key @key is around
 *
 * Caller must disable preemption.
 *
 * @key: the key to look for
 * @compare: the object comparator
 *
 * Returns: the kcpe object if found, NULL otherwise
 */
struct kcp_entry* got_this_kcp(u64 key, kcp_compare_fpt compare);

/**
 * add_kcp - add kcp to the percpu HT
 * 
 * Caller must disable preemption.
 *
 * @key: the key
 * @data: the specific data
 *
 * Returns: the added kcpe
 */
struct kcp_entry *add_kcp(u64 key, void* data); 

/**
 * del_kcp_bykey - del kcp by key
 *
 * @key: the key to lookup
 * @compare: the object comparator
 */
void del_kcp_bykey(u64 key, kcp_compare_fpt compare);

#endif
