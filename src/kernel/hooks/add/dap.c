#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <hooks/add/utils.h>

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
	*((struct vm_fault**)krpi->data) = vmf;

	return 0;
}

static bool dap_further_pte_checks(
		pte_t pte, int rw, __maybe_unused int exec,
		__maybe_unused void *args)
{
	if(!(pte_flags(pte) & _PAGE_USER)) {
		scid_err("got a kernel mapping instead of a user one");
		return false;
	}

	if(!rw) {
		scid_err("at this point, an rw mapping was expected");
		return false;
	}

	return true;
}

static int do_anonymous_page__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf;
	unsigned long cpu_flags;

	/* worth any further check? 
	 * In this case, it is enough to check for retval != 0.
	 * Any other vm_fault_reason is either an error (e.g. OOM), 
	 * something related to debugging (HWPOISON) or ->fault related
	 * (DONE_COW)
	 */
	if(regs_return_value(regs) != 0)
		return 0;

	/* copy vmf ptr from entry handler */
	vmf = *((struct vm_fault**)krpi->data);

	/* this could be possible */
	if(!vmf->pte || !vmf->ptl) {
		scid_err("pte or ptl is NULL, this is strange...");
		return 0;
	}

	/* get the page table lock */
	spin_lock_irqsave(vmf->ptl, cpu_flags);

	if(!add_pages_byfolio(vmf->pte, dap_further_pte_checks, NULL, true, NULL))
		scid_err("unable to add pages");

	/* release the page table lock */
	spin_unlock_irqrestore(vmf->ptl, cpu_flags);

	return 0;
}

struct kretprobe do_anonymous_page__krp = {
	.entry_handler = do_anonymous_page__ehkrphook,
	.handler = do_anonymous_page__hkrphook,
	.kp.symbol_name = do_anonymous_page__symbol,
	.data_size = sizeof(struct vm_fault*)
};
