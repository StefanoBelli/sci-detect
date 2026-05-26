#include <linux/spinlock.h>
#include <linux/kprobes.h>
#include <linux/pgtable.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <hooks/add/utils.h>

/* since private may be changed frequently... */
static_assert(
		__builtin_types_compatible_p(
			typeof(private((struct vm_fault_entry*)0)),
			typeof(void*)));

#define finish_mkwrite_fault__symbol "finish_mkwrite_fault"

static int finish_mkwrite_fault__phkphook(
		__maybe_unused struct kprobe *kp, 
		struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;

	struct vm_fault_entry *entry = got_this_vmf(vmf);
	if(!entry)
		return 0;

	private(entry) = (void*) 2;
	return 0;
}

struct kprobe finish_mkwrite_fault__kp = {
	.pre_handler = finish_mkwrite_fault__phkphook,
	.symbol_name = finish_mkwrite_fault__symbol,
};

/*
 * this was wp_page_reuse, which I didn't see
 * initially, that is inlined (so, no kprobes on it)
 *
 * converting it into a hook for do_wp_page
 */

#define do_wp_page__symbol "do_wp_page"

static int do_wp_page__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;

	/* are we on the right kernel control path? */
	struct vm_fault_entry *entry = got_this_vmf(vmf);
	if(!entry)
		return 1;

	/* almost a noop, just copy params for handler */
	*((struct vm_fault_entry**)krpi->data) = entry;
	return 0;
}

/* see below. this is the core fn's prototype */
static inline void __do_wp_page_reuse(struct vm_fault *vmf); 

/* this is the alternative to hooking into inlined wp_page_reuse:
 * we try to reconstruct the flow logic that made do_wp_page to
 * call wp_page_reuse 
 */

static int do_wp_page__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	struct vm_fault_entry *entry = *((struct vm_fault_entry**)krpi->data);
	struct vm_fault *vmf;

	/* if wp_page_copy got executed, situation got handled in hooks in wpc.c.
	 * Nothing to do here. */
	if(private(entry) == (void*) 1)
		return 0;

	unsigned long rrv = regs_return_value(regs);

	/* if finish_mkwrite_fault got executed and its rv = 0, wp_page_reuse got called */
	if(private(entry) == (void*) 2) {
		/* but here we check for outer do_wp_page rv, and the following can happen:
		 *  - wp_page_shared: finish_mkwrite_fault successful if rv = 0 or rv = VM_FAULT_COMPLETED 
		 *  - wp_pfn_shared: finsih_mkwrite_fault successful if rv = 0 */
		if(
				(!rrv || (rrv & VM_FAULT_COMPLETED)) && 
				!(rrv & (VM_FAULT_ERROR | VM_FAULT_NOPAGE))) {
			vmf = vmf(entry);
			goto __do_wpr;
		}

		return 0;
	}

	/* for the remaining situations,
	 * it is enough to just check if rrv != 0 */
	if(rrv)
		return 0;

	vmf = vmf(entry);

	struct vm_area_struct *vma = vmf->vma;
	bool shared = vma->vm_flags & (VM_SHARED | VM_MAYSHARE);
	struct page *page = vmf->page;

	/* here, the following expression:
	 * expr = !vma->vm_ops || !vma->vm_ops->pfn_mkwrite (or ->page_mkwrite)
	 * always returns true, because the situation was already
	 * handled by finish_mkwrite_fault (the !expr). 
	 * If we got here, then for sure finish_mkwrite_fault didn't get called 
	 * and expr is always true. Therefore, superfluous checks got removed.
	 * (see wp_pfn_shared, wp_page_shared and how they get called in do_wp_page)
	 */
	if(shared)
		goto __do_wpr;
	else {
		if(!page)
			return 0;

		struct folio *folio = page_folio(page);
		if(!folio)
			return 0;

		if(!folio_try_get(folio)) {
			scid_err("unable to get folio");
			return 0;
		}

		bool folio_anon = folio_test_anon(folio);
		folio_put(folio);
		bool unshare = vmf->flags & FAULT_FLAG_UNSHARE;
		bool page_anon_excl = PageAnonExclusive(page);

		if(folio_anon && page_anon_excl && !unshare)
			goto __do_wpr;
	}
	
	return 0;

__do_wpr:
	__do_wp_page_reuse(vmf);
	return 0;
}

struct kretprobe do_wp_page__krp = {
	.entry_handler = do_wp_page__ehkrphook,
	.handler = do_wp_page__hkrphook,
	.kp.symbol_name = do_wp_page__symbol,
	.data_size = sizeof(struct vm_fault_entry*),
};

static int ____do_wpr_prior_checks(struct vm_fault *vmf)
{
	if(!(vmf->flags & FAULT_FLAG_WRITE) && !(vmf->flags & FAULT_FLAG_MKWRITE)) {
		scid_err("expecting write or mkwrite fault");
		return 0;
	}

	if(!(vmf->flags & FAULT_FLAG_ORIG_PTE_VALID)) {
		scid_err("orig_pte should be valid");
		return 0;
	}

	if(vmf->flags & FAULT_FLAG_REMOTE) {
		scid_err("this is unexpected...");
		return 0;
	}

	if(pte_none(vmf->orig_pte)) {
		scid_err("orig_pte is none");
		return 0;
	}

	if(!pte_present(vmf->orig_pte)) {
		scid_err("orig_pte is not present");
		return 0;
	}

	if(pte_write(vmf->orig_pte)) {
		scid_err("orig_pte is already writeable");
		return 0;
	}

	return 1;
}

#define new_pte_from_wpr(flgs) ( \
	(flags & _PAGE_ACCESSED) && \
	(flags & _PAGE_DIRTY) && \
	(flags & _PAGE_SOFT_DIRTY))

static bool dwp_further_pte_checks(
		pte_t pte, int rw, 
		__maybe_unused int exec, void* args)
{
	struct vm_area_struct *vma = (struct vm_area_struct*) args;

	if(!(vma->vm_flags & VM_WRITE)) {
		scid_warn("vma without VM_WRITE ???");
		return false;
	}

	unsigned long flags = pte_flags(pte);
	if(!new_pte_from_wpr(flags)) {
		scid_err("not a new pte from wp_page_reuse");
		return false;
	}

	if(!rw) {
		scid_err("expecting a write-enabled pte");
		return false;
	}

	return true;
}

#undef new_pte_from_wpr

static void ____do_wpr_inspect_pte_after(struct vm_fault *vmf)
{
	unsigned long cpu_flags;

	/* check if pte in vmf is valid, post-wp_page_reuse */
	if(!vmf->ptl || !vmf->pte) {
		scid_err("invalid pte in vmf");
		return;
	}

	/* get the page table lock */
	spin_lock_irqsave(vmf->ptl, cpu_flags);

	if(!add_one_page(vmf->pte, dwp_further_pte_checks, vmf->vma, NULL))
		scid_err("unable to add page");

	/* release the page table lock */
	spin_unlock_irqrestore(vmf->ptl, cpu_flags);
}

static inline void __do_wp_page_reuse(struct vm_fault *vmf)
{
	if(!____do_wpr_prior_checks(vmf))
		return;

	____do_wpr_inspect_pte_after(vmf);
}
