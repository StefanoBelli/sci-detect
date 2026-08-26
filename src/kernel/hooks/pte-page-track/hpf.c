#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <ptealtprot.h>
#include <hooks/activekps.h>

#ifdef DO_PTE_ALT_PROT
#	include <linux/rcupdate.h>
#	include <linux/pid.h>
#	include <pgtrack.h>
#	include <kpsleepable.h>
#	include <resolve_syms/pte_offset_map_lock.h>
#endif

#define handle_pte_fault__symbol "handle_pte_fault"

struct kretprobe handle_pte_fault__krp;

static int handle_pte_fault__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	show_kp_nmissed(handle_pte_fault__krp.kp, "handle_pte_fault");

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

#define DEFINE_MPI(__mpivar, __vmf, __ptlp, __ptep) \
	struct my_pte_info __mpivar = { \
		.vma = (__vmf)->vma, \
		.ptlp = (__ptlp), \
		.ptep = (__ptep), \
		.addr = (__vmf)->address, \
	}

static inline void  __do_pte_fixup_ptealtprot(
		struct page_status *pgs, unsigned long addr, struct vm_area_struct *vma,
		pte_t *ptep, spinlock_t *ptlp, struct kprobe *kp)
{
	struct my_pte_info mpi = {
		.addr = addr,
		.ptep = ptep,
		.ptlp = ptlp,
		.vma = vma,
	};
	
	pte_fixup_ptealtprot(pgs, &mpi, kp);
}

static bool __lock_mm(struct mms_lock_control *mmslk, struct kprobe *kp)
{
	if(!mmslk->rlock_target_mm) {
		if(mmslk->target_vma)
			vma_assert_locked(mmslk->target_vma);
		else
			mmap_assert_locked(mmslk->target_mm);

		return true;
	}

	if(!mmslk->trylock)
		KPSLEEPABLE(kp, 
				mmap_read_lock(mmslk->target_mm);
		);
	else {
		if(!mmap_read_trylock(mmslk->target_mm)) {
			scid_warn("unable to acquire mmap_read_trylock");
			return false;
		}
	}

	return true;
}

static void __unlock_mm(struct mms_lock_control *mmslk)
{
	if(!mmslk->rlock_target_mm) {
		if(mmslk->target_vma)
			vma_assert_locked(mmslk->target_vma);
		else
			mmap_assert_locked(mmslk->target_mm);

		return;
	}

	mmap_read_unlock(mmslk->target_mm);
}

static void enforce_faulted_around_ptes(
		struct list_head *head, unsigned long my_address, 
		struct vm_area_struct *vma, struct mms_lock_control *mmslk, 
		struct kprobe *kp)
{
	struct faulted_around_pte *entry;
	struct faulted_around_pte *tmp;
	struct page_status *pgs;
	unsigned long pfn;
	pte_t pte;
	bool pgs_ok;
	spinlock_t *ptl;

	list_for_each_entry_safe(entry, tmp, head, node) {
		if(entry->addr == my_address)
			goto __cleanup_and_continue;

		/* this should be the same for every ptep here (same PTE table) */
		ptl = ptep_lockptr(vma->vm_mm, entry->ptep);

		spin_lock(ptl);
		pte = ptep_get(entry->ptep);
		pfn = page_to_pfn(pte_page(pte));
		spin_unlock(ptl);

		rcu_read_lock();
		pgs = lookup_pfn_pgtrack(pfn);
		pgs_ok = pgs && try_page_status_get(pgs);
		rcu_read_unlock();

		if(unlikely(!pgs_ok))
			goto __cleanup_and_continue;

		if(likely(!READ_ONCE(pgs->pap)))
			goto __pgs_put;

		if(pgs->pap->init) {
			DEFINE_SNAPSHOT_EXTRAS_WITH_PTR(snpex, 
					task_pid_nr(current), pfn, entry->addr);

			none_ptealtprot(pgs, mmslk, snpex, kp);
			goto __pgs_put;
		}

		if(!__lock_mm(mmslk, kp))
			goto __pgs_put;

		__do_pte_fixup_ptealtprot(pgs, entry->addr, vma, entry->ptep, ptl, kp);
		__unlock_mm(mmslk);

__pgs_put:
		page_status_put(pgs);
__cleanup_and_continue:
		list_del(&entry->node);
		kfree(entry);
	}
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
	pte_t *ptep = vmf(vmfe)->pte;
	spinlock_t *ptl;
	enum fault_flag vmf_flags = orig_flags(vmfe);
	bool locked = false; /* whether the per-VMA lock or mmap_lock is acquired */
	struct mm_struct *target_mm = vmf(vmfe)->vma->vm_mm; /* support for FAULT_FLAG_REMOTE */
	struct vm_area_struct *target_vma = NULL;
	struct kprobe *kp;

	/*
	 * If errored, don't do anything, just terminate.
	 */
	if(unlikely(retval & VM_FAULT_ERROR))
		goto __end_fail;

	kp = kpat(handle_pte_fault__krp, krpi);

	/* 
	 * this condition checks if either the mmap_read_lock or the per-VMA
	 * lock is taken when exiting handle_pte_fault. Check handle_mm_fault code
	 * for further details, but this serves to avoid potential deadlock condition
	 * when ptealtprot code acquires the target_mm mmap_lock (read).
	 *
	 * If, on return, VM_FAULT_RETRY *and* VM_FAULT_COMPLETED are both *NOT* set, the
	 * mmap_lock or the per-VMA lock is held.
	 *
	 * Now, if VM_FAULT_COMPLETED is the one that is set, nothing to do, the mmap_lock 
	 * (or the per-VMA lock) is not held anymore, that is, we need to rlock it later on.
	 *
	 * If VM_FAULT_RETRY is set and the per-VMA lock was held, nothing to do,
	 * it is not held anymore.
	 *
	 * If VM_FAULT_RETRY is set and the mmap_lock is held, this means it may or may not be held:
	 * this depends on how handle_mm_fault got called: if FAULT_FLAG_RETRY_NOWAIT is enabled
	 * then this means that on retry the mmap_lock is still held, otherwise, 
	 * the mmap_lock is not held. "@FAULT_FLAG_RETRY_NOWAIT: Don't drop mmap_lock and wait when retrying." 
	 *
	 * Why the split on VM_FAULT_RETRY? Because FAULT_FLAG_RETRY_NOWAIT is actively used by GUP but
	 * not the #PF handler and GUP only uses the mmap_read_lock, while the #PF handler can both acquire
	 * the mmap_read_lock or the per-VMA lock. If FAULT_FLAG_RETRY_NOWAIT is set then for sure the
	 * mmap_lock was taken (at least for our limited cases, that is when handle_mm_fault is called by
	 * the #PF handler or GUP).
	 *
	 * See: https://elixir.bootlin.com/linux/v7.1.4/source/include/linux/mm_types.h#L1751
	 */

	locked = !(retval & (VM_FAULT_RETRY | VM_FAULT_COMPLETED));

	if(retval & VM_FAULT_RETRY)
	 	/* 
	 	 * if we get here, then locked_mm = false, it may change if i
	 	 * FAULT_FLAG_RETRY_NOWAIT is set... (if set the mmap_lock is NOT DROPPED, locked = true)
	 	 */
		locked = vmf_flags & FAULT_FLAG_RETRY_NOWAIT;

	/*
	 * if the per-VMA lock is the one still acquired, use the VMA directly (skip the mmap_read_lock, vma_lookup later on, basically).
	 * if the mmap_lock is the one still acquired, skip the mmap_read_lock but still, use the mm actively.
	 * if nothing held at all (locked = false), do the regular path: mmap_read_lock and so on...
	 *
	 * support for FAULT_FLAG_REMOTE?? If REMOTE is set, then GUP is used, but GUP never acquires the per-VMA lock... 
	 *  this should never happen in this case.
	 */
	if(locked && likely(vmf_flags & FAULT_FLAG_VMA_LOCK))
		target_vma = vmf(vmfe)->vma;

	/*
	 * if FAULT_FLAG_REMOTE is enabled then target_mm != current->mm. Anyway, rlock it
	 * only if needed (that is, if !locked_mm is true) because paths that bring to
	 * handle_mm_fault (GUP and page fault handler) already to the mmap_read_lock on the
	 * target_mm, but within the function (handle_mm_fault) it may happen that the rlock 
	 * is released (so we need to rlock), see locked and comments above. 
	 *
	 * Never trylock.
	 */
	DEFINE_MMS_LOCK_CONTROL(mmslk, target_mm, target_vma, !locked, false);

	/* if VM_FAULT_RETRY is set we may still have the fault arounded ptes to inspect, 
	 * and if VM_FAULT_NOPAGE is NOT set, ptep is expected to be NOT NULL and VALID,
	 * and still have the fault arounded ptes to inspect 
	 */
	if(retval & VM_FAULT_RETRY || unlikely(!(retval & VM_FAULT_NOPAGE) && !ptep))
		goto __end;

	/* if VM_FAULT_NOPAGE is set, ptep is not valid!! */
	if(retval & VM_FAULT_NOPAGE) {
		ptep = THUNK(pte_offset_map_lock)(target_mm, vmf(vmfe)->pmd, vmf(vmfe)->address, &ptl);
		if(!ptep) {
			scid_err("unable to pte_offset_map_lock");
			goto __end;
		}
	} else {
		ptl = vmf(vmfe)->ptl;
		spin_lock(ptl);
	}

	/* changed my mind on this, acquired the ptl... */
	pte = ptep_get(ptep);

	if(retval & VM_FAULT_NOPAGE)
		pte_unmap_unlock(ptep, ptl);
	else
		spin_unlock(ptl);

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

		DEFINE_MPI(mpi, vmf(vmfe), ptl, ptep);
		DEFINE_SNAPSHOT_EXTRAS_WITH_PTR(snapex, 
				task_pid_nr(current), pfn, vmf(vmfe)->real_address);

		wrex_ptealtprot(pgs, vmf_flags, &mmslk, &mpi, snapex, kp);
	} else 
		goto __end;

__put_pgs_end:
	page_status_put(pgs);
__end:
	enforce_faulted_around_ptes(
			&fa_ptes_head(vmfe), vmf(vmfe)->address, 
			vmf(vmfe)->vma, &mmslk, kp);
__end_fail:

#endif /* DO_PTE_ALT_PROT */

	del_vmf(vmfe);
	return 0;
}

struct kretprobe handle_pte_fault__krp = {
	.entry_handler = handle_pte_fault__ehkrphook,
	.handler = handle_pte_fault__hkrphook,
	.kp.symbol_name = handle_pte_fault__symbol,
	.data_size = sizeof(struct vm_fault_entry*),
	.maxactive = KPS_MAXACTIVE,
};
