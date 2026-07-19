#ifndef SCID_PTEALTPROT_H
#define SCID_PTEALTPROT_H

#if !defined(DISABLE_PAGE_SNAPSHOT) && !defined(DISABLE_PTE_ALT_PROT)
#	define DO_PTE_ALT_PROT 1
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) && !defined(DISABLE_PTE_ALT_PROT) */

#ifdef DO_PTE_ALT_PROT

#include <linux/mm_types.h>
#include <linux/mutex.h>

struct ptealtprot_struct {
	struct mutex lock;
	bool init : 1;
	bool write : 1;
	bool noprot : 1;
};

#endif /* DO_PTE_ALT_PROT */

/* fwd decl */
struct page_status;

/**
 * new_ptealtprot - alloc and init ptealtprot_struct
 *
 * @pgs: the pgs
 */
void new_ptealtprot(struct page_status *pgs);

/**
 * free_ptealtprot - free the ptealtprot_struct
 *
 * @pgs: the pgs
 */
void free_ptealtprot(struct page_status *pgs);

/**
 * wrex_ptealtprot - apply pte prot alternation, both
 * write and execute are possible targets
 * 
 * May sleep.
 *
 * Usage: page fault handler return handler
 *
 * @pgs: the pgs
 * @ff: the fault flags
 * @skip_lock_this_mm: don't acquire the mmap_read_lock for this mm
 */
void wrex_ptealtprot(
		struct page_status *pgs, enum fault_flag ff, 
		struct mm_struct *skip_lock_this_mm,
		struct vm_area_struct *vma, pte_t* ptep,
		spinlock_t *ptlp, unsigned long addr);

/**
 * exonly_ptealtprot - apply pte prot alternation, only
 * execute is a possible target
 * 
 * May sleep.
 *
 * Usage: segmentation fault handler
 *
 * @pgs: the pgs
 * @skip_lock_this_mm: don't acquire the mmap_read_lock for this mm
 */
void exonly_ptealtprot(
		struct page_status *pgs, struct mm_struct *skip_lock_this_mm);

/**
 * none_ptealtprot - apply pte prot alternation, all
 * PTEs will have both wr and ex disabled.
 * 
 * May sleep.
 *
 * Before calling this, you may check pgs->pap->init optimistically:
 *  * if ->init is true then acquire the lock and recheck 
 *  * if ->init is false... don't do anything!
 *
 * Usage: first WX page-detection caused by mprotect, called when
 * a system call returns.
 *
 * @pgs: the pgs
 * @skip_lock_this_mm: don't acquire the mmap_read_lock for this mm
 *
 * Returns: true if cleared all protection bits (->init was true), false otherwise
 */
bool none_ptealtprot(
		struct page_status *pgs, struct mm_struct *skip_lock_this_mm);

/**
 * pte_fixup_ptealtprot - adjust pte protection bits after
 * according to the current shadow perms.
 *
 * May sleep.
 *
 * Caller must **NOT** acquire the page table lock of @ptep (subtle deadlock
 * warning due to different lock acquisition patterns)
 *
 * Caller must ensure VMA is properly locked (whether is per-VMA lock or whole mmap lock).
 *
 * This is meaningful only when ->init is false (that is, mprotect
 * after the page is detected as WX and alternation mechanism already
 * started). Contrary to none_ptealtprot:
 *
 * Usage: already detected WX-page mprotect assoc. pte.
 *
 * @ptep: the ptep to adjust
 * @vma: the vma
 * @ptlp: the ptr to ptl of @ptep
 * @pgs: the pgs
 * @addr: the va
 */
void pte_fixup_ptealtprot(
		pte_t* ptep, struct vm_area_struct *vma, spinlock_t *ptlp, 
		struct page_status *pgs, unsigned long addr);

/**
 * setup_ptealtprot - prepare it
 *
 * Returns: 0 if ok, not 0 otherwise
 */
int setup_ptealtprot(void);

/**
 * teardown_ptealtprot - teardown
 */
void teardown_ptealtprot(void);

#endif
