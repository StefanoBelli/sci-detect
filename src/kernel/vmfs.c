#include <linux/percpu.h>
#include <linux/slab.h>

#include <vmfs.h>

DEFINE_PER_CPU(struct list_head, vmfs_head);

void setup_vmfs_pcp_list_heads(void) 
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct list_head *lh = per_cpu_ptr(&vmfs_head, cpu);
		INIT_LIST_HEAD(lh);
	}
}

void teardown_vmfs_pcp_list_heads(void) 
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct list_head *lh = per_cpu_ptr(&vmfs_head, cpu);
		struct vm_fault_entry *entry;
		struct vm_fault_entry *tmp;

		list_for_each_entry_safe(entry, tmp, lh, node) {
			kfree(entry);
		}
	}
}

int got_this_vmf(struct vm_fault *vmf) {
	struct vm_fault_entry *entry;
	struct list_head *lh = this_cpu_ptr(&vmfs_head);
	list_for_each_entry(entry, lh, node) {
		if(vmf == entry->vmf)
			return 1;
	}

	return 0;
}

