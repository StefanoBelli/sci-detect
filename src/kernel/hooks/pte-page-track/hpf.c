#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT
#	include <linux/rcupdate.h>
#	include <pgtrack.h>
#	include <kpsleepable.h>
#endif

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

	entry = add_vmf(vmf, vmf->flags);
	if(!entry) {
		scid_err("add_vmf failed");
		return 1;
	}

	*((struct vm_fault_entry**)krpi->data) = entry;

	return 0;
}

#ifdef DO_PTE_ALT_PROT

/* avoid locking if already locked */
static inline bool still_locked_vma_or_mmap(vm_fault_t return_flags)
{
	return !(return_flags & (VM_FAULT_RETRY | VM_FAULT_COMPLETED));
}

struct kretprobe handle_pte_fault__krp;

#endif

static int handle_pte_fault__hkrphook(
		struct kretprobe_instance *krpi, __maybe_unused struct pt_regs *regs)
{
	struct vm_fault_entry *vmfe = *((struct vm_fault_entry**) krpi->data);

#ifdef DO_PTE_ALT_PROT
	vm_fault_t retval = regs_return_value(regs);
	struct page_status *pgs;
	pte_t pte;
	unsigned long pfn;
	bool pfn_found;
	pte_t *vmf_ptep = vmf(vmfe)->pte;
	enum fault_flag vmf_flags = orig_flags(vmfe);
	bool skip_mmap_rlock;

	if(unlikely(retval & VM_FAULT_ERROR))
		goto __end;

	skip_mmap_rlock = still_locked_vma_or_mmap(retval);

	if(!vmf_ptep)
		goto __end;

	/* 
	 * try not to acquire the ptl since this is 
	 * only an atomic read, let's see... 
	 */
	pte = ptep_get(vmf_ptep);
	pfn = page_to_pfn(pte_page(pte));

	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(pfn);
	pfn_found = pgs && try_page_status_get(pgs);
	rcu_read_unlock();

	if(likely(pfn_found)) {
		if(likely(!pgs->pap))
			goto __put_pgs_end;

		struct kprobe *mykp = kpat(handle_pte_fault__krp, krpi);
		KPSLEEPABLE(mykp,
				wrex_ptealtprot(
					pgs, vmf_flags, 
					skip_mmap_rlock ? current->mm : NULL,
					vmf(vmfe)->vma, vmf_ptep, 
					vmf(vmfe)->ptl, vmf(vmfe)->address);
		);
	} else 
		goto __end;

__put_pgs_end:
	page_status_put(pgs);
__end:

#endif /* DO_PTE_ALT_PROT */

	del_vmf(vmfe);
	return 0;
}

struct kretprobe handle_pte_fault__krp = {
	.entry_handler = handle_pte_fault__ehkrphook,
	.handler = handle_pte_fault__hkrphook,
	.kp.symbol_name = handle_pte_fault__symbol,
	.data_size = sizeof(struct vm_fault_entry*),
};
