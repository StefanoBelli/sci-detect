#ifndef SCID_SFES_H
#define SCID_SFES_H

#include <ptealtprot.h>

#if defined(DO_PTE_ALT_PROT) || defined(SCID_CONFIG_TESTING)

#include <linux/slab.h>
#include <linux/sched.h>

#include <logging.h>
#include <kcps.h>

/* extras which will be read by force_sig_fault hook */
struct sig_fault_extras_entry {
	struct task_struct *tsk;  /* acts as key for kcps */

	unsigned long error_code; /* the x86 pushed-on-stack error code */
	unsigned long address;    /* the 'faulting' virtual address */
	int si_code;              /* most likely SEGV_MAPERR or SEGV_ACCERR */

	/* 
	 * these are read from the target 
	 * vm_area_struct when the page fault handler had
	 * the lock held (either the mmap_lock or per-VMA lock),
	 * immediately before releasing that lock.
	 * See __bad_area, and its hook in hooks/pte-page-track/fsf.c
	 */
	vm_flags_t orig_vm_flags;
};

#define tsk(entry) ((entry)->tsk)
#define error_code(entry) ((entry)->error_code)
#define address(entry) ((entry)->address)
#define si_code(entry) ((entry)->si_code)
#define orig_vm_flags(entry) ((entry)->orig_vm_flags)

static bool __sfe_kcp_comparator(struct kcp_entry *entry, u64 key)
{
	return key == (u64) tsk((struct sig_fault_extras_entry*) entry->data);
}

/**
 * got_this_sfe - got this sfe?
 *
 * @tsk: the task
 *
 * Returns: the sfee if we got it, NULL otherwise
 */
static inline struct sig_fault_extras_entry *got_this_sfe(
		struct task_struct *tsk)
{
	struct kcp_entry *kcpe = got_this_kcp((u64) tsk, __sfe_kcp_comparator);
	if(unlikely(!kcpe))
		return NULL;

	return kcpe->data;
}

/**
 * add_sfe - add the sfe
 *
 * @tsk: the task descriptor
 * @error_code: the error code
 * @address: the address
 * @si_code the si_code
 * @orig_vm_flags: the vm_flags
 *
 * Returns: the sfee
 */
static inline struct sig_fault_extras_entry* add_sfe(
		struct task_struct *tsk, unsigned long error_code,
		unsigned long address, int si_code, vm_flags_t orig_vm_flags)
{
	struct kcp_entry *kcpe;
	struct sig_fault_extras_entry *sfee;

	sfee = kmalloc(sizeof(struct sig_fault_extras_entry), GFP_ATOMIC);
	if(unlikely(!sfee)) {
		scid_err("memory exhausted");
		return NULL;
	}

	tsk(sfee) = tsk;
	error_code(sfee) = error_code;
	address(sfee) = address;
	si_code(sfee) = si_code;
	orig_vm_flags(sfee) = orig_vm_flags;

	kcpe = add_kcp((u64) tsk, sfee);
	if(unlikely(!kcpe)) {
		kfree(sfee);
		return NULL;
	}

	return kcpe->data;
}

/**
 * del_sfe - del this sfee
 *
 * @sfee: the sfee
 */
static inline void del_sfe(struct sig_fault_extras_entry *sfee)
{
	del_kcp_bykey((u64) sfee->tsk, __sfe_kcp_comparator);
	kfree(sfee);
}

#endif /* defined(DO_PTE_ALT_PROT) || defined(SCID_CONFIG_TESTING) */

#endif /* SCID_SFES_H */
