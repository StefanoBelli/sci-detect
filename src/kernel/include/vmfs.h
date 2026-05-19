#ifndef SCID_VMFS_H
#define SCID_VMFS_H

#include <linux/mm.h>
#include <linux/list.h>

/* keep it opaque */
struct vm_fault_entry;

void setup_vmfs_pcp_lists(void);
void teardown_vmfs_pcp_lists(void);

/* caller must ensure preemption disabled */
int got_this_vmf(struct vm_fault*);

/* caller must ensure preemption disabled */
struct vm_fault_entry* add_vmf(struct vm_fault*);

/* caller must ensure preemption disabled */
void del_vmf(struct vm_fault_entry*);

#endif
