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

struct kretprobe handle_pte_fault__krp;

#define DEFINE_MPI_BY_VMF(__mpivar, __vmf) \
	struct my_pte_info __mpivar = { \
		.vma = (__vmf)->vma, \
		.ptlp = (__vmf)->ptl, \
		.ptep = (__vmf)->pte, \
		.addr = (__vmf)->address, \
	}

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
	bool locked_mm;

	if(unlikely(retval & VM_FAULT_ERROR))
		goto __end;

	if(!vmf_ptep)
		goto __end;

	/* 
	 * try not to acquire the ptl since this is 
	 * only an atomic read, let's see... 
	 */
	pte = ptep_get(vmf_ptep);
	if(unlikely(pte_none(pte) || !pte_present(pte)))
		goto __end;

	pfn = page_to_pfn(pte_page(pte));

	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(pfn);
	pfn_found = pgs && try_page_status_get(pgs);
	rcu_read_unlock();

	if(likely(pfn_found)) {
		if(likely(!pgs->pap))
			goto __put_pgs_end;

		DEFINE_MPI_BY_VMF(mpi, vmf(vmfe));
		DEFINE_SNAPSHOT_EXTRAS_WITH_PTR(snapex, 
				task_pid_vnr(current), pfn, vmf(vmfe)->real_address);

		/* 
	 	 * this condition checks if either the mmap_read_lock or the per-VMA
	 	 * lock is taken when exiting handle_pte_fault. Check handle_mm_fault code
	 	 * for further details, but this serves to avoid potential deadlock condition
	 	 * when ptealtprot code acquires the current->mm read lock.
	 	 */
		locked_mm = !(retval & (VM_FAULT_RETRY | VM_FAULT_COMPLETED));

		/*
		 * argument @rlkmm indicates whether the current->mm read lock must
		 * be acquired or not (see above),
		 *  rlkmm = true when locked_mm = false (take the lock if the current
		 *    kernel control path released it)
		 *
		 *  rlkmm = false when locked_mm = true (don't attempt to acquire the
		 *    mmap read lock if current kernel control path still got it)
		 */
		wrex_ptealtprot(pgs, vmf_flags, !locked_mm, 
				&mpi, snapex, kpat(handle_pte_fault__krp, krpi));
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
