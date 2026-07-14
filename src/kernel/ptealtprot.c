#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/slab.h>
#include <pgtrack.h>
#include <logging.h>

static struct kmem_cache *pap_cachep;

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

void wrex_locked_ptealtprot(struct ptealtprot_struct *pap)
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
