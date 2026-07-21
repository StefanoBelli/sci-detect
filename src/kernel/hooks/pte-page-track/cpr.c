#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <asm/pgtable_types.h>

#include <resolve_syms/pte_offset_map_lock.h>
#include <hooks/pte-page-track/utils/addpages.h>
#include <kpsleepable.h>
#include <testing/testing.h>
#include <logging.h>
#include <ptealtprot.h>

#define MY_TESTING_SUBSYS_NAME "pte-page-track-cpr-hook"

#define INVALID_CP_FLAGS(cpfl) \
	((cpfl) & MM_CP_PROT_NUMA || \
	 (cpfl) & MM_CP_UFFD_WP || \
	 (cpfl) & MM_CP_UFFD_WP_ALL || \
	 (cpfl) & MM_CP_UFFD_WP_RESOLVE)

#define CHECK_UNHANDLED_CP_FLAGS_RETURN(cpfl) \
	do { \
		if(unlikely(INVALID_CP_FLAGS((cpfl)))) { \
			scid_warnf("unhandled cp_flags: %ld", (cpfl)); \
			return 1; \
		} \
	} while(0)

#define CHECK_NONHARMFUL_PGPROT_RETURN(pgp) \
	do { \
		pgprotval_t val = pgprot_val((pgp)); \
		if((val & __NX) && !(val & __RW)) \
			return 1; \
	} while(0)

/* TODO optimize for pgprot */

struct change_pte_range_args {
	struct vm_area_struct *vma;
	pmd_t *pmd;
	unsigned long addr;
	unsigned long end;
};

#define change_pte_range__symbol "change_pte_range"

static int change_pte_range__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	__testing("entry");

	struct change_pte_range_args *cpr_args;
	unsigned long cp_flags = *((unsigned long*) regs->sp + 1);
	pgprot_t newprot;

	CHECK_UNHANDLED_CP_FLAGS_RETURN(cp_flags);

	newprot = __pgprot((pgprotval_t) regs->r8);
	CHECK_NONHARMFUL_PGPROT_RETURN(newprot);

	cpr_args = (struct change_pte_range_args*) krpi->data;

	cpr_args->vma = (struct vm_area_struct *) regs->si;
	cpr_args->pmd = (pmd_t*) regs->dx;
	cpr_args->addr = regs->cx;
	cpr_args->end = regs->r8;

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

	/* start of the critical section involving the ptl declared above */
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

	/* end of the critical section involving the ptl declared above */
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

#ifdef DO_PTE_ALT_PROT

#define change_protection_range__symbol "change_protection_range"

struct change_protection_range_args {
	struct vm_area_struct *vma;
	unsigned long start;
	unsigned long end;
};

static int change_protection_range__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct change_protection_range_args *cpr_args;
	unsigned long cp_flags = regs->r9;
	pgprot_t newprot;

	CHECK_UNHANDLED_CP_FLAGS_RETURN(cp_flags);

	newprot = __pgprot((pgprotval_t) regs->r8);
	CHECK_NONHARMFUL_PGPROT_RETURN(newprot);

	cpr_args = (struct change_protection_range_args *) krpi->data;

	cpr_args->vma = (struct vm_area_struct *) regs->si;
	cpr_args->start = regs->dx;
	cpr_args->end = regs->cx;

	return 0;
}

static void __do_ptealtprot(
		unsigned long addr, struct vm_area_struct *vma, struct kprobe *kp)
{

}

struct kretprobe change_protection_range__krp;

static int change_protection_range__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct change_protection_range_args *cpr_args;
	unsigned long rrv;
	unsigned long astart;
	unsigned long aend;
	struct vm_area_struct *vma;
	struct kprobe *kp;

	/* returns the number of affected pages, 0 if none, < 0 on failure */
	rrv = regs_return_value(regs);
	if(!rrv || rrv < 0)
		return 0;

	cpr_args = (struct change_protection_range_args*) krpi->data;

	astart = cpr_args->start;
	aend = cpr_args->end;
	vma = cpr_args->vma;
	kp = kpat(change_protection_range__krp, krpi);

	for(; astart != aend; astart += PAGE_SIZE)
		__do_ptealtprot(astart, vma, kp);

	return 0;
}

struct kretprobe change_protection_range__krp = {
	.entry_handler = change_protection_range__ehkrphook,
	.handler = change_protection_range__hkrphook,
	.kp.symbol_name = change_protection_range__symbol,
	.data_size = sizeof(struct change_protection_range_args),
};

#endif /* DO_PTE_ALT_PROT */
