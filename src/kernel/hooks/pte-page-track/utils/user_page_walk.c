#include <hooks/pte-page-track/utils/user_page_walk.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/pagewalk.h>
#include <linux/mm.h>
#include <linux/compiler.h>

#include <resolve_syms/walk_page_range.h>
#include <logging.h>
#include <kpsleepable.h>

#define ERROR_INVALID_PTE -123456

static int upw_entry_pte(
		pte_t *ptep, __always_unused unsigned long addr, 
		__always_unused unsigned long next, struct mm_walk *walk)
{
	struct page **out_page = walk->private;
	pte_t pte = ptep_get(ptep);

	if(pte_none(pte) || !pte_present(pte))
		return ERROR_INVALID_PTE;

	*out_page = pte_page(pte);

	return 0;
}

/* don't use GUP, as it will alter PTEs, cause faultins, ... */
struct page *user_page_walk(unsigned long addr, bool quiet, struct kprobe *kp)
{
	int rv;
	struct mm_struct *mm = current->mm;
	unsigned long astart = addr & PAGE_MASK;
	unsigned long aend = astart + PAGE_SIZE;
	struct page *page = NULL;
	struct mm_walk_ops wops = {
		.pte_entry = upw_entry_pte,
		.walk_lock = PGWALK_RDLOCK,
	};

	/* lock mm */
	KPSLEEPABLE(kp,
			mmap_read_lock(mm);
	);

	rv = THUNK(walk_page_range)(mm, astart, aend, &wops, &page);

	/* unlock mm */
	mmap_read_unlock(mm);

	if(rv && !quiet)
		scid_errf("walk_page_range failed with rv=%d", rv);

	return page;
}

#endif /* DO_PTE_ALT_PROT */
