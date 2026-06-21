#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/compiler.h>
#include <asm-generic/bitsperlong.h>

#include <vmfs.h>
#include <logging.h>
#include <hooks/pte-page-track/utils/addpages.h>
#include <testing/testing.h>

#define MY_TESTING_SUBSYS_NAME "pte-page-track-spr-hook"

typedef unsigned long __ckb_type;

/* ensure enough space for a 64 bit-sized (long) bitmap, 
 * useful if "private" field of struct vm_fault_entry gets changed
 * and I forget about this */
static_assert(
		(sizeof(__ckb_type) * 8) == BITS_PER_LONG && 
		(sizeof(private((struct vm_fault_entry*)0)) * 8) == BITS_PER_LONG);

enum caller_kcp_bitnr : __ckb_type {
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
	__testing("caller-fmp");
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
	__testing("caller-df");
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
	__testing("caller-ff");
	__raise_caller(CALLER_FINISH_FAULT_BITNR, regs);
	return 0;
}

struct kprobe finish_fault__kp = {
	.pre_handler = finish_fault__phkphook,
	.symbol_name = finish_fault__symbol,
};

/* set_pte_range hook */

#define set_pte_range__symbol "set_pte_range"

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

/* 
 * - kernel control path ("kcp") made of multiple callers obv.
 * - optimizing compiler should be able to reduce bitwise ops a lot,
 * reducing work at runtime
 */
#define REGULAR_SPT_KCP_BITS \
	((1 << CALLER_FINISH_FAULT_BITNR) | (1 << CALLER_DO_FAULT_BITNR))

#define FAULT_AROUND_SPT_KCP_BITS \
	((1 << CALLER_FILEMAP_MAP_PAGES_BITNR) | (1 << CALLER_DO_FAULT_BITNR))

#define FALLBACK_SPT_KCP_BITS \
	(REGULAR_SPT_KCP_BITS | FAULT_AROUND_SPT_KCP_BITS)

#define is_fault_around_kcp(bm) \
	((bm & EVERY_POSSIBLE_CALLER) == FAULT_AROUND_SPT_KCP_BITS)

#define is_regular_kcp(bm) \
	((bm & EVERY_POSSIBLE_CALLER) == REGULAR_SPT_KCP_BITS)

#define is_fallback_kcp(bm) \
	((bm & EVERY_POSSIBLE_CALLER) == FALLBACK_SPT_KCP_BITS)

#define is_legit_kcp(bm) \
	(is_regular_kcp(bm) || is_fault_around_kcp(bm) || is_fallback_kcp(bm))

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

	__testing("entry-ok");

	struct set_pte_range_args args = {
		.vmf = vmf,
		.nr = (unsigned int) regs->cx,
	};

	memcpy(krpi->data, &args, sizeof(struct set_pte_range_args));

	return 0;
}

static bool spr_further_pte_checks(
		pte_t pte, __maybe_unused int rw, 
		__maybe_unused int exec, __maybe_unused void* args)
{
	if(!(pte_flags(pte) & _PAGE_USER)) {
		scid_warn("ignoring kernel mapping instead of a user one");
		return false;
	}

	return true;
}

static int set_pte_range__hkrphook(
		struct kretprobe_instance *krpi, 
		__maybe_unused struct pt_regs *regs)
{
	struct set_pte_range_args *args;

	args = (struct set_pte_range_args*) krpi->data;

	if(!args->vmf->ptl) {
		scid_err("ptl is NULL");
		return 0;
	}

	if(!args->vmf->pte) {
		scid_err("ptep is NULL");
		return 0;
	}

	if(!args->nr) {
		scid_err("nr is 0");
		return 0;
	}

	__testing("return-ok");

	/* 
	 * no need to take the page table lock as it is already held
	 * by the outer finish_fault function.
	 */
	if(!spin_is_locked(args->vmf->ptl)) {
		scid_err("this is strange... ptl should be taken :/");
		return 0;
	}

	struct pg_track_forward_args pgt_args = {
		.creat = true,
		.va = args->vmf->real_address,
	};

	/* sets contiguous pages to continuous linear addrs */
	if(!add_pages_bynr(
				args->vmf->pte, spr_further_pte_checks, NULL, 
				args->nr, &pgt_args))
		scid_err("unable to add pages");
	else
		__testing("pages-ok");

	return 0;
}

#undef is_regular_kcp
#undef is_fault_around_kcp
#undef is_legit_kcp
#undef REGULAR_SPT_KCP_BITS
#undef FAULT_AROUND_SPT_KCP_BITS

struct kretprobe set_pte_range__krp = {
	.entry_handler = set_pte_range__ehkrphook,
	.handler = set_pte_range__hkrphook,
	.kp.symbol_name = set_pte_range__symbol,
	.data_size = sizeof(struct set_pte_range_args),
};
