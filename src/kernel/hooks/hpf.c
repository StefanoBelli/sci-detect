#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/percpu.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>

#define handle_pte_fault__symbol "handle_pte_fault"
static int handle_pte_fault__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	struct list_head *head;
	struct vm_fault_entry *entry;

	if((entry = kzalloc(sizeof(struct vm_fault_entry), GFP_ATOMIC)) == NULL) {
		scid_err("memory exhausted");
		return 1;
	}

	entry->vmf = (struct vm_fault*) regs->di;
	
	head = this_cpu_ptr(&vmfs_head);
	list_add(&entry->node, head);

	memcpy(krpi->data, &entry, sizeof(struct vm_fault_entry*));

	return 0;
}

static int handle_pte_fault__hkrphook(
		struct kretprobe_instance *krpi, __maybe_unused struct pt_regs *regs)
{
	struct vm_fault_entry *entry;

	memcpy(&entry, krpi->data, sizeof(struct vm_fault_entry*));

	list_del(&entry->node);
	kfree(entry);

	return 0;
}

struct kretprobe handle_pte_fault__krp = {
	.entry_handler = handle_pte_fault__ehkrphook,
	.handler = handle_pte_fault__hkrphook,
	.kp.symbol_name = handle_pte_fault__symbol,
	.data_size = sizeof(struct vm_fault_entry*),
};
