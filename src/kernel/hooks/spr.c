#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>

enum caller_kcp_bitnr : unsigned long {
	CALLER_DO_FAULT_BITNR = 0,
	CALLER_FINISH_FAULT_BITNR = 1,
	CALLER_FILEMAP_MAP_PAGES_BITNR = 2,

	NR_CALLER_KCP_BITNRS,
	EVERY_POSSIBLE_CALLER = (1 << NR_CALLER_KCP_BITNRS) - 1,
};

static __always_inline void __raise_caller(
		enum caller_kcp_bitnr caller_bitnr, struct pt_regs *regs) 
{
	struct vm_fault *vmf = (struct vm_fault *) regs->di;
	
	struct vm_fault_entry *entry = got_this_vmf(vmf);
	if(!entry)
		return;

	/* private field itself is used as a bitmap */
	unsigned long *caller_bitmapp = (unsigned long*) &private(entry);

	/* don't need any kind of atomic RMW bitop */
	__set_bit(caller_bitnr, caller_bitmapp);
}

/* filemap_map_pages hook */

#define filemap_map_pages__symbol "filemap_map_pages"

static int filemap_map_pages__phkphook(
		__maybe_unused struct kprobe *kp, 
		struct pt_regs *regs)
{
	__raise_caller(CALLER_FILEMAP_MAP_PAGES_BITNR, regs);
	return 0;
}

struct kprobe filemap_map_pages__kp = {
	.pre_handler = filemap_map_pages__phkphook,
	.symbol_name = filemap_map_pages__symbol,
};

/* do_fault hook */

#define do_fault__symbol "do_fault"

static int do_fault__phkphook(
		__maybe_unused struct kprobe *kp, 
		struct pt_regs *regs)
{
	__raise_caller(CALLER_DO_FAULT_BITNR, regs);
	return 0;
}

struct kprobe do_fault__kp = {
	.pre_handler = do_fault__phkphook,
	.symbol_name = do_fault__symbol,
};

/* finish_fault hook */

#define finish_fault__symbol "finish_fault"

static int finish_fault__phkphook(
		__maybe_unused struct kprobe *kp, 
		struct pt_regs *regs)
{
	__raise_caller(CALLER_FINISH_FAULT_BITNR, regs);
	return 0;
}

struct kprobe finish_fault__kp = {
	.pre_handler = finish_fault__phkphook,
	.symbol_name = finish_fault__symbol,
};

/* set_pte_range hook */

#define set_pte_range__symbol "set_pte_range"

#define REGULAR_SPT_KCP_BITS \
	((1 << CALLER_FINISH_FAULT_BITNR) | (1 << CALLER_DO_FAULT_BITNR))

#define FAULT_AROUND_SPT_KCP_BITS \
	((1 << CALLER_FILEMAP_MAP_PAGES_BITNR) | (1 << CALLER_DO_FAULT_BITNR))

#define is_fault_around_kcp(bm) \
	((bm & EVERY_POSSIBLE_CALLER) == FAULT_AROUND_SPT_KCP_BITS)

#define is_regular_kcp(bm) \
	((bm & EVERY_POSSIBLE_CALLER) == REGULAR_SPT_KCP_BITS)

#define is_legit_kcp(bm) \
	(is_regular_kcp(bm) || is_fault_around_kcp(bm))

/*
 * args that are passed to set_pte_args,
 * forwarded from the entry_handler to the
 * handler, in order to scan affected PTEs
 * post-action. Only meaningful args are left.
 */
struct set_pte_range_args {
	/* the vm_fault descriptor */
	struct vm_fault *vmf;

	/* the number of PTEs to create */
	unsigned int nr;
};

static int set_pte_range__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;
	struct vm_fault_entry *entry;

	entry = got_this_vmf(vmf);
	if(!entry)
		return 1;

	unsigned long caller_bitmap = (unsigned long) private(entry);
	if(!is_legit_kcp(caller_bitmap))
		return 1;

	struct set_pte_range_args args = {
		.vmf = vmf,
		.nr = (unsigned int) regs->cx,
	};

	memcpy(krpi->data, &args, sizeof(struct set_pte_range_args));

	return 0;
}

#undef REQUIRED_CALLER_BITS

static __always_inline void __scan_one_pte(pte_t pte) {
	if(pte_none(pte)) {
		scid_warn("ignoring none pte");
		return;
	}

	if(!pte_present(pte)) {
		scid_warn("ignoring non-present pte");
		return;
	}

	if(!(pte_flags(pte) & _PAGE_USER)) {
		scid_warn("ignoring kernel mapping instead of a user one");
		return;
	}

	struct page *page = pte_page(pte);
	if(!page) {
		scid_warn("ignoring pte (not able to get the page descriptor from it)");
		return;
	}

	int pte_has_rw = pte_write(pte);
	int pte_has_exec = pte_exec(pte);
}

static int set_pte_range__hkrphook(
		struct kretprobe_instance *krpi, 
		__maybe_unused struct pt_regs *regs)
{
	struct set_pte_range_args *args;

	args = (struct set_pte_range_args*) krpi->data;

	if(!args->vmf->ptl || !args->vmf->pte || !args->nr) {
		scid_err("invalid arguments");
		return 0;
	}

	/* 
	 * no need to take the page table lock as it is already held
	 * by the outer finish_fault function.
	 */
	if(!spin_is_locked(args->vmf->ptl)) {
		scid_err("this is strange... ptl should be taken :/");
		return 0;
	}

	/* sets contiguous pages to continuous linear addrs */
	unsigned int nr_pages = args->nr;
	pte_t *cur_ptep = args->vmf->pte;

	do {
		pte_t cur_pte = ptep_get(cur_ptep);
		__scan_one_pte(cur_pte);
		cur_ptep++;
	} while(--nr_pages);

	return 0;
}

struct kretprobe set_pte_range__krp = {
	.entry_handler = set_pte_range__ehkrphook,
	.handler = set_pte_range__hkrphook,
	.kp.symbol_name = set_pte_range__symbol,
	.data_size = sizeof(struct set_pte_range_args),
};
