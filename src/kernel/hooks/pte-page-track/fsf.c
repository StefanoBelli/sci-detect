#include <ptealtprot.h>
#include <testing/testing.h>
#define MY_TESTING_SUBSYS_NAME "pte-page-track-fsf-hook"

#if defined(DO_PTE_ALT_PROT) || defined(SCID_CONFIG_TESTING)

#ifdef DO_PTE_ALT_PROT

#include <linux/kprobes.h>
#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/rcupdate.h>
#include <asm/nospec-branch.h>
#include <asm-generic/rwonce.h>

#include <hooks/pte-page-track/utils/user_page_walk.h>
#include <pgtrack.h>
#include <logging.h>

#define WITH_NEW_IP 1

#endif /* DO_PTE_ALT_PROT */

#include <asm/trap_pf.h>
#include <sfes.h>

#define WITH_ORIG_IP 0

#define REQUIRED_CPU_ERROR_CODE ( \
		X86_PF_PROT | \
		X86_PF_USER | \
		X86_PF_INSTR)

static bool fsf_checks_ok(struct pt_regs *regs, void __user **_addr)
{
	int sig = regs->di;
	int code;
	void __user* addr;
	struct sig_fault_extras_entry *sfee;

	/* signal being delivered */
	if(unlikely(sig != SIGSEGV))
		return false;

	code = regs->si;

	/* type of segment violation */
	if(code != SEGV_ACCERR)
		return false;

	addr = (void __user*) regs->dx;

	/* is this the right kernel control path? */
	sfee = got_this_sfe(current);
	if(unlikely(!sfee))
		return false;

	/* this is a consistency check */
	if(address(sfee) != (unsigned long) addr || si_code(sfee) != code) {
		scid_warn("got different sig fault extras, from validated ones");
		return false;
	}

	/* now check the error code */
	if(error_code(sfee) != REQUIRED_CPU_ERROR_CODE)
		return false;

	/* 
	 * check the orig_vm_flags, that is, the ones that we read
	 * before dropping the mmap_lock or the per-VMA lock in __bad_area
	 *
	 * this is to emulate classical page fault handler behaviour with 
	 * writes: access_error detects that it is a write, 
	 * but the VMA has VM_WRITE cleared? Segfault. Don't go any further.
	 *
	 * Similarly, we don't want to do anything if the VMA has *not* 
	 * VM_EXEC enabled. 
	 */
	if(!(orig_vm_flags(sfee) & VM_EXEC))
		return false;

	/* everything is ok */
	*_addr = addr;
	return true;
}

#ifdef DO_PTE_ALT_PROT

#define __dont_optimize_no_frame_pointer \
	__attribute__((__optimize__("-fomit-frame-pointer,-O0")))

static noinline __dont_optimize_no_frame_pointer 
void __return_from_subroutine(void)
{
	return;
}

struct kprobe force_sig_fault__kp;

#endif /* DO_PTE_ALT_PROT */

#define force_sig_fault__symbol "force_sig_fault"

static int force_sig_fault__phkphook(
		__always_unused struct kprobe *kp, struct pt_regs *regs)
{
	void __user* addr;
	int rv = WITH_ORIG_IP;

#ifdef DO_PTE_ALT_PROT
	struct page *page;
	unsigned long pfn;
	struct page_status *pgs;
	bool pgs_success;
#endif /* DO_PTE_ALT_PROT */

	__testing("entry");

	if(!fsf_checks_ok(regs, &addr))
		return rv;

	__testing("checks-ok");

#ifdef DO_PTE_ALT_PROT

	page = user_page_walk((unsigned long) addr, false, true, &force_sig_fault__kp);
	if(!page)
		return rv;

	pfn = page_to_pfn(page);

	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(pfn);
	pgs_success = pgs && try_page_status_get(pgs);
	rcu_read_unlock();

	if(likely(pgs_success)) {
		if(likely(!READ_ONCE(pgs->pap)))
			goto __put_pgs;

		DEFINE_SNAPSHOT_EXTRAS_WITH_PTR(snapex, 
				task_pid_nr(current), pfn, (unsigned long) addr);

		/*
		 * segmentation faults are delivered (run when coming back to userspace)
		 * in the page fault handler, which is called synchronously wrt the user
		 * control path, so, the target_mm is always current->mm. Always do
		 * mmap_read_lock since either the per-VMA lock or the whole mmap lock
		 * is released earlier (by this kernel control path, see __bad_area:
		 * https://elixir.bootlin.com/linux/v6.15/source/arch/x86/mm/fault.c#L837)
		 *
		 * If __bad_area_nosemaphore is called by the wrapper bad_area_nosemaphore,
		 * with si_code=SEGV_MAPERR, the mmap_read_lock is not held as well: that's
		 * because either bad_area_nosemaphore is called very early, so, no mmap_lock
		 * acquired, or because the corresponding VMA could not be found in the mm
		 * of current and auxiliary function lock_mm_and_find_vma releases the lock
		 * before returning NULL. 
		 * See: https://elixir.bootlin.com/linux/v6.15/source/arch/x86/mm/fault.c#L1360
		 *
		 * Anyway, we don't actually care about the SEGV_MAPERR case.
		 *
		 * No trylocks.
		 * We are sure that target_vma = NULL because no per-VMA lock is held.
		 */
		DEFINE_MMS_LOCK_CONTROL(mmslk, current->mm, NULL, true, false);

		/* 
		 * here, we always acquire the mmap read lock for current->mm.
		 * As when force_sig_fault gets called, this lock or the per-VMA lock
		 * has been released.
		 */
		exonly_ptealtprot(pgs, &mmslk, snapex, &force_sig_fault__kp);

		rv = WITH_NEW_IP;
		regs->ip = (unsigned long) &__return_from_subroutine;
		goto __put_pgs;
	}
	
	return rv;

__put_pgs:
	page_status_put(pgs);

#endif /* DO_PTE_ALT_PROT */

	return rv;
}

struct kprobe force_sig_fault__kp = {
	.symbol_name = force_sig_fault__symbol,
	.pre_handler = force_sig_fault__phkphook,
};

/* 
 * we need to hook into __bad_area 
 * to be able to properly distinguish the situation.
 * No other way since the signal's code is either
 * SEGV_MAPERR or SEGV_ACCERR (write or exec, who knows?)
 *
 * But we need more infos (that is, the x86 arch-specific
 * error code).
 *
 * The entry handler of __bad_area will read the vma->vm_flags.
 * Why is that? Because we will pass those to force_sig_fault to
 * take decisions (see fsf_checks_ok above).
 * To ensure *some* consistency, we read the vm_flags here because when
 * __bad_area gets called thread still has either the "originally" acquired
 * mmap_lock or the per-VMA lock, by the arch-specific #PF handler.
 *
 * To see why this is ok, check out when bad_area_access_error gets called:
 *
 * --> https://elixir.bootlin.com/linux/v7.1.4/source/arch/x86/mm/fault.c#L1330
 * --> https://elixir.bootlin.com/linux/v7.1.4/source/arch/x86/mm/fault.c#L1368
 *
 * In the first one, the per-VMA lock is acquired (very likely this is to happen),
 * while in the second one, the PF handler decides to do fallback to the whole
 * mmap_lock. The last two parameters allow __bad_area to decide which
 * lock is actually taken of the two mentioned above, so release it.
 *
 *  * bad_area_access_error(..., NULL, vma) => the per-VMA lock is taken
 *  * bad_area_access_error(..., mm, vma)   => the mmap_lock is taken
 *
 * Note that in either case vma is NOT NULL! In the second case however, kernel
 * acquires the mmap_lock and then does some kind of vma lookup (so, vma not NULL).
 *
 * If VMA happens to be NULL, it is a SEGV_MAPERR (no valid mapping established for the
 * virtual address), and it is handled by bad_area_nosemaphore ("nosemaphore" means no
 * mmap_lock or no-per-VMA-lock to release, because we couldn't acquire them in the
 * first place, since no valid mapping is out there), but this is another story.
 */

#define __bad_area__symbol "__bad_area"

static int __bad_area__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	unsigned long error_code = regs->si;
	unsigned long address = regs->dx;
	struct vm_area_struct *vma = (struct vm_area_struct *) regs->r8;
	int si_code = *((unsigned long*) regs->sp + 1);
	struct task_struct *tsk = current;
	struct sig_fault_extras_entry *sfee;
	vm_flags_t vmflgs = vma->vm_flags; /* reading orig_vm_flags, its ok, see above */

	sfee = add_sfe(tsk, error_code, address, si_code, vmflgs);
	if(unlikely(!sfee)) {
		scid_err("add_sfe failed");
		return 1;
	}

	*((struct sig_fault_extras_entry**) krpi->data) = sfee;
	return 0;
}

static int __bad_area__hkrphook(
		struct kretprobe_instance *krpi, __always_unused struct pt_regs *regs)
{
	del_sfe(*((struct sig_fault_extras_entry**) krpi->data));
	return 0;
}

struct kretprobe __bad_area__krp = {
	.kp.symbol_name = __bad_area__symbol,
	.entry_handler = __bad_area__ehkrphook,
	.handler = __bad_area__hkrphook,
	.data_size = sizeof(struct sig_fault_extras_entry*),
};

#endif /* defined(DO_PTE_ALT_PROT) || defined(SCID_CONFIG_TESTING) */
