#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/spinlock.h>

#include <vmfs.h>
#include <logging.h>
#include <hooks/pageutils.h>

#define do_anonymous_page__symbol "do_anonymous_page"

static int do_anonymous_page__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;

	/* are we on the right kernel control path? */
	if(!got_this_vmf(vmf))
		return 1;

	/* don't care about zero page (demand paging, first read) */
	if(!(vmf->flags & FAULT_FLAG_WRITE))
		return 1;

	/* just some consistency checks */
	if(vmf->pte) {
		scid_err("pte is not NULL");
		return 1;
	}

	if(vmf->real_address >= TASK_SIZE) {
		scid_err("address too high (kernel), we care about user");
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
	if(!vmf->pte || !vmf->ptl) {
		scid_err("pte or ptl is NULL, this is strange...");
		return 0;
	}

	/* get the page table lock */
	spin_lock_irqsave(vmf->ptl, cpu_flags);

	pte_t first_pte = ptep_get(vmf->pte);
	if(pte_none(first_pte)) {
		scid_err("zeroed pte");
		goto __dap_handler_unlock;
	}

	if(!pte_present(first_pte)) {
		scid_err("not-present pte");
		goto __dap_handler_unlock;
	}

	/* should not happen in any case... */
	if(!(pte_flags(first_pte) & _PAGE_USER)) {
		scid_err("got a kernel mapping instead of a user one");
		goto __dap_handler_unlock;
	}

	struct page *page = get_one_page_from_pte(first_pte);
	if(!page) {
		scid_err("unable to get page");
		goto __dap_handler_unlock;
	}

	int pte_has_rw = pte_write(first_pte);
	int pte_has_exec = pte_exec(first_pte);

	if(!pte_has_rw) {
		scid_err("at this point, an rw mapping was expected");
		goto __dap_handler_unlock;
	}

	//pr_info("page is rw: %d, exec: %d\n", pte_has_rw, pte_has_exec);

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
