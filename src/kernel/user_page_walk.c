#include <user_page_walk.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/pagewalk.h>
#include <linux/compiler.h>

#include <resolve_syms/walk_page_range.h>
#include <logging.h>
#include <kpsleepable.h>

#define ERROR_INVALID_PTE -123456

struct upw_args {
	pte_t **ptep;
	spinlock_t **ptlp;
	struct vm_area_struct **vma;
	struct page *page;
};

static int upw_entry_pte(
		pte_t *ptep, __always_unused unsigned long addr, 
		__always_unused unsigned long next, struct mm_walk *walk)
{
	struct upw_args *out = walk->private;

	/* pte is locked */
	pte_t pte = ptep_get(ptep);

	if(pte_none(pte) || !pte_present(pte))
		return ERROR_INVALID_PTE;

	out->page = pte_page(pte);

	if(out->ptep)
		*out->ptep = ptep;

	if(out->ptlp)
		*out->ptlp = ptep_lockptr(walk->mm, ptep);

	if(out->vma)
		*out->vma = walk->vma;

	return 0;
}

/* don't use GUP, as it will alter PTEs, cause faultins, ... */
struct page *user_page_walk_ptep_vma(
		unsigned long addr, bool quiet, bool rlkmm, 
		pte_t **ptep, spinlock_t **ptlp, struct vm_area_struct **vma,
		struct kprobe *kp)
{
	int rv;
	struct mm_struct *mm = current->mm;
	unsigned long astart = addr & PAGE_MASK;
	unsigned long aend = astart + PAGE_SIZE;
	struct mm_walk_ops wops = {
		.pte_entry = upw_entry_pte,
		.walk_lock = PGWALK_RDLOCK,
	};
	struct upw_args walk_privargs = {
		.ptep = ptep,
		.ptlp = ptlp,
		.vma = vma,
	};

	/* lock mm */
	if(rlkmm) {
		if(kp)
			KPSLEEPABLE(kp,
				mmap_read_lock(mm);
			);
		else
			mmap_read_lock(mm);
	}

	rv = THUNK(walk_page_range)(mm, astart, aend, &wops, &walk_privargs);

	/* unlock mm */
	if(rlkmm)
		mmap_read_unlock(mm);

	if(rv && !quiet)
		scid_errf("walk_page_range failed with rv=%d", rv);

	return walk_privargs.page;
}

#endif /* DO_PTE_ALT_PROT */
