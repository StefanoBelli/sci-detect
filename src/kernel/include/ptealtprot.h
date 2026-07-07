#ifndef SCID_PTEALTPROT_H
#define SCID_PTEALTPROT_H

#include <linux/mm_types.h>

/* fwd decl */
struct page_mms;
struct page_status;

/**
 * add_mm_to_pgs - manually add a (mm, addr) pair to mms of the page.
 *
 * May be useful if there's contention on a reverse mapping used lock.
 *
 * @pgs: the struct page_status having a non-NULL struct page_mms*
 * @mm: the mm descriptor
 * @addr: the associated virtual address
 */
void add_mm_to_pgs(struct page_status *pgs, struct mm_struct *mm, unsigned long addr);

/**
 * fixup_prot_for_pte - silently deny page protection bits upgrade, based
 * on current shadow perm for @pgs.
 *
 * @pgs: the pgs
 * @ptep: the ptep to fixup
 */
void fixup_prot_for_pte(struct page_status *pgs, pte_t *ptep);

/**
 * update_mm_addr - if mremap happens, this catches the virtual address change
 *
 * @pgs: the pgs
 * @mm: the mm
 * @old_addr: old addr to match for
 * @new_addr: new addr to substitute
 */
void update_mm_addr(struct page_status *pgs, struct mm_struct *mm, 
		unsigned long old_addr, unsigned long new_addr);

/**
 * new_page_mms_lock_pgs - create, init, lock and then publish mms for @pgs
 *
 * @pgs: the pgs to publish mms for
 */
void new_page_mms_lock_pgs(struct page_status *pgs);

/**
 * free_mms_from_pgs - free mms from pgs, basically, just utility function
 * to reduce the number of preprocessor directives around the code
 */
void free_mms_from_pgs(struct page_status *pgs);

/**
 * mms_lock - take the lock of a mms (pointed by @pgs)
 *
 * @pgs: the pgs pointing to mms
 */
void mms_lock(struct page_status *pgs);

/**
 * mms_unlock - release the lock of a mms (pointed by @pgs)
 *
 * @pgs: the pgs pointing to mms
 */
void mms_unlock(struct page_status *pgs);

/**
 * alternate_ptes_locked - alternate PTEs assuming mms lock taken by calling
 * thread
 *
 * @pgs: the pgs
 * @flags: the flags
 */
void alternate_ptes_locked(struct page_status *pgs, enum fault_flag flags);

/**
 * alternate_ptes - main routine used to alternate ptes' protection bits
 *
 * @pgs: the pgs having a non-NULL struct page_mms*
 * @flags: the fault_flag that comes along with struct vm_fault
 */
static inline void alternate_ptes(struct page_status *pgs, enum fault_flag flags)
{

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	mms_lock(pgs);
	alternate_ptes_locked(pgs, flags);
	mms_unlock(pgs);
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}

/**
 * setup_ptealtprot - setup PTE alternating protection mechanism
 */
int setup_ptealtprot(void);

/**
 * teardown_ptealtprot - teardown PTE alternating protection mechanism
 */
void teardown_ptealtprot(void);

#endif
