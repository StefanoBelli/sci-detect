#ifndef SCID_VMFS_H
#define SCID_VMFS_H

#include <linux/mm.h>
#include <linux/rwlock.h>

/* exposed, not opaque, to avoid function call overhead */
struct vm_fault_entry {
	struct {
		/* "key" of the main kernel control path */
		struct vm_fault *vmf;

		/* don't touch: needed to del_vmf - points to pcp-list lock */
		rwlock_t *list_lock; 

		/* private hooks data, depends on kernel control path */
		void *private;
	} value;

	/* don't touch */
	struct hlist_node node;
};

/* you may access these fields */
#define vmf(entry) ((entry)->value.vmf)
#define private(entry) ((entry)->value.private)

void setup_vmfs_pcp_lists(void);
void teardown_vmfs_pcp_lists(void);

/* 
 * Check if probes are being called from the
 * right kernel control path
 *
 * Returns 1 if this is true, 0 otherwise.
 *
 * Caller must ensure preemption disabled */
struct vm_fault_entry* got_this_vmf(struct vm_fault*);

/* 
 * Add the vmf used to check the kcp.
 *
 * Returns NULL if memory exhausted, ptr to the
 * entry otherwise.
 *
 * Caller must ensure preemption disabled */
struct vm_fault_entry* add_vmf(struct vm_fault*);


/*
 * Delete the vmf, this kcp is done and no more 
 * valid
 *
 * Whether preemption is enabled or not, doesn't
 * matter
 */
void del_vmf(struct vm_fault_entry*);


#endif
