#ifndef SCID_PTEALTPROT_H
#define SCID_PTEALTPROT_H

#if !defined(DISABLE_PAGE_SNAPSHOT) && !defined(DISABLE_PTE_ALT_PROT)
#	define DO_PTE_ALT_PROT 1
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) && !defined(DISABLE_PTE_ALT_PROT) */

#ifdef DO_PTE_ALT_PROT

#include <linux/mutex.h>

struct ptealtprot_struct {
	struct mutex lock;
	bool init : 1;
	bool write : 1;
};

#endif /* DO_PTE_ALT_PROT */

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
 * wrex_locked_ptealtprot - apply pte prot alternation, both
 * write and execute are possible targets
 * 
 * Caller must acquire the lock first, and release it later.
 * May sleep. Use kpsleepable.
 *
 * Usage: page fault handler return handler
 *
 * @pap: the ptealtprot_struct
 */
void wrex_locked_ptealtprot(struct ptealtprot_struct *pap);

/**
 * exonly_locked_ptealtprot - apply pte prot alternation, only
 * execute is a possible target
 * 
 * Caller must acquire the lock first, and release it later.
 * May sleep. Use kpsleepable.
 *
 * Usage: segmentation fault handler
 *
 * @pap: the ptealtprot_struct
 */
void exonly_locked_ptealtprot(struct ptealtprot_struct *pap);

/**
 * none_locked_ptealtprot - apply pte prot alternation, all
 * PTEs will have both wr and ex disabled.
 * 
 * Caller must acquire the lock first, and release it later.
 * May sleep. Use kpsleepable.
 *
 * Usage: first WX page-detection caused by mprotect
 *
 * @pap: the ptealtprot_struct
 */
void none_locked_ptealtprot(struct ptealtprot_struct *pap);

/**
 * pte_fixup_locked_ptealtprot - adjust pte protection bits after
 * according to the current shadow perms.
 *
 * Caller must acquire the lock first, and release it later.
 * May sleep. Use kpsleepable.
 *
 * Usage: already detected WX-page mprotect assoc. pte.
 *
 * @ptep: the ptep to adjust
 * @pap: the ptealtprot_struct
 */
void pte_fixup_locked_ptealtprot(pte_t* ptep, struct ptealtprot_struct *pap);

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
