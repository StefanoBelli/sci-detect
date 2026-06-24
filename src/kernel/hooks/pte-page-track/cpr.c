#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/mm.h>

#include <resolve_syms/pte_offset_map_lock.h>
#include <hooks/pte-page-track/utils/addpages.h>

#include <logging.h>
#include <testing/testing.h>

#define MY_TESTING_SUBSYS_NAME "pte-page-track-cpr-hook"

struct change_pte_range_args {
	/* the vma */
	struct vm_area_struct *vma;

	/* the pmd entry which points to pte */
	pmd_t *pmd;

	/* the starting virtual address of the range */
	unsigned long addr;

	/* the ending virtual address of the range */
	unsigned long end;

	/* new protection bits */
	/* pgprot_t newprot; */

	/* change protection flags */
	/* unsigned long cp_flags; */
};

#define change_pte_range__symbol "change_pte_range"

static int change_pte_range__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	__testing("entry");

	struct change_pte_range_args *cpr_args = (struct change_pte_range_args*) krpi->data;
	unsigned long cp_flags = *((unsigned long*) regs->sp + 1);

	cpr_args->vma = (struct vm_area_struct *) regs->si;
	cpr_args->pmd = (pmd_t*) regs->dx;
	cpr_args->addr = regs->cx;
	cpr_args->end = regs->r8;
	/* cpr_args->newprot.pgprot = (pgprotval_t) regs->r9; */
	/* cpr_args->cp_flags = cp_flags; */

	bool invalid =
		cp_flags & MM_CP_PROT_NUMA ||
		cp_flags & MM_CP_UFFD_WP ||
		cp_flags & MM_CP_UFFD_WP_ALL ||
		cp_flags & MM_CP_UFFD_WP_RESOLVE;

	if(invalid) {
		scid_warnf("unhandled cp_flags: %ld", cp_flags);
		return 1;
	}

	return 0;
}

static int change_pte_range__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct change_pte_range_args *cpr_args;

	/* returns the number of affected pages, 0 if none, < 0 on failure */
	unsigned long rrv = regs_return_value(regs);
	if(!rrv || rrv < 0)
		return 0;

	__testing("return-ok");

	cpr_args = (struct change_pte_range_args*) krpi->data;

	struct mm_struct *mm = cpr_args->vma->vm_mm;
	pmd_t *pmd = cpr_args->pmd;
	unsigned long addr = cpr_args->addr;
	unsigned long end = cpr_args->end;
	spinlock_t *ptl;

	pte_t *ptep = THUNK(pte_offset_map_lock)(mm, pmd, addr, &ptl);
	if(!ptep) {
		scid_warn("NULL ptep");
		return 0;
	}

	struct pg_track_forward_args pgt_args = {
		.creat = false,
		.va = addr,
	};

	do {
		__maybe_unused bool rv;
		rv = add_one_page(ptep, NULL, NULL, NULL, &pgt_args);

		/* see also add_one_page, this may happen frequently due to
		 * a PTE that is pte_none being passed anyway to change_pte_range
		 * for userfaultfd reasons, we simply ignore it
		 */
#ifdef __CPR_WARN_UNABLE_TO_ADD_PAGE
		if(!rv)
			scid_warn("unable to add page");
#endif

	} while(ptep++, addr += PAGE_SIZE, addr != end);

	pte_unmap_unlock(ptep, ptl);

	__testing("pages-ok");

	return 0;
}

struct kretprobe change_pte_range__krp = {
	.entry_handler = change_pte_range__ehkrphook,
	.handler = change_pte_range__hkrphook,
	.kp.symbol_name = change_pte_range__symbol,
	.data_size = sizeof(struct change_pte_range_args),
};
