#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/mm.h>

#include <resolve_syms/pte_offset_map_lock.h>
#include <hooks/pte-page-track/utils/addpages.h>
#include <ptealtprot.h>
#include <pgtrack.h>
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

#ifdef DO_PTE_ALT_PROT

struct mm_to_pgs_args {
	struct mm_struct *mm;
	unsigned long addr;
	pte_t *ptep;
};

#define INIT_MM_TO_PGS_ARGS(pair, _mm, _addr, _ptep) \
	do { \
		(pair) = (struct mm_to_pgs_args) { \
			.mm = (_mm), \
			.addr = (_addr), \
			.ptep = (_ptep), \
		}; \
	} while(0)

#endif

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

#ifdef DO_PTE_ALT_PROT
	unsigned long nr_total_pages = 0;
	unsigned long nr_pages = 0;
	unsigned long cnt_addr = addr;
	struct mm_to_pgs_args *mtp_args;
	struct page_status *pgs;
#endif

	pte_t *ptep = THUNK(pte_offset_map_lock)(mm, pmd, addr, &ptl);
	if(!ptep) {
		scid_warn("NULL ptep");
		return 0;
	}

	struct pg_track_forward_args pgt_args = {
		.creat = false,
		.va = addr,
		.flags = 0,
	};

#ifdef DO_PTE_ALT_PROT
	do {
		nr_total_pages++;
	} while(cnt_addr += PAGE_SIZE, cnt_addr != end);

	mtp_args = kmalloc(sizeof(struct mm_to_pgs_args) * nr_total_pages, GFP_ATOMIC);
	if(!mtp_args) {
		scid_err("memory exhausted");
		return 0;
	}
#endif

	do {
		__maybe_unused bool rv;
		rv = add_one_page(ptep, NULL, NULL, NULL, &pgt_args);

		if(!rv) {
			/* see also add_one_page, this may happen frequently due to
		 	 * a PTE that is pte_none being passed anyway to change_pte_range
		 	 * for userfaultfd reasons, we simply ignore it
		 	 */

#ifdef __CPR_WARN_UNABLE_TO_ADD_PAGE
			scid_warn("unable to add page");
#endif

			continue;
		}

#ifdef DO_PTE_ALT_PROT
		INIT_MM_TO_PGS_ARGS(mtp_args[nr_pages++], mm, addr, ptep);
#endif

	} while(ptep++, addr += PAGE_SIZE, addr != end);

	pte_unmap_unlock(ptep, ptl);

	/* TODO move everything out and fixup code */

#ifdef DO_PTE_ALT_PROT
	rcu_read_lock();

	for(unsigned long i = 0; i < nr_pages; i++) {
		pgs = lookup_pfn_pgtrack(
				page_to_pfn(pte_page(ptep_get(mtp_args[i].ptep))));

		if(pgs) {
			spin_lock(&pgs->pg_mms.lock);
			add_mm_to_pgs_locked(pgs, mtp_args[i].mm, mtp_args[i].addr);

			if(atomic64_read(&pgs->perms) == PERM_BITS) {
				if(pgs->pg_mms.first_alt_handled) {
					zeroprot_ptes_locked(pgs);
					pgs->pg_mms.first_alt_handled = false;
				} /* else
					fixup_prot_for_pte_locked(pgs, mtp_args[i].ptep); */
			}

			spin_unlock(&pgs->pg_mms.lock);
		}
	}

	rcu_read_unlock();

	kfree(mtp_args);
#endif
	__testing("pages-ok");

	return 0;
}

struct kretprobe change_pte_range__krp = {
	.entry_handler = change_pte_range__ehkrphook,
	.handler = change_pte_range__hkrphook,
	.kp.symbol_name = change_pte_range__symbol,
	.data_size = sizeof(struct change_pte_range_args),
};
