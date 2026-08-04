#ifndef SCID_PTEALTPROT_H
#define SCID_PTEALTPROT_H

#if !defined(DISABLE_PTE_ALT_PROT)
#	define DO_PTE_ALT_PROT 1
#endif /* !defined(DISABLE_PTE_ALT_PROT) */

#ifdef DO_PTE_ALT_PROT

#include <linux/mutex.h>

struct ptealtprot_struct {
	struct mutex lock;
	bool init : 1;
	bool write : 1;
	bool noprot : 1;
};

#endif /* DO_PTE_ALT_PROT */

#include <linux/mm_types.h>
#include <linux/spinlock.h>
#include <linux/kprobes.h>

#ifndef DISABLE_PAGE_SNASPHOT

struct snapshot_extras {
	pid_t pid;
	unsigned long pfn;
	unsigned long raddr;
};

#define DEFINE_SNAPSHOT_EXTRAS_WITH_PTR(__name, __pid, __pfn, __raddr) \
	struct snapshot_extras __##__name = { \
		.pid = (__pid), \
		.pfn = (__pfn), \
		.raddr = (__raddr), \
	}; \
	struct snapshot_extras *__name = &__##__name

#else
struct snapshot_extras;
#define DEFINE_SNAPSHOT_EXTRAS_WITH_PTR(__name, __pid, __pfn, __raddr) \
	struct snapshot_extras *__name = NULL
#endif

struct my_pte_info {
	pte_t *ptep;
	spinlock_t *ptlp;
	struct vm_area_struct *vma;
	unsigned long addr;
};

struct mms_lock_control {
	struct mm_struct *target_mm;        /* the mm we're operating on */
	struct vm_area_struct *target_vma;  /* if this is set (not null), then vma is locked
										 * via a per-vma lock. This VMA will be used 
										 * instead of the regular mm_struct's control path */
	bool rlock_target_mm;               /* acquire the read lock, for the target_mm ? */
	bool trylock;                       /* acquire ALL the mmap_locks via a trylock */
};

#define DEFINE_MMS_LOCK_CONTROL(__name, __target_mm, __target_vma, __rlock_target_mm, __trylock) \
	struct mms_lock_control __name = { \
		.target_mm = (__target_mm), \
		.target_vma = (__target_vma), \
		.rlock_target_mm = (__rlock_target_mm), \
		.trylock = (__trylock) \
	}

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
 * May sleep. Expecting process context.
 * Don't acquire the pap->lock
 *
 * Usage: page fault handler return handler
 *
 * @pgs: the pgs
 * @ff: the fault flags
 * @mmslk: the struct mms_lock_control
 * @mpi: fault handler's own manipulated PTE infos
 * @snapex: snapshot extra infos
 * @kp: the current kprobe
 */
void wrex_ptealtprot(
		struct page_status *pgs, enum fault_flag ff, 
		struct mms_lock_control *mmslk, struct my_pte_info *mpi, 
		struct snapshot_extras *snapex, struct kprobe *kp);

/**
 * exonly_ptealtprot - apply pte prot alternation, only
 * execute is a possible target
 * 
 * May sleep. Expecting process context.
 * Don't acquire the pap->lock.
 *
 * Usage: segmentation fault handler
 *
 * @pgs: the pgs
 * @mmslk: the struct mms_lock_control
 * @snapex: snapshot extra infos
 * @kp: the current kprobe
 */
void exonly_ptealtprot(
		struct page_status *pgs, struct mms_lock_control *mmslk,
		struct snapshot_extras *snapex, struct kprobe *kp);

/**
 * none_ptealtprot - apply pte prot alternation, all
 * PTEs will have both wr and ex disabled.
 * 
 * May sleep. Expecting process context.
 * Don't acquire the pap->lock.
 *
 * Before calling this, you may check pgs->pap->init optimistically:
 *  * if ->init is true then acquire the lock and recheck 
 *  * if ->init is false... don't do anything!
 *
 * Usage: first WX page-detection caused by mprotect, called when
 * a system call returns.
 *
 * @pgs: the pgs
 * @mmslk: the struct mms_lock_control
 * @snapex: snapshot extra infos
 * @kp: the current kprobe
 *
 * Returns: true if cleared all protection bits (->init was true), false otherwise
 */
bool none_ptealtprot(struct page_status *pgs, struct mms_lock_control *mmslk,
		struct snapshot_extras *snapex, struct kprobe *kp);

/**
 * pte_fixup_ptealtprot - adjust pte protection bits after
 * according to the current shadow perms.
 *
 * May sleep. Expecting process context.
 *
 * Caller must **NOT** acquire the page table lock of @ptep (subtle deadlock
 * warning due to different lock acquisition patterns)
 *
 * Caller must ensure VMA is properly locked (whether is per-VMA lock or whole mmap lock).
 *
 * This is meaningful only when ->init is false (that is, mprotect
 * after the page is detected as WX and alternation mechanism already
 * started).
 *
 * Usage: already detected WX-page mprotect assoc. pte.
 *
 * @pgs: the pgs
 * @mpi: my_pte_info
 * @kp: the current running kprobe
 *
 */
void pte_fixup_ptealtprot(
		struct page_status *pgs, struct my_pte_info *mpi, 
		struct kprobe *kp);

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
