#ifndef SCID_VMFS_H
#define SCID_VMFS_H

#include <linux/mm.h>
#include <linux/list.h>

struct vm_fault_entry {
	struct vm_fault *vmf;
	struct list_head node;
};

DECLARE_PER_CPU(struct list_head, vmfs_head);

void setup_vmfs_pcp_list_heads(void);
void teardown_vmfs_pcp_list_heads(void);

/* caller must ensure preemption disabled */
int got_this_vmf(struct vm_fault*);

#endif
