#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#include <vmfs.h>
#include <logging.h>
#include <hooks/pageutils.h>

#define wp_page_copy__symbol "wp_page_copy"

static int wp_page_copy__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;

	/* are we on the right kernel control path? */
	if(!got_this_vmf(vmf))
		return 1;

	/* consistency checks */
	if(!(vmf->flags & FAULT_FLAG_WRITE)) {
		scid_err("not a write fault");
		return 1;
	}

	if(vmf->flags & FAULT_FLAG_MKWRITE) {
		scid_err("unexpected pte mkwrite fault");
		return 1;
	}

	if(vmf->flags & FAULT_FLAG_REMOTE) {
		scid_err("write fault for remote task");
		return 1;
	}

	if(!(vmf->flags & FAULT_FLAG_ORIG_PTE_VALID)) {
		scid_err("orig_pte is not valid");
		return 1;
	}

	if(pte_none(vmf->orig_pte)) {
		scid_err("orig_pte is none");
		return 1;
	}

	if(!pte_present(vmf->orig_pte)) {
		scid_err("orig_pte is not present");
		return 1;
	}

	if(pte_write(vmf->orig_pte)) {
		scid_err("orig_pte has write flag");
		return 1;
	}

	/* alright, see you on the return handler... */
	*((struct vm_fault**)krpi->data) = vmf;
	//memcpy(krpi->data, &vmf, sizeof(struct vm_fault*));
	return 0;
}

static int wp_page_copy__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf;
	unsigned long cpu_flags;

	if(regs_return_value(regs) != 0)
		return 0;

	/* get vmf pointer from entry handler */
	vmf = *((struct vm_fault**)krpi->data);
	//memcpy(&vmf, krpi->data, sizeof(struct vm_fault*));

	if(!vmf->ptl || !vmf->pte) {
		scid_err("ptl or pte is NULL on ret");
		return 0;
	}

	/* get the page table lock */
	spin_lock_irqsave(vmf->ptl, cpu_flags);

	pte_t pte = ptep_get(vmf->pte);

	if(pte_same(pte, vmf->orig_pte)) {
		scid_warn("COW failed, kernel will retry");
		goto __wpc_handler_unlock;
	}

	/* TODO may need to extend support for files?
	 * Where multiple pages in a single folio are possible*/
	struct page *page = get_one_page_from_pte(pte);
	if(!page) {
		scid_err("unable to get page");
		goto __wpc_handler_unlock;
	}

	int pte_has_write = pte_write(pte);
	int pte_has_exec = pte_exec(pte);

	if(!pte_has_write) {
		scid_warn("COW failed, kernel will retry");
		goto __wpc_handler_unlock;
	}

	/* put the page table lock*/
__wpc_handler_unlock:
	spin_unlock_irqrestore(vmf->ptl, cpu_flags);

	return 0;
}

struct kretprobe wp_page_copy__krp = {
	.entry_handler = wp_page_copy__ehkrphook,
	.handler = wp_page_copy__hkrphook,
	.kp.symbol_name = wp_page_copy__symbol,
	.data_size = sizeof(struct vm_fault*),
};
