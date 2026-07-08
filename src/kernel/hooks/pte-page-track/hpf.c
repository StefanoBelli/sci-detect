#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <ptealtprot.h>
#include <pgtrack.h>

#define handle_pte_fault__symbol "handle_pte_fault"

static int handle_pte_fault__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	WARN_ON(irqs_disabled());

	struct vm_fault *vmf;
	struct vm_fault_entry *entry;

	vmf = (struct vm_fault*) regs->di;
	if(!vmf) {
		scid_warn("vmf is NULL");
		return 1;
	}

	entry = add_vmf(vmf);
	if(!entry) {
		scid_err("add_vmf failed");
		return 1;
	}

	*((struct vm_fault_entry**)krpi->data) = entry;

	return 0;
}

static int handle_pte_fault__hkrphook(
		struct kretprobe_instance *krpi, __maybe_unused struct pt_regs *regs)
{
	struct vm_fault_entry *vmfe = *((struct vm_fault_entry **)krpi->data);
	__maybe_unused struct vm_fault *_vmf = vmf(vmfe);
	__maybe_unused struct page_status *pgs;
	struct page *page;

	del_vmf(vmfe);

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	if(!_vmf->pte)
		return 0;

	page = pte_page(ptep_get(_vmf->pte));

	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(page_to_pfn(page));
	if(pgs && pgs->pg_mms) {
		if(pgs->pg_mms->init_task == current) {
			pgs->pg_mms->init_task = NULL;
			spin_unlock(&pgs->pg_mms->lock);
		} else {
			spin_lock(&pgs->pg_mms->lock);
			
			spin_unlock(&pgs->pg_mms->lock);
		}
	}
	rcu_read_unlock();

#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

	return 0;
}

struct kretprobe handle_pte_fault__krp = {
	.entry_handler = handle_pte_fault__ehkrphook,
	.handler = handle_pte_fault__hkrphook,
	.kp.symbol_name = handle_pte_fault__symbol,
	.data_size = sizeof(struct vm_fault_entry*),
};
