#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/compiler.h>

#include <resolve_syms/rmap_walk.h>
#include <resolve_syms/flush_tlb_mm_range.h>
#include <pgtrack.h>
#include <logging.h>

static struct kmem_cache *pap_cachep;

static inline void __scid_flush_tlb_page(struct vm_area_struct *vma, unsigned long a)
{
	THUNK(flush_tlb_mm_range)(vma->vm_mm, a, a + PAGE_SIZE, PAGE_SHIFT, false);
}

/* the mm and associated virtual addr */
struct addr_spc {
	struct mm_struct *mm;
	unsigned long addr;

	struct list_head node;
};

struct __rmap_one_addr_spc_args {
	/* head of the list */
	struct list_head *head;

	/* errored */
	bool errored;
};

static void free_addr_spcs_list(struct list_head **head)
{
	struct addr_spc *entry;
	struct addr_spc *tmp;

	list_for_each_entry_safe(entry, tmp, *head, node) {
		list_del(&entry->node);
		mmput_async(entry->mm);
		kfree(entry);
	}

	kfree(*head);
	*head = NULL;
}

static bool __rmap_one_addr_spc(
		__always_unused struct folio *folio, struct vm_area_struct *vma, 
		unsigned long addr, void *arg)
{
	struct __rmap_one_addr_spc_args *args = arg;
	struct addr_spc *aspc;

	/* even though it may be possible to sleep, try to hold the
	 * rmap lock (whether it is for a file-backed or anonymous mapping)
	 * least as possible */
	aspc = kmalloc(sizeof(struct addr_spc), GFP_ATOMIC);
	if(unlikely(!aspc)) {
		scid_err("memory exhausted");
		args->errored = true;
		return false;
	}

	aspc->mm = vma->vm_mm;
	aspc->addr = addr;
	list_add(&aspc->node, args->head);

	mmgrab(aspc->mm);
	return true;
}

/* may sleep, acquires rmap lock */
static struct list_head *all_addr_spcs_from_folio(struct folio *folio)
{
	struct __rmap_one_addr_spc_args args;
	struct rmap_walk_control rwc;
	struct list_head *head;

	if(unlikely(!folio_mapped(folio)))
		return NULL;

	folio_lock(folio);

	head = kmalloc(sizeof(struct list_head), GFP_KERNEL);
	if(unlikely(!head)) {
		scid_err("memory exhausted");
		goto __unlock;
	}

	INIT_LIST_HEAD(head);

	memset(&args, 0, sizeof(args));
	memset(&rwc, 0, sizeof(rwc));

	args.head = head;
	rwc.rmap_one = __rmap_one_addr_spc;
	rwc.arg = &args;

	/* this acquires rmap lock */
	THUNK(rmap_walk)(folio, &rwc);

	if(unlikely(args.errored)) {
		scid_err("rmap_walk errored");
		goto __free_list_unlock;
	}

	folio_unlock(folio);
	return head;

__free_list_unlock:
	free_addr_spcs_list(&head);
__unlock:
	folio_unlock(folio);
	return NULL;
}

#endif

void new_ptealtprot(struct page_status *pgs)
{

#ifdef DO_PTE_ALT_PROT
	pgs->pap = kmem_cache_alloc(pap_cachep, GFP_ATOMIC);
	if(!pgs->pap) {
		scid_err("memory exhausted");
		return;
	}

	pgs->pap->init = true;
	pgs->pap->write = false;
	mutex_init(&pgs->pap->lock);
#endif

}

void free_ptealtprot(struct page_status *pgs)
{

#ifdef DO_PTE_ALT_PROT	
	if(pgs->pap)
		kmem_cache_free(pap_cachep, pgs->pap);
#endif

}

void wrex_locked_ptealtprot(struct ptealtprot_struct *pap, enum fault_flag ff)
{

}

void exonly_locked_ptealtprot(struct ptealtprot_struct *pap)
{

}

void none_locked_ptealtprot(struct ptealtprot_struct *pap)
{

}

void pte_fixup_locked_ptealtprot(pte_t* ptep, struct ptealtprot_struct *pap)
{

}

int setup_ptealtprot(void)
{

#ifdef DO_PTE_ALT_PROT
	pap_cachep = kmem_cache_create(
			"scid__pap_cache", 
			sizeof(struct ptealtprot_struct), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);

	if(!pap_cachep)
		return -ENOMEM;
#endif

	return 0;
}

void teardown_ptealtprot(void)
{

#ifdef DO_PTE_ALT_PROT
	kmem_cache_destroy(pap_cachep);
#endif

}
