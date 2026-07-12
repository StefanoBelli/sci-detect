#include <ptealtprot.h>

#if !defined(DIABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)

#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/rmap.h>

#include <resolve_syms/page_vma_mapped_walk.h>
#include <resolve_syms/rmap_walk.h>
#include <pgtrack.h>
#include <logging.h>

/* this gets useful when rmap lock is contended */
struct mms {
	struct mm_struct *mm;
	unsigned long addr;

	struct list_head node;
};

struct page_mms {
	spinlock_t lock;
	unsigned long shadow_write;
	struct list_head *mms_head;
};

static struct kmem_cache *mms_head_cachep;
static struct kmem_cache *page_mms_cachep;
static struct kmem_cache *mms_cachep;

#endif /* !defined(DIABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

void new_page_mms_lock_pgs(struct page_status *pgs)
{

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	struct page_mms *pg_mms;

	pg_mms = kmem_cache_alloc(page_mms_cachep, GFP_ATOMIC);
	if(!pg_mms) {
		scid_err("memory exhausted");
		return;
	}

	spin_lock_init(&pg_mms->lock);
	pg_mms->shadow_write = 0;
	pg_mms->mms_head = NULL;

	spin_lock(&pg_mms->lock);

	/* publish */
	pgs->pg_mms = pg_mms;
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}

void mms_lock(struct page_status *pgs)
{

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	spin_lock(&pgs->pg_mms->lock);
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}

void mms_unlock(struct page_status *pgs)
{

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	spin_unlock(&pgs->pg_mms->lock);
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
static void __del_all_mms(struct list_head *mms_head)
{
	struct mms *entry;

	list_for_each_entry(entry, mms_head, node) {
		mmdrop(entry->mm);
		kmem_cache_free(mms_cachep, entry);
	}

	kmem_cache_free(mms_head_cachep, mms_head);
}
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

void free_mms_from_pgs(struct page_status *pgs)
{

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	if(!pgs->pg_mms)
		return;

	if(pgs->pg_mms->mms_head)
		__del_all_mms(pgs->pg_mms->mms_head);

	kmem_cache_free(page_mms_cachep, pgs->pg_mms);
	pgs->pg_mms = NULL;
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)

struct rmap_one_altptes_args {
	/* passed by caller */
	struct page_mms *pg_mms;

	/* shall be read by caller */
	bool error;

	/* used by the callback internally */
	bool already_called;
};

/* to successfully "grab the mm", call this when sure that it will not go away... */
static bool __mms_grab_mm_and_publish(
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

static bool __rmap_one_altptes(
		__always_unused struct folio *folio, 
		struct vm_area_struct *vma, unsigned long addr, void* arg)
{
	struct rmap_one_altptes_args *args = arg;

	if(!args->already_called) {
		args->already_called = true;

		/*
		 * a possible option would be to queue_work this,
		 * but it is worth it?
		 */
		__del_all_mms(args->pg_mms->mms_head);

		args->pg_mms->mms_head = kmem_cache_alloc(mms_head_cachep, GFP_ATOMIC);
		if(unlikely(!args->pg_mms->mms_head))
			goto __memory_exhausted_error;

		INIT_LIST_HEAD(args->pg_mms->mms_head);
	}

	if(unlikely(!__mms_grab_mm_and_publish(
					vma->vm_mm, addr, args->pg_mms->mms_head)))
		goto __memory_exhausted_error;

	return true;

__memory_exhausted_error:
	args->error = true;
	scid_err("memory exhausted");
	return false;
}

static void __do_alternate_ptes(struct folio *folio, struct page_mms *pg_mms)
{
	struct mms *entry;
	struct mms *tmp;

	list_for_each_entry_safe(entry, tmp, pg_mms->mms_head, node) {
		/* the whole address space is not valid anymore */
		if(!mmget_not_zero(entry->mm))
			goto __failure_drop_delete;

		/* minor failure: unable to take the read lock */
		if(!mmap_read_trylock(entry->mm)) {
			mmput(entry->mm);
			continue;
		}

		struct vm_area_struct *vma;
		vma = vma_lookup(entry->mm, entry->addr);

		/* no such vma exists that contains 'addr' */
		if(!vma)
			goto __failure_unlock_put_drop_delete;

		DEFINE_FOLIO_VMA_WALK(pvmw, folio, vma, entry->addr, 0);
		bool map_ok = THUNK(page_vma_mapped_walk)(&pvmw);

		/* for some reason, unable to get the PTE */
		if(!map_ok)
			goto __failure_unlock_put_drop_delete;
		
		/* TODO do the actual alternation */
		/* TODO do the tlb flush */

__failure_unlock_put_drop_delete:
		mmap_read_unlock(entry->mm);
		mmput(entry->mm);
__failure_drop_delete:
		mmdrop(entry->mm);
		list_del(&entry->node);
		kmem_cache_free(mms_cachep, entry);
	}
}

#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

/* lock is already taken */
void alternate_ptes_locked(struct page_status *pgs, enum fault_flag flags)
{

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	struct folio *folio;
	struct rmap_one_altptes_args roa_args;
	struct rmap_walk_control rwc;

	/* determine the type of fault first */
	if(!(flags & FAULT_FLAG_WRITE)) {
		if(!(flags & FAULT_FLAG_INSTRUCTION)) {
			scid_err("invalid flags");
			return;
		} else
			/* enable shadow write */
			pgs->pg_mms->shadow_write = 1;
	} else
		/* disable shadow write */
		pgs->pg_mms->shadow_write = 0;

	/* attempt the rmap... */
	folio = page_folio(pgs->page);
	memset(&rwc, 0, sizeof(rwc));
	rwc.try_lock = true;
	rwc.rmap_one = __rmap_one_altptes;
	rwc.arg = &roa_args;
	roa_args.already_called = false;
	roa_args.pg_mms = pgs->pg_mms;
	roa_args.error = false;

	THUNK(rmap_walk)(folio, &rwc);

	if(roa_args.error)
		return;

	__do_alternate_ptes(folio, pgs->pg_mms);

#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}

void add_mm_to_pgs(struct page_status *pgs, struct mm_struct *mm, unsigned long addr)
{

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

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	page_mms_cachep = kmem_cache_create(
			"scid__page_mms_cache", 
			sizeof(struct page_mms), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);
	if(!page_mms_cachep) {
		scid_err("unable to create a new kmem_cache for page_mms");
		return -ENOMEM;
	}

	mms_cachep = kmem_cache_create(
			"scid__mms_cache", 
			sizeof(struct mms), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);
	if(!mms_cachep) {
		scid_err("unable to create a new kmem_cache for mms");
		goto __destroy_from_page_mms;
	}

	mms_head_cachep = kmem_cache_create(
			"scid__mms_head_cache", 
			sizeof(struct list_head), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);
	if(!mms_head_cachep) {
		scid_err("unable to create a new kmem_cache for mms heads");
		goto __destroy_from_mms;
	}

#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

	return 0;

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
__destroy_from_mms:
	kmem_cache_destroy(mms_cachep);
__destroy_from_page_mms:
	kmem_cache_destroy(page_mms_cachep);
	return -ENOMEM;
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}

void teardown_ptealtprot(void)
{

#if !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT)
	kmem_cache_destroy(mms_head_cachep);
	kmem_cache_destroy(mms_cachep);
	kmem_cache_destroy(page_mms_cachep);
#endif /* !defined(DISABLE_PAGE_SNAPSHOT) || !defined(DISABLE_PTE_ALT_PROT) */

}
