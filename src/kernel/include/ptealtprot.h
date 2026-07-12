#ifndef SCID_PTEALTPROT_H
#define SCID_PTEALTPROT_H

#if !defined(DISABLE_PAGE_SNAPSHOT) && !defined(DISABLE_PTE_ALT_PROT)
#	define DO_PTE_ALT_PROT 1
#endif

#include <linux/mm_types.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/spinlock.h>
#include <linux/types.h>

struct mms {
	struct mm_struct *mm;
	unsigned long addr;

	struct list_head node;
};

struct page_mms {
	spinlock_t lock;
	bool shadow_write;

	/* this must be used by cpr */
	bool first_alt_handled;

	struct list_head mms_head;
};

#endif /* DO_PTE_ALT_PROT */

/* fwd decl */
struct page_status;

/**
 * add_mm_to_pgs_locked - manually add a (mm, addr) pair to mms of the page.
 *
 * Caller must ensure the page_mms::lock is acquired.
 *
 * @pgs: the struct page_status having a non-NULL struct page_mms*
 * @mm: the mm descriptor
 * @addr: the associated virtual address
 */
void add_mm_to_pgs_locked(struct page_status *pgs, struct mm_struct *mm, unsigned long addr);

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
 * init_pg_mms - initialize pg_mms field of pgs
 *
 * @pgs: the pgs
 */
void init_pg_mms(struct page_status *pgs);

/**
 * free_mms_from_pgs - free mms from pgs, basically, just utility function
 * to reduce the number of preprocessor directives around the code
 */
void free_mms_from_pgs(struct page_status *pgs);

/**
 * alternate_ptes_locked - alternate PTEs assuming mms lock taken by calling
 * thread
 *
 * @pgs: the pgs
 * @vmf: the vmf
 */
void alternate_ptes_locked(struct page_status *pgs, struct vm_fault *vmf);

/**
 * zeroprot_ptes_locked
 *
 * @pgs: the pgs
 */
void zeroprot_ptes_locked(struct page_status *pgs);

/**
 * setup_ptealtprot - setup PTE alternating protection mechanism
 */
int setup_ptealtprot(void);

/**
 * teardown_ptealtprot - teardown PTE alternating protection mechanism
 */
void teardown_ptealtprot(void);

#endif
