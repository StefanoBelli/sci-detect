#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/compiler.h>
#include <linux/sched/mm.h>
#include <linux/workqueue.h>
#include <asm-generic/rwonce.h>
#include <asm-generic/barrier.h>

#include <resolve_syms/rmap_walk.h>
#include <resolve_syms/flush_tlb_mm_range.h>
#include <resolve_syms/page_vma_mapped_walk.h>
#include <kpsleepable.h>
#include <pgtrack.h>
#include <logging.h>

#ifndef DISABLE_PAGE_SNAPSHOT
#	include <pgsnap.h>
#endif

static struct kmem_cache *pap_cachep;
static struct workqueue_struct *drop_mm_wq;

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

#define warn_sync_mmdrop(mm, kp) \
	do { \
		scid_warn("fallback to non-work-deferring offloading mmdrop"); \
		KPSLEEPABLE((kp), \
				mmdrop((mm)); \
		); \
	} while(0)

static void free_addr_spc(struct addr_spc *entry, struct kprobe *kp)
{
	struct drop_mm_work_args *work_args;

	work_args = kmalloc(sizeof(struct drop_mm_work_args), GFP_ATOMIC);
	if(unlikely(!work_args))
		warn_sync_mmdrop(entry->mm, kp);
	else {
		work_args->mm = entry->mm;
		INIT_WORK(&work_args->work, __drop_mm_work);
		if(!queue_work(drop_mm_wq, &work_args->work)) {
			scid_err("unable to queue work");
			warn_sync_mmdrop(entry->mm, kp);
		}
	}

	list_del(&entry->node);
	kfree(entry);
}

#undef warn_sync_mmdrop

struct __rmap_one_addr_spc_args {
	/* head of the list */
	struct list_head *head;

	/* errored */
	bool errored;
};

static void free_addr_spcs_list(struct list_head **head, struct kprobe *kp)
{
	struct addr_spc *entry;
	struct addr_spc *tmp;

	list_for_each_entry_safe(entry, tmp, *head, node)
		free_addr_spc(entry, kp);

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
static struct list_head *all_addr_spcs_from_folio(
		struct folio *folio, struct kprobe *kp)
{
	struct __rmap_one_addr_spc_args args;
	struct rmap_walk_control rwc;
	struct list_head *head;

	if(unlikely(!folio_mapped(folio)))
		return NULL;

	KPSLEEPABLE(kp, 
			folio_lock(folio);
			head = kmalloc(sizeof(struct list_head), GFP_KERNEL);
	);

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
	KPSLEEPABLE(kp, 
			THUNK(rmap_walk)(folio, &rwc);
	);

	if(unlikely(args.errored)) {
		scid_err("rmap_walk errored");
		goto __free_list_unlock;
	}

	folio_unlock(folio);
	return head;

__free_list_unlock:
	free_addr_spcs_list(&head, kp);
__unlock:
	folio_unlock(folio);
	return NULL;
}

#define __for_each_addr_spc_do(name, addr_spcs_head, rlk_cur_mm, __action__) \
	do { \
		struct addr_spc *name; \
		\
		list_for_each_entry(name, (addr_spcs_head), node) { \
			if(!(rlk_cur_mm) && name->mm == current->mm) \
				continue; \
			\
			__action__\
		} \
	} while(0)

static inline void rlock_all_mm_in_addr_spcs(
		struct list_head *addr_spcs_head, bool rlk_cur_mm, struct kprobe *kp)
{
	__for_each_addr_spc_do(entry, addr_spcs_head, rlk_cur_mm, 
			KPSLEEPABLE(kp, 
				mmap_read_lock(entry->mm);
			);
	);
}

static inline void unlock_all_mm_in_addr_spcs(
		struct list_head *addr_spcs_head, bool rlk_cur_mm)
{
	__for_each_addr_spc_do(entry, addr_spcs_head, rlk_cur_mm, 
			mmap_read_unlock(entry->mm);
	);
}

#undef __for_each_addr_spc_do

/* callback that passes one pte (one for each call)
 *
 * when this is called, the following locks are taken:
 *  1. the ptealtprot lock
 *  2. the mmap read lock
 *  3. the ptl lock
 *
 * Not necessarily in this particular order, see header
 * docs.
 */
typedef void (*pte_one_fpt)(
		pte_t* ptep, 
		struct vm_area_struct *vma, 
		unsigned long addr, 
		struct ptealtprot_struct *pap);

static void ptes_walk_from_folio_locked(
		struct folio *folio, pte_one_fpt pte_one, 
		struct ptealtprot_struct *pap, struct list_head *addr_spcs_head,
		pte_t *skip_ptep, struct kprobe *kp)
{
	struct addr_spc *entry;
	struct addr_spc *pos;

	KPSLEEPABLE(kp, 
			folio_lock(folio);
	);

	list_for_each_entry_safe(entry, pos, addr_spcs_head, node) {
		struct vm_area_struct *vma;
		bool map_ok;

		/* the whole address space is not valid anymore */
		if(!mmget_not_zero(entry->mm))
			goto __failure_put;

		/* lookup the vma */
		vma = vma_lookup(entry->mm, entry->addr);
		if(!vma)
			goto __failure_put;

		/* page_vma_mapped_walk */
		DEFINE_FOLIO_VMA_WALK(pvmw, folio, vma, entry->addr, 0);
		map_ok = THUNK(page_vma_mapped_walk)(&pvmw);

		/* for some reason, unable to get the PTE */
		if(unlikely(!map_ok))
			goto __failure_put;

		/* consistency checks */
		if(likely(pvmw.pmd && pvmw.pte)) {
			if(likely(pvmw.ptl && spin_is_locked(pvmw.ptl))) {

				/* pass the ptep */
				if(likely(pte_one && pvmw.pte != skip_ptep))
					pte_one(pvmw.pte, vma, entry->addr, pap);

				pte_unmap_unlock(pvmw.pte, pvmw.ptl);
			} else
				scid_warn("expecting the ptl lock");
		} else
			scid_warn("pvmw returned invalid config");

		/* before next iteration */
__failure_put:
		mmput_async(entry->mm);
	}

	folio_unlock(folio);
}

#define INVALID_PTE(pte) \
	(pte_none((pte)) || !pte_present((pte)))

static void noneprot_pte_one(
		pte_t* ptep, 
		struct vm_area_struct *vma, 
		unsigned long addr,
		__always_unused struct ptealtprot_struct *pap)
{
	pte_t pte = ptep_get(ptep);

	if(INVALID_PTE(pte))
		return;

	pte = pte_set_flags(pte, _PAGE_NX);
	pte = pte_wrprotect(pte);

	set_pte(ptep, pte);
	__scid_flush_tlb_page(vma, addr);
}

static void exonly_pte_one(
		pte_t* ptep, 
		struct vm_area_struct *vma, 
		unsigned long addr,
		__always_unused struct ptealtprot_struct *pap)
{
	pte_t pte = ptep_get(ptep);

	if(INVALID_PTE(pte))
		return;

	if(vma->vm_flags & VM_EXEC) 
		pte = pte_mkexec(pte);

	pte = pte_wrprotect(pte);

	set_pte(ptep, pte);
	__scid_flush_tlb_page(vma, addr);
}

static void wrex_pte_one(
		pte_t* ptep, 
		struct vm_area_struct *vma, 
		unsigned long addr,
		struct ptealtprot_struct *pap)
{
	pte_t pte = ptep_get(ptep);

	if(INVALID_PTE(pte))
		return;

	if(pap->write)
		pte = pte_set_flags(pte, _PAGE_NX);
	else {
		if(vma->vm_flags & VM_EXEC)
			pte = pte_mkexec(pte);
	}

	/* CoW reasons */
	pte = pte_wrprotect(pte);

	set_pte(ptep, pte);
	__scid_flush_tlb_page(vma, addr);
}

static inline void maybe_mkwrite_mypte(bool shdw_write, struct my_pte_info *mpi)
{
	if(shdw_write && mpi->vma->vm_flags & VM_WRITE) {
		pte_t pte;

		spin_lock(mpi->ptlp);

		pte = ptep_get(mpi->ptep);
		if(INVALID_PTE(pte)) {
			spin_unlock(mpi->ptlp);
			return;
		}

		pte = pte_mkwrite_novma(pte);
		pte = pte_set_flags(pte, _PAGE_NX);
		set_pte(mpi->ptep, pte);
		spin_unlock(mpi->ptlp);
		__scid_flush_tlb_page(mpi->vma, mpi->addr);
	}
}

#undef INVALID_PTE

static struct folio* __init_collect_aspcs_and_lock(
		struct page_status *pgs, struct list_head **addr_spcs_head, 
		bool rlkmm, struct kprobe *kp)
{
	struct folio *folio;

	folio = page_folio(pgs->page);
	if(!folio_try_get(folio)) {
		scid_warn("unable to get folio, this is strange :/");
		return NULL;
	}

	*addr_spcs_head = all_addr_spcs_from_folio(folio, kp);
	rlock_all_mm_in_addr_spcs(*addr_spcs_head, rlkmm, kp);

	KPSLEEPABLE(kp, 
			mutex_lock(&pgs->pap->lock);
	);

	return folio;
}

static void __end_free_aspcs_and_unlock(
		struct page_status *pgs, struct list_head **addr_spcs_head, 
		bool rlkmm, struct folio *folio, struct kprobe *kp)
{
	mutex_unlock(&pgs->pap->lock);
	unlock_all_mm_in_addr_spcs(*addr_spcs_head, rlkmm);
	folio_put(folio);

	free_addr_spcs_list(addr_spcs_head, kp);
}

#ifndef DISABLE_PAGE_SNAPSHOT
#	define DEFINE_INITIATED_RIGHT_NOW() bool initiated_right_now = false
#else
#	define DEFINE_INITIATED_RIGHT_NOW()
#endif

#ifndef DISABLE_PAGE_SNAPSHOT
#	define SET_INITIATED_RIGHT_NOW() \
		do { \
			initiated_right_now = true; \
		} while(0)
#else
#	define SET_INITIATED_RIGHT_NOW()
#endif

#ifdef DEBUG_PRINTS_PTEALTPROT
#	define DEBUG_PAP(x) x
#else
#	define DEBUG_PAP(x)
#endif

#ifndef DISABLE_PAGE_SNAPSHOT

static inline void do_snapshot(
		struct page_status *pgs, struct snapshot_extras *snapex, 
		enum fault_flag ff, bool irn, __maybe_unused const char* name)
{
	if(!irn) {
		DEBUG_PAP(scid_infof("%s doing a snapshot", name));
		make_page_snap(pgs, snapex->pid, snapex->pfn, snapex->raddr, ff);
	}
}

#else
#	define do_snapshot(pgs, snapex, ff, irn, name)
#endif

#endif /* DO_PTE_ALT_PROT */

void new_ptealtprot(__maybe_unused struct page_status *pgs)
{

#ifdef DO_PTE_ALT_PROT
	struct ptealtprot_struct *pap;

	pap = kmem_cache_alloc(pap_cachep, GFP_ATOMIC);
	if(!pap) {
		scid_err("memory exhausted");
		return;
	}

	pap->noprot = false;
	pap->init = true;
	pap->write = false;
	mutex_init(&pap->lock);

	WRITE_ONCE(pgs->pap, pap);
	smp_mb();

#endif

}

void free_ptealtprot(__maybe_unused struct page_status *pgs)
{

#ifdef DO_PTE_ALT_PROT	
	if(pgs->pap)
		kmem_cache_free(pap_cachep, pgs->pap);
#endif

}

/*
 * possible configs:
 *
 *  --> first call is this
 *  init = true, noprot = false,
 *
 *  --> regular checks, we don't care
 *  init = false, noprot = false,
 *
 *  --> first call was none_ptealtprot 
 *  init = false, noprot = true,
 */

#ifdef DO_PTE_ALT_PROT

#define PHASED_PTEALTPROT(__pap, __on_init__, __on_noprot__, __on_regular__) \
	\
	if((__pap)->init) { \
		/* this is the first call */ \
		BUG_ON((__pap)->write); \
		BUG_ON((__pap)->noprot); \
		\
		/* here we need to choose the init mode */ \
		__on_init__ \
		\
		(__pap)->init = false; \
	} else { \
		/* this is NOT the first call */ \
		if((__pap)->noprot) { \
			/* prior call disabled both W and X */ \
			BUG_ON((__pap)->write); \
			\
			/* here we need to choose the noprot mode */ \
			__on_noprot__ \
			\
			(__pap)->noprot = false; \
		} else { \
			\
			/* do regular operations */ \
			__on_regular__ \
			\
		} \
	}

#endif

void wrex_ptealtprot(
		__maybe_unused struct page_status *pgs, 
		__maybe_unused enum fault_flag ff, 
		__maybe_unused bool rlkmm,
		__maybe_unused struct my_pte_info *mpi,
		__maybe_unused struct snapshot_extras *snapex,
		__maybe_unused struct kprobe *kp)
{

#ifdef DO_PTE_ALT_PROT
	struct folio *folio;
	bool invalid_flags;
	struct list_head *addr_spcs_head;
	DEFINE_INITIATED_RIGHT_NOW();

	invalid_flags = 
		!ff || 
		(!(ff & FAULT_FLAG_INSTRUCTION) && !(ff & FAULT_FLAG_WRITE)) ||
		((ff & FAULT_FLAG_INSTRUCTION) && (ff & FAULT_FLAG_WRITE));

	scid_infof("called, invalid_flags=%d, %d, %d %d", invalid_flags, ff, ff & FAULT_FLAG_WRITE, ff & FAULT_FLAG_INSTRUCTION);

	if(unlikely(invalid_flags))
		return;

	folio = __init_collect_aspcs_and_lock(pgs, &addr_spcs_head, rlkmm, kp);
	if(!folio) {
		scid_warn("unable to get folio");
		return;
	}

	PHASED_PTEALTPROT(
			pgs->pap
			,

			/* on init */
			SET_INITIATED_RIGHT_NOW();
			pgs->pap->write = ff & FAULT_FLAG_WRITE;
			,

			/* on noprot */
			pgs->pap->write = ff & FAULT_FLAG_WRITE;
			,

			/* on regular */
			bool must_alternate;

			must_alternate = 
				(pgs->pap->write && (ff & FAULT_FLAG_INSTRUCTION)) ||
				(!pgs->pap->write && (ff & FAULT_FLAG_WRITE));

			if(must_alternate) {

				scid_info("alternation!");
				pgs->pap->write = !pgs->pap->write;
			} else {

				scid_info("NOT alternating");
				goto __unlock_all_and_free;
			}
	);

	do_snapshot(pgs, snapex, ff, initiated_right_now, "wrex");
	ptes_walk_from_folio_locked(
			folio, wrex_pte_one, pgs->pap, addr_spcs_head, mpi->ptep, kp);

__unlock_all_and_free:
	maybe_mkwrite_mypte(pgs->pap->write, mpi);

	__end_free_aspcs_and_unlock(pgs, &addr_spcs_head, rlkmm, folio, kp);

#endif

}

void exonly_ptealtprot(
		__maybe_unused struct page_status *pgs, 
		__maybe_unused bool rlkmm,
		__maybe_unused struct snapshot_extras *snapex,
		__maybe_unused struct kprobe *kp)
{

#ifdef DO_PTE_ALT_PROT
	struct list_head *addr_spcs_head;
	struct folio *folio;
	DEFINE_INITIATED_RIGHT_NOW();

	folio = __init_collect_aspcs_and_lock(pgs, &addr_spcs_head, rlkmm, kp);
	if(!folio) {
		scid_warn("unable to get folio");
		return;
	}

	PHASED_PTEALTPROT(
			pgs->pap
			,

			/* on init */
			/* here we keep write disabled as we are going
		 	 * to init to allow exec, not write */
		 	SET_INITIATED_RIGHT_NOW();
			,

			/* on noprot */
			/* here we keep write disabled as we are going
			 * to init to allow exec, not write */
			,

			/* on regular */
			/* prior call disabled either W or X */
			if(pgs->pap->write)
				/* ... if disabled X, alternate by disabling W */
				pgs->pap->write = false;
			else
				/* ... othw, if disabled W, keep it disabled */
				goto __unlock_all_and_free;
	);

	do_snapshot(pgs, snapex, FAULT_FLAG_INSTRUCTION, initiated_right_now, "exonly");
	ptes_walk_from_folio_locked(
			folio, exonly_pte_one, pgs->pap, addr_spcs_head, NULL, kp);

__unlock_all_and_free:
	__end_free_aspcs_and_unlock(pgs, &addr_spcs_head, rlkmm, folio, kp);
#endif

}

#ifdef DO_PTE_ALT_PROT
#undef PHASED_PTEALTPROT
#endif

/* 
 * this is called within a running system call (AND NOT the
 * page fault handler)
 */
bool none_ptealtprot(
		__maybe_unused struct page_status *pgs, 
		__maybe_unused bool rlkmm,
		__maybe_unused struct snapshot_extras *snapex,
		__maybe_unused struct kprobe *kp)
{
	bool rv = true;

#ifdef DO_PTE_ALT_PROT
	struct list_head *addr_spcs_head;
	struct folio *folio;
	DEFINE_INITIATED_RIGHT_NOW();

	folio = __init_collect_aspcs_and_lock(pgs, &addr_spcs_head, rlkmm, kp);
	if(!folio) {
		scid_warn("unable to get folio");
		return rv;
	}

	/* if already initiated, don't do anything */
	if(!pgs->pap->init) {
		rv = false;
		goto __unlock_all_and_free;
	} else {
		/* this can't be possible, since this is the only
		 * call that does noprot enabling, and disables init
		 * that is, init = true and noprot = true can't be possible
		 */
		BUG_ON(pgs->pap->noprot);
		SET_INITIATED_RIGHT_NOW();
	}

	pgs->pap->init = false;
	pgs->pap->noprot = true;

	do_snapshot(pgs, snapex, 0, initiated_right_now, "noneprot");

	/* otherwise, zeroprot all the PTEs */
	ptes_walk_from_folio_locked(
			folio, noneprot_pte_one, NULL, addr_spcs_head, NULL, kp);

__unlock_all_and_free:
	__end_free_aspcs_and_unlock(pgs, &addr_spcs_head, rlkmm, folio, kp);

#endif

	return rv;
}

/* todo use my_pte_info */
void pte_fixup_ptealtprot(
		__maybe_unused struct page_status *pgs,
		__maybe_unused struct my_pte_info *mpi)
{

#ifdef DO_PTE_ALT_PROT
	pte_t pte;

	if(pgs->pap->init)
		return;

	spin_lock(mpi->ptlp);

	pte = ptep_get(mpi->ptep);

	pte = pte_wrprotect(pte);

	if(pgs->pap->noprot)
		pte = pte_set_flags(pte, _PAGE_NX);
	else {
		if(pgs->pap->write)
			pte = pte_set_flags(pte, _PAGE_NX);
		else {
			if(mpi->vma->vm_flags & VM_EXEC)
				pte = pte_mkexec(pte);
		}
	}

	spin_unlock(mpi->ptlp);

	__scid_flush_tlb_page(mpi->vma, mpi->addr);

#endif

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

	if(!pap_cachep) {
		scid_err("unable to create cache");
		return -ENOMEM;
	}

	drop_mm_wq = alloc_workqueue("scid-drop-mm-wq", WQ_PERCPU, 0);
	if(!drop_mm_wq) {
		scid_err("unable to create wq");
		kmem_cache_destroy(pap_cachep);
		return -ENOMEM;
	}
#endif

	return 0;
}

void teardown_ptealtprot(void)
{

#ifdef DO_PTE_ALT_PROT
	flush_workqueue(drop_mm_wq);
	drain_workqueue(drop_mm_wq);
	destroy_workqueue(drop_mm_wq);

	kmem_cache_destroy(pap_cachep);
#endif

}
