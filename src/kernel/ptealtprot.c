#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include <resolve_syms/flush_tlb_mm_range.h>
#include <resolve_syms/page_vma_mapped_walk.h>
#include <pgtrack.h>
#include <logging.h>

static void __scid_flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
{
	scid_info("flush tlb placeholder");
	THUNK(flush_tlb_mm_range)(vma->vm_mm, addr, addr + PAGE_SIZE, PAGE_SHIFT, false);
}

static struct kmem_cache *mms_cachep;

struct drop_mm_work_args {
	struct mm_struct *mm;
	struct work_struct work;
};

static void __drop_mm_work(struct work_struct *work)
{
	struct drop_mm_work_args *args = container_of(work, struct drop_mm_work_args, work);
	mmdrop(args->mm);

	kfree(args);
}

static inline void __drop_del_free_mms(struct mms *entry)
{
	struct drop_mm_work_args *work_args;
	work_args = kmalloc(sizeof(struct drop_mm_work_args), GFP_ATOMIC);
	if(unlikely(!work_args)) {
		scid_warn("fallback to non-work-deferring offloading mmdrop");
		mmdrop(entry->mm);
	} else {
		work_args->mm = entry->mm;
		INIT_WORK(&work_args->work, __drop_mm_work);
		schedule_work(&work_args->work);
	}

	list_del(&entry->node);
	kmem_cache_free(mms_cachep, entry);
}

static void __del_all_mms(struct list_head *mms_head)
{
	struct mms *entry;
	struct mms *tmp;

	list_for_each_entry_safe(entry, tmp, mms_head, node)
		__drop_del_free_mms(entry);
}

/* to successfully "grab the mm", call this when sure that it will not go away... */
static bool __mms_grab_mm_and_publish_locked(
		struct mm_struct *mm, unsigned long addr, struct list_head *mms_head)
{
	/* allocate new mms descriptor */
	struct mms *mms = kmem_cache_alloc(mms_cachep, GFP_ATOMIC);
	if(unlikely(!mms)) {
		scid_err("memory exhausted");
		return false;
	}

	/* init and publish it */
	mms->mm = mm;
	mms->addr = addr;
	list_add(&mms->node, mms_head);

	/* grab the mm to avoid freeing it from under us */
	mmgrab(mms->mm);

	return true;
}

static void __do_alternate_ptes(struct folio *folio, struct page_mms *pg_mms)
{
	struct mms *entry;
	struct mms *tmp;

	list_for_each_entry_safe(entry, tmp, &pg_mms->mms_head, node) {
		/* the whole address space is not valid anymore */
		if(!mmget_not_zero(entry->mm)) {
			scid_info("mmgget");
			goto __failure_drop_delete;
		}

		/* minor failure: unable to take the read lock */
		if(!mmap_read_trylock(entry->mm)) {
			scid_info("mmlock");
			goto __skip_put;
		}

		struct vm_area_struct *vma;
		vma = vma_lookup(entry->mm, entry->addr);

		/* no such vma exists that contains 'addr' */
		if(!vma) {
			scid_info("vma");
			goto __failure_unlock_put_drop_delete;
		}

		DEFINE_FOLIO_VMA_WALK(pvmw, folio, vma, entry->addr, 0);
		bool map_ok = THUNK(page_vma_mapped_walk)(&pvmw);

		/* for some reason, unable to get the PTE */
		if(!map_ok) {
			scid_info("map");
			goto __failure_unlock_put_drop_delete;
		}

		if(pvmw.pmd && pvmw.pte) {
			if(pvmw.ptl && spin_is_locked(pvmw.ptl)) {
				scid_info("alternating");

				/* TODO validate VMA flags */

				pte_t pte = ptep_get(pvmw.pte);
				scid_infof("before: 0x%lx", pte_val(pte));
				
				pte = pte_wrprotect(pte);

				if(pg_mms->shadow_write) {
					scid_info("shadow_write!");
					pte = pte_set_flags(pte, _PAGE_NX);
				} else {
					scid_info("shadow exec!");
					pte = pte_mkexec(pte);
				}

				set_pte(pvmw.pte, pte);
				__scid_flush_tlb_page(vma, entry->addr);

				scid_infof("addr = 0x%lx\n", entry->addr);
				pte_unmap_unlock(pvmw.pte, pvmw.ptl);
			} else
				scid_warn("expecting the ptl lock");
		} else 
			scid_warn("pvmw returned invalid config");

		/* before next iteration... */
		mmap_read_unlock(entry->mm);
__skip_put:
		mmput_async(entry->mm);
		continue;

		/* failure code */
__failure_unlock_put_drop_delete:
		mmap_read_unlock(entry->mm);
		mmput_async(entry->mm);
__failure_drop_delete:
		__drop_del_free_mms(entry);
	}
}

/* mms lock is held */
static void __prot_adjust_exec(struct vm_fault *vmf, bool shadow_is_exec)
{
	pte_t pte;

	if(!shadow_is_exec) {
		return;
	}

	/* hold the page table lock */
	spin_lock(vmf->ptl);

	pte = ptep_get(vmf->pte);
	
	/* this is unlikely as this is being run AFTER the 
	 * page fault handler who setupped the PTEs */
	if(unlikely(!pte_present(pte) || pte_none(pte)))
		goto __unlock;

	/* this is run when the page fault handler is 
	 * executed, and when the #PF handler is executed,
	 * we either have the per-VMA lock or the whole
	 * mmap read lock (see FAULT_FLAG_VMA_LOCK). 
	 *
	 * * lock_vma_under_rcu: https://elixir.bootlin.com/linux/v7.1.2/source/arch/x86/mm/fault.c#L1325
	 *  \----> handle_mm_fault(... FAULT_FLAG_VMA_LOCK ...): https://elixir.bootlin.com/linux/v7.1.2/source/arch/x86/mm/fault.c#L1334
	 *
	 * OR
	 *
	 * * lock_mm_and_find_vma: https://elixir.bootlin.com/linux/v7.1.2/source/arch/x86/mm/fault.c#L1357
	 *  \----> handle_mm_fault(...): https://elixir.bootlin.com/linux/v7.1.2/source/arch/x86/mm/fault.c#L1385
	 *
	 * This is about this VMA and this PTE, not "remote" ones.
	 */
	if(vmf->vma->vm_flags & VM_EXEC) {
		//set_pte(vmf->pte, pte_mkexec(pte));
		__scid_flush_tlb_page(vmf->vma, vmf->address);
	}

	/* release the page table lock */
__unlock:
	spin_unlock(vmf->ptl);
}

/* mms lock is held */
static bool __check_prot_adjust(struct vm_fault *vmf, struct page_mms *pg_mms)
{
	//bool invalid_flags;
	bool same_prot;

	/*
	invalid_flags =
		!(vmf->flags & FAULT_FLAG_WRITE) && 
		!(vmf->flags & FAULT_FLAG_INSTRUCTION);
	if(invalid_flags)
		return false;
		*/

	same_prot = 
		(pg_mms->shadow_write && (vmf->flags & FAULT_FLAG_WRITE)) ||
		(!pg_mms->shadow_write && (vmf->flags & FAULT_FLAG_INSTRUCTION));
	if(same_prot) {
		scid_info("adjust");
		__prot_adjust_exec(vmf, !pg_mms->shadow_write);
		return false;
	}

	/* otherwise, we must actuate the alternation */
	pg_mms->shadow_write = vmf->flags & FAULT_FLAG_WRITE;
	scid_infof("fault flag write: %d, fault flag instr: %d",
			vmf->flags & FAULT_FLAG_WRITE, vmf->flags & FAULT_FLAG_INSTRUCTION);
	return true;
}

#endif /* DO_PTE_ALT_PROT */

/* lock is already taken */
void alternate_ptes_locked(struct page_status *pgs, struct vm_fault *vmf)
{

#ifdef DO_PTE_ALT_PROT
	struct folio *folio;

	if(!__check_prot_adjust(vmf, &pgs->pg_mms))
		return;

	scid_info("alternate");

	folio = page_folio(pgs->page);

	__do_alternate_ptes(folio, &pgs->pg_mms);

#endif /* DO_PTE_ALT_PROT */

}

void init_pg_mms(struct page_status *pgs)
{

#ifdef DO_PTE_ALT_PROT
	spin_lock_init(&pgs->pg_mms.lock);
	pgs->pg_mms.shadow_write = false;
	pgs->pg_mms.first_alt_handled = true;
	INIT_LIST_HEAD(&pgs->pg_mms.mms_head);
#endif  /* DO_PTE_ALT_PROT */

}

void free_mms_from_pgs(struct page_status *pgs)
{

#ifdef DO_PTE_ALT_PROT
	__del_all_mms(&pgs->pg_mms.mms_head);
#endif /* DO_PTE_ALT_PROT */

}

void zeroprot_ptes_locked(struct page_status *pgs)
{

}

void add_mm_to_pgs_locked(struct page_status *pgs, struct mm_struct *mm, unsigned long addr)
{

#ifdef DO_PTE_ALT_PROT
	struct mms *entry;
	struct mms *tmp;
	bool found = false;

	list_for_each_entry_safe(entry, tmp, &pgs->pg_mms.mms_head, node) {
		if(!mmget_not_zero(entry->mm)) {
			__drop_del_free_mms(entry);
			continue;
		}

		if(entry->addr == addr && entry->mm == mm)
			found = true;

		mmput_async(entry->mm);
	}

	if(found)
		return;

	if(unlikely(!__mms_grab_mm_and_publish_locked(mm, addr, &pgs->pg_mms.mms_head)))
		scid_err("memory exhausted");
#endif /* DO_PTE_ALT_PROT */

}

void fixup_prot_for_pte(struct page_status *pgs, pte_t *ptep)
{

}

void update_mm_addr(struct page_status *pgs, struct mm_struct *mm, 
		unsigned long old_addr, unsigned long new_addr)
{

}

int setup_ptealtprot(void)
{

#ifdef DO_PTE_ALT_PROT
	mms_cachep = kmem_cache_create(
			"scid__mms_cache", 
			sizeof(struct mms), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);
	if(!mms_cachep) {
		scid_err("unable to create a new kmem_cache for mms");
		return -ENOMEM;
	}

#endif /* DO_PTE_ALT_PROT */

	return 0;
}

void teardown_ptealtprot(void)
{

#ifdef DO_PTE_ALT_PROT
	kmem_cache_destroy(mms_cachep);
#endif /* DO_PTE_ALT_PROT */

}
