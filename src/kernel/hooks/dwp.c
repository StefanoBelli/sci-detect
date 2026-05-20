#include <linux/spinlock.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/pgtable.h>

#include <vmfs.h>
#include <logging.h>

/* see below. this is the core fn's prototype */
static inline void __do_wp_page_reuse(struct vm_fault *vmf); 

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
	if(!got_this_vmf(vmf))
		return 1;

	/* almost a noop, just copy params for handler */
	memcpy(krpi->data, &vmf, sizeof(struct vm_fault*));
	return 0;
}


static int do_wp_page__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{

	struct vm_fault *vmf;

	if(regs_return_value(regs) != 0)
		return 0;

	memcpy(&vmf, krpi->data, sizeof(struct vm_fault*));

	/* this is the alternative to hooking into inlined wp_page_reuse:
	 * we try to reconstruct the flow logic that made do_wp_page to
	 * call wp_page_reuse */
	struct vm_area_struct *vma = vmf->vma;
	if(vma->vm_flags & (VM_SHARED | VM_MAYSHARE))
		return 0;

	struct page *page = vmf->page;
	if(!page)
		return 0;

	struct folio *folio = page_folio(page);
	if(!folio)
		return 0;

	/* idk if this is *really* needed */
	if(!folio_try_get(folio))
		return 0;

	/* PageAnonExclusive is set anyway if branch is taken
	 * (see https://elixir.bootlin.com/linux/v7.0.9/source/mm/memory.c#L4235) */
	if(folio_test_anon(folio) && PageAnonExclusive(page)) {
		if(!(vmf->flags & FAULT_FLAG_UNSHARE))
			__do_wp_page_reuse(vmf);
	}

	folio_put(folio);
	return 0;
}

struct kretprobe do_wp_page__krp = {
	.entry_handler = do_wp_page__ehkrphook,
	.handler = do_wp_page__hkrphook,
	.kp.symbol_name = do_wp_page__symbol,
	.data_size = sizeof(struct vm_fault*),
};

static int ____do_wpr_prior_checks(struct vm_fault *vmf)
{
	if(!(vmf->flags & FAULT_FLAG_WRITE)) {
		scid_err("expecting write fault");
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

	pte_t pte = ptep_get(vmf->pte);

	if(pte_none(pte)) {
		scid_err("none pte in vmf");
		goto _____wpr_handler_unlock;
	}

	if(!pte_present(pte)) {
		scid_err("non present pte in vmf");
		goto _____wpr_handler_unlock;
	}

	if(!(vmf->vma->vm_flags & VM_WRITE)) {
		scid_warn("vma without VM_WRITE ???");
		goto _____wpr_handler_unlock;
	}

	unsigned long flags = pte_flags(pte);
	if(!new_pte_from_wpr(flags)) {
		scid_err("not a new pte from wp_page_reuse");
		goto _____wpr_handler_unlock;
	}

	int pte_has_write = pte_write(pte);
	int pte_has_exec = pte_exec(pte);

	if(!pte_has_write) {
		scid_err("expecting a write-enabled pte");
		goto _____wpr_handler_unlock;
	}

	/* release the page table lock */
_____wpr_handler_unlock:
	spin_unlock_irqrestore(vmf->ptl, cpu_flags);
}

#undef new_pte_from_wpr

static inline void __do_wp_page_reuse(struct vm_fault *vmf)
{
	if(!____do_wpr_prior_checks(vmf))
		return;

	____do_wpr_inspect_pte_after(vmf);
}
