#include <linux/kprobes.h>
#include <linux/bitops.h>
#include <linux/compiler.h>

#include <vmfs.h>

#define CALLER_DO_FAULT_BITNR 0
#define CALLER_FINISH_FAULT_BITNR 1

/* do_fault hook */

#define do_fault__symbol "do_fault"

static int do_fault__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault *) regs->di;
	
	struct vm_fault_entry *entry = got_this_vmf(vmf);
	if(!entry)
		return 1;

	__set_bit(CALLER_DO_FAULT_BITNR, get_caller_bitmap(entry));

	*((struct vm_fault_entry**)krpi->data) = entry;
	//memcpy(krpi->data, &entry, sizeof(struct vm_fault_entry*));

	return 0;
}

static int do_fault__hkrphook(
		struct kretprobe_instance *krpi, 
		__maybe_unused struct pt_regs *regs)
{
	//struct vm_fault_entry *entry;

	//memcpy(&entry, krpi->data, sizeof(struct vm_fault_entry*));

	unsigned long *caller_bm = 
		get_caller_bitmap(*(struct vm_fault_entry**)krpi->data);

	__clear_bit(CALLER_DO_FAULT_BITNR, caller_bm);
	return 0;
}

struct kretprobe do_fault__krp = {
	.entry_handler = do_fault__ehkrphook,
	.handler = do_fault__hkrphook,
	.kp.symbol_name = do_fault__symbol,
};

/* finish_fault hook */

#define finish_fault__symbol "finish_fault"

static int finish_fault__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault *) regs->di;
	
	struct vm_fault_entry *entry = got_this_vmf(vmf);
	if(!entry)
		return 1;

	__set_bit(CALLER_FINISH_FAULT_BITNR, get_caller_bitmap(entry));

	*((struct vm_fault_entry**)krpi->data) = entry;
	//memcpy(krpi->data, &entry, sizeof(struct vm_fault_entry*));

	return 0;
}

static int finish_fault__hkrphook(
		struct kretprobe_instance *krpi, 
		__maybe_unused struct pt_regs *regs)
{
	//struct vm_fault_entry *entry;

	//memcpy(&entry, krpi->data, sizeof(struct vm_fault_entry*));
	
	unsigned long *caller_bm = 
		get_caller_bitmap(*(struct vm_fault_entry**)krpi->data);

	__clear_bit(CALLER_FINISH_FAULT_BITNR, caller_bm);
	return 0;
}

struct kretprobe finish_fault__krp = {
	.entry_handler = finish_fault__ehkrphook,
	.handler = finish_fault__hkrphook,
	.kp.symbol_name = finish_fault__symbol,
};

/* set_pte_range hook */

#define set_pte_range__symbol "set_pte_range"

#define REQUIRED_CALLER_BITS \
	(CALLER_FINISH_FAULT_BITNR | CALLER_DO_FAULT_BITNR)

/*
 * args that are passed to set_pte_args,
 * forwarded from the entry_handler to the
 * handler, in order to scan affected PTEs
 * post-action
 */
struct set_pte_range_args {
	/* the vm_fault descriptor */
	struct vm_fault *vmf;

	/* the folio that contains page */
	struct folio *folio;

	/* the first page to create a PTE for */
	struct page *page;

	/* the number of PTEs to create */
	unsigned int nr;

	/* starting linear address */
	unsigned long addr;
};

static int set_pte_range__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;
	struct vm_fault_entry *entry;
	unsigned long bm_result;

	entry = got_this_vmf(vmf);
	if(!entry)
		return 1;

	bm_result = *get_caller_bitmap(entry) & REQUIRED_CALLER_BITS;
	if(bm_result != REQUIRED_CALLER_BITS)
		return 1;

	struct set_pte_range_args args = {
		.vmf = vmf,
		.folio = (struct folio*) regs->si,
		.page = (struct page*) regs->dx,
		.nr = (unsigned int) regs->cx,
		.addr = (unsigned long) regs->r8
	};

	memcpy(krpi->data, &args, sizeof(struct set_pte_range_args));

	return 0;
}

#undef REQUIRED_CALLER_BITS

static int set_pte_range__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct set_pte_range_args *args;

	args = (struct set_pte_range_args*) krpi->data;

	//memcpy(&args, krpi->data, sizeof(struct set_pte_range_args));

	//pr_info("args: nr=%d, addr=%px\n", args->nr, (void*)args->addr);
	return 0;
}

struct kretprobe set_pte_range__krp = {
	.entry_handler = set_pte_range__ehkrphook,
	.handler = set_pte_range__hkrphook,
	.kp.symbol_name = set_pte_range__symbol,
	.data_size = sizeof(struct set_pte_range_args),
};
