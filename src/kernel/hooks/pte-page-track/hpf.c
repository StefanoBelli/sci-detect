#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT
#	include <linux/rcupdate.h>
#	include <linux/pid.h>
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
	bool locked_mm = false;
	struct mm_struct *target_mm;
	struct kprobe *kp;

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
				task_pid_nr(current), pfn, vmf(vmfe)->real_address);

		/* 
	 	 * this condition checks if either the mmap_read_lock or the per-VMA
	 	 * lock is taken when exiting handle_pte_fault. Check handle_mm_fault code
	 	 * for further details, but this serves to avoid potential deadlock condition
	 	 * when ptealtprot code acquires the target_mm read lock.
	 	 *
	 	 * If, on return, VM_FAULT_RETRY *and* VM_FAULT_COMPLETED are both *NOT* set, the
	 	 * mmap_lock is held.
	 	 *
	 	 * Now, if VM_FAULT_COMPLETED is the one that is set, nothing to do, the mmap_lock
	 	 * is not held anymore, that is, we need to rlock it later on.
	 	 *
	 	 * If VM_FAULT_RETRY is set, instead, this means the mmap_lock may or may not be held:
	 	 * this depends on how handle_mm_fault got called: if FAULT_FLAG_RETRY_NOWAIT is enabled
	 	 * then this means that on retry the mmap_lock is still held, otherwise, the mmap_lock
	 	 * is not held. "@FAULT_FLAG_RETRY_NOWAIT: Don't drop mmap_lock and wait when retrying." 
	 	 *
	 	 * Edit: introduced the check about whether the per-VMA lock or the whole mmap_lock was
	 	 * acquired. This is because we need to, later on, acquire the whole mmap_lock to inspect
	 	 * the mm. Only holding the per-VMA lock is not enough. If the per-VMA lock is held, 
	 	 * then locked_mm = false, otherwise do further checks, as described above.
	 	 *
	 	 * See: https://elixir.bootlin.com/linux/v7.1.4/source/include/linux/mm_types.h#L1751
	 	 */

	 	if(unlikely(!(vmf_flags & FAULT_FLAG_VMA_LOCK))) {
			locked_mm = !(retval & (VM_FAULT_RETRY | VM_FAULT_COMPLETED));
	 		if(retval & VM_FAULT_RETRY)
	 			/* 
	 			 * if we get here, then locked_mm = false, it may change if i
	 			 * FAULT_FLAG_RETRY_NOWAIT is set...
	 			 */
				locked_mm = vmf_flags & FAULT_FLAG_RETRY_NOWAIT;
		}

		/* support for FAULT_FLAG_REMOTE, aka, remote mm */
		target_mm = vmf(vmfe)->vma->vm_mm;

		/*
		 * if FAULT_FLAG_REMOTE is enabled then target_mm != current->mm. Anyway, rlock it
		 * only if needed (that is, if !locked_mm is true) because paths that bring to
		 * handle_mm_fault (GUP and page fault handler) already to the mmap_read_lock on the
		 * target_mm, but within the function (handle_mm_fault) it may happen that the rlock 
		 * is released (so we need to rlock), see locked_mm. Never trylock.
		 */
		DEFINE_MMS_LOCK_CONTROL(mmslk, target_mm, !locked_mm, false);

		kp = kpat(handle_pte_fault__krp, krpi);

		/*
		 * argument @rlkmm indicates whether the current->mm read lock must
		 * be acquired or not (see above),
		 *  rlkmm = true when locked_mm = false (take the lock if the current
		 *    kernel control path released it)
		 *
		 *  rlkmm = false when locked_mm = true (don't attempt to acquire the
		 *    mmap read lock if current kernel control path still got it)
		 */
		wrex_ptealtprot(pgs, vmf_flags, &mmslk, &mpi, snapex, kp);
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
