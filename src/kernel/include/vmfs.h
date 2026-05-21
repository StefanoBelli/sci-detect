#ifndef SCID_VMFS_H
#define SCID_VMFS_H

#include <linux/mm.h>

/* keep it opaque. def. inside vmfs.c */
struct vm_fault_entry;

int setup_vmfs_pcp_lists(void);
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

enum caller_enum {
	CALLER_FINISH_FAULT = (1 << 0),
};

int is_caller_vmfe(struct vm_fault_entry *, enum caller_enum);
void set_caller_vmfe(struct vm_fault_entry *, enum caller_enum);
void unset_caller_vmfe(struct vm_fault_entry *, enum caller_enum);

#endif
