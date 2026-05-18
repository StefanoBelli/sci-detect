#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/spinlock.h>

#include <vmfs.h>
#include <modname.h>

#define do_anonymous_page__symbol "do_anonymous_page"
static int do_anonymous_page__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;

	/* don't care about zero page (demand paging, first read) */
	if(!(vmf->flags & FAULT_FLAG_WRITE))
		return 1;

	/* are we on the right kernel control path? */
	if(!got_this_vmf(vmf))
		return 1;

	/* just some consistency checks */
	if(vmf->pte) {
		pr_err(MODNAME ": pte is not null\n");
		return 1;
	}

	if(vmf->ptl && spin_is_locked(vmf->ptl)) {
		pr_err(MODNAME ": spin is locked\n");
		return 1;
	}

	if(vmf->real_address >= TASK_SIZE) {
		pr_err(MODNAME ": very high linear address (kernel)\n");
		return 1;
	}

	/* ok then, pass vmf ptr to the handler */
	memcpy(krpi->data, &vmf, sizeof(struct vm_fault*));

	return 0;
}

static int do_anonymous_page__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf;
	unsigned long cpu_flags;

	/* worth any further check? */
	if(regs_return_value(regs) != 0)
		return 0;

	/* copy vmf ptr from entry handler */
	memcpy(&vmf, krpi->data, sizeof(struct vm_fault*));

	/* this could be possible */
	if(!vmf->pte || !vmf->ptl)
		return 0;

	/* get the page table lock */
	spin_lock_irqsave(vmf->ptl, cpu_flags);

	pte_t *first_pte = vmf->pte;
	if(pte_none(*first_pte))
		goto __dap_handler_unlock;

	if(!pte_present(*first_pte))
		goto __dap_handler_unlock;

	struct page *page = pte_page(*first_pte);
	if(!page)
		goto __dap_handler_unlock;

	/* should not happen in any case... */
	if(!(pte_flags(*first_pte) & _PAGE_USER))
		goto __dap_handler_unlock;

	/* every page is part of a folio */
	struct folio *folio = page_folio(page);

	/* TODO check if this makes sense */
	if(folio_nr_pages(folio) > 1)
		goto __dap_handler_unlock;

	int pte_has_rw = pte_write(*first_pte);
	int pte_has_exec = pte_exec(*first_pte);

	pr_info("page is rw: %d, exec: %d\n", pte_has_rw, pte_has_exec);

	/* release the page table lock */
__dap_handler_unlock:
	spin_unlock_irqrestore(vmf->ptl, cpu_flags);

	return 0;
}

struct kretprobe do_anonymous_page__krp = {
	.entry_handler = do_anonymous_page__ehkrphook,
	.handler = do_anonymous_page__hkrphook,
	.kp.symbol_name = do_anonymous_page__symbol,
	.data_size = sizeof(struct vm_fault*)
};
