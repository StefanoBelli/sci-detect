#ifndef SCID_SFES_H
#define SCID_SFES_H

#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/slab.h>
#include <linux/sched.h>

#include <logging.h>
#include <kcps.h>

struct sig_fault_extras_entry {
	struct task_struct *tsk;

	unsigned long error_code;
	unsigned long address;
	int si_code;
};

#define tsk(entry) ((entry)->tsk)
#define error_code(entry) ((entry)->error_code)
#define address(entry) ((entry)->address)
#define si_code(entry) ((entry)->si_code)

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
 *
 * Returns: the sfee
 */
static inline struct sig_fault_extras_entry* add_sfe(
		struct task_struct *tsk, unsigned long error_code,
		unsigned long address, int si_code)
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

#endif /* DO_PTE_ALT_PROT */

#endif /* SCID_SFES_H */
