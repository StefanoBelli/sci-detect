#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/kprobes.h>
#include <linux/mm.h>
#include <asm-generic/rwonce.h>
#include <uapi/linux/mman.h>
#include <uapi/asm-generic/mman-common.h>

#include <resolve_syms/page_vma_mapped_walk.h>
#include <hooks/pte-page-track/utils/obtain_user_page.h>
#include <kpsleepable.h>
#include <logging.h>
#include <pgtrack.h>

#define ksys_mmap_pgoff__symbol "ksys_mmap_pgoff"

static int ksys_mmap_pgoff__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	unsigned long prot = regs->dx;
	unsigned long flags = regs->cx;

	if(!(prot & (PROT_EXEC | PROT_WRITE)))
		return 1;

	if(flags == (MAP_ANONYMOUS | MAP_PRIVATE))
		return 1;

	if(flags == (MAP_ANONYMOUS | MAP_SHARED))
		return 1;

	return 0;
}

static inline struct page *do_oup(struct kprobe *kp, unsigned long addr)
{
	struct page *page;

	__enable_sleep(kp);
	page = obtain_user_page_from_addr(addr);
	__disable_sleep(kp);

	return page;
}

static inline bool do_none_ptealtprot(
		struct kprobe *kp, struct page_status *pgs)
{
	scid_info("altprot none");

	bool rv;

	__enable_sleep(kp);
	mutex_lock(&pgs->pap->lock);
	rv = none_locked_ptealtprot(pgs, NULL);
	mutex_unlock(&pgs->pap->lock);
	__disable_sleep(kp);

	return rv;
}

static void do_pte_fixup_ptealtprot(
		struct kprobe *kp, unsigned long addr, struct page_status *pgs)
{
	scid_info("altprot fixup");

	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	struct folio *folio;

	/* this "sleepable" section may be reduced? */
	__enable_sleep(kp);

	/* take the read lock */
	mmap_read_lock(mm);
	
	/* since this is the current->mm, no need to get and/or grab it*/
	vma = vma_lookup(mm, addr);
	if(!vma)
		goto __unlock;

	/* do the page table walk */
	folio = page_folio(pgs->page);
	DEFINE_FOLIO_VMA_WALK(pvmw, folio, vma, addr, 0);
	if(unlikely(!THUNK(page_vma_mapped_walk)(&pvmw))) 
		goto __unlock;

	/* consistency checks */
	if(likely(pvmw.pmd && pvmw.pte)) {
		if(likely(pvmw.ptl && spin_is_locked(pvmw.ptl))) {
			/* release the lock as requested */
			pte_unmap_unlock(pvmw.pte, pvmw.ptl);

			/* apply the alternating mechanism */
			mutex_lock(&pgs->pap->lock);
			pte_fixup_locked_ptealtprot(pvmw.pte, vma, pvmw.ptl, pgs, addr);
			mutex_unlock(&pgs->pap->lock);
		} else
			scid_err("got invalid spinlock from pvmw");
	} else
		scid_err("invalid pmd/pte from pvmw");

__unlock:
	/* release the read lock */
	mmap_read_unlock(mm);

	/* this "sleepable" section may be reduced? */
	__disable_sleep(kp);
}

struct kretprobe ksys_mmap_pgoff__krp;

static int ksys_mmap_pgoff__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct page *page;
	unsigned long addr = regs_return_value(regs);
	struct page_status *pgs;
	bool pgs_ok;
	struct kprobe *kp;

	if((long) addr < 0)
		return 0;

	kp = kpat(ksys_mmap_pgoff__krp, krpi);

	page = do_oup(kp, addr);
	if(!page)
		return 0;

	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(page_to_pfn(page));
	pgs_ok = pgs && try_page_status_get(pgs);
	rcu_read_unlock();

	if(unlikely(!pgs_ok))
		return 0;

	if(likely(!READ_ONCE(pgs->pap)))
		goto __pgs_put;

	if(pgs->pap->init && do_none_ptealtprot(kp, pgs))
		goto __pgs_put;

	do_pte_fixup_ptealtprot(kp, addr, pgs);

__pgs_put:
	page_status_put(pgs);

	return 0;
}

struct kretprobe ksys_mmap_pgoff__krp = {
	.entry_handler = ksys_mmap_pgoff__ehkrphook,
	.handler = ksys_mmap_pgoff__hkrphook,
	.kp.symbol_name = ksys_mmap_pgoff__symbol,
};

#endif /* DO_PTE_ALT_PROT */
