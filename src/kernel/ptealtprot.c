#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/mmap_lock.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/compiler.h>
#include <linux/sched/mm.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/kdebug.h>
#include <linux/jiffies.h>
#include <linux/hashtable.h>
#include <linux/version.h>
#include <asm-generic/rwonce.h>
#include <asm-generic/barrier.h>

#include <resolve_syms/rmap_walk.h>
#include <resolve_syms/flush_tlb_mm_range.h>
#include <resolve_syms/page_vma_mapped_walk.h>
#include <resolve_syms/task_work_add.h>
#include <resolve_syms/task_work_cancel_match.h>
#include <kpsleepable.h>
#include <pgtrack.h>
#include <logging.h>
#include <user_page_walk.h>

#ifndef DISABLE_PAGE_SNAPSHOT
#	include <pgsnap.h>
#endif

//#define DEBUG_PRINTS_PTEALTPROT
#ifdef DEBUG_PRINTS_PTEALTPROT
#	include <linux/pid.h>
#	include <linux/sched.h>
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
	struct vm_area_struct *vma;
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

	/* don't even think about it. 
	 * Cannot assign aspc->vma = vma and later skip the
	 * vma_lookup.
	 */
	aspc->vma = NULL;
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

static inline void rlock_all_mm_in_addr_spcs(
		struct list_head *addr_spcs_head, 
		struct mms_lock_control *mmslk, struct kprobe *kp)
{
	struct addr_spc *entry;
	struct addr_spc *tmp;
	struct mm_struct *target_mm = mmslk->target_mm;
	struct vm_area_struct *target_vma = mmslk->target_vma;
	bool rlock_target_mm = mmslk->rlock_target_mm;

	list_for_each_entry_safe(entry, tmp, addr_spcs_head, node) {
		if(!rlock_target_mm && entry->mm == target_mm) {
			if(target_vma) {
				vma_assert_locked(target_vma);
				entry->vma = target_vma;
			} else {
				mmap_assert_locked(target_mm);
				entry->vma = NULL;
			}

			continue;
		}

		if(!mmslk->trylock)
			KPSLEEPABLE(kp, 
					mmap_read_lock(entry->mm);
			);
		else {
			if(!mmap_read_trylock(entry->mm)) {
				scid_warn("unable to acquire mmap_read_trylock");
				free_addr_spc(entry, kp);
			}
		}
	}
}

static inline void unlock_all_mm_in_addr_spcs(
		struct list_head *addr_spcs_head, struct mms_lock_control *mmslk)
{
	struct addr_spc *entry;
	struct addr_spc *tmp;
	struct mm_struct *target_mm = mmslk->target_mm;
	struct vm_area_struct *target_vma = mmslk->target_vma;
	bool rlock_target_mm = mmslk->rlock_target_mm;

	list_for_each_entry_safe(entry, tmp, addr_spcs_head, node) {
		if(!rlock_target_mm && entry->mm == target_mm) {
			if(!target_vma)
				mmap_assert_locked(target_mm);
			else
				vma_assert_locked(target_vma);

			continue;
		}

		mmap_read_unlock(entry->mm);
	}
}

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
		struct vm_area_struct *vma = entry->vma;
		bool map_ok;

		/* the whole address space is not valid anymore */
		if(!mmget_not_zero(entry->mm))
			goto __failure_put;

		/* lookup the vma if not already present 
		 * (the per-VMA-locked target_vma is not present) */
		if(!vma) {
			/* don't even think about skipping this by assigning
			 * the vma found in the rmap phase. mmap_lock acq/rel/acq
			 * In other words, the read vma ptr from rmap phase 
			 * may not be valid anymore due to the mmap_lock being dropped.
			 */
			vma = vma_lookup(entry->mm, entry->addr);
			if(!vma)
				goto __failure_put;
		}

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

/* used to ensure lock acquisition ordering, comparator callback, see below */
static int __addr_spc_cmp_by_mm(
		__always_unused void *priv, 
		const struct list_head *a, const struct list_head *b)
{
	struct addr_spc *aspc_a = list_entry(a, struct addr_spc, node);
	struct addr_spc *aspc_b = list_entry(b, struct addr_spc, node);
	unsigned long mm_a = (unsigned long) aspc_a->mm;
	unsigned long mm_b = (unsigned long) aspc_b->mm;

	if(mm_a < mm_b)
		return -1;

	if(mm_a > mm_b)
		return 1;

	return 0;
}

static struct folio* __init_collect_aspcs_and_lock(
		struct page_status *pgs, struct list_head **addr_spcs_head, 
		struct mms_lock_control *mmslk, struct kprobe *kp)
{
	struct folio *folio;

	folio = page_folio(pgs->page);
	if(!folio_try_get(folio)) {
		scid_warn("unable to get folio, this is strange :/");
		return NULL;
	}

	/* strict locking order required 
	 * be advised: ** DEADLOCK ** risk */
#ifdef PAP_INIT_LOCK_ORDER_STRICT
	KPSLEEPABLE(kp,
			mutex_lock(&pgs->pap->lock);
	);
#endif /* PAP_INIT_LOCK_ORDER_STRICT */

	*addr_spcs_head = all_addr_spcs_from_folio(folio, kp);
	if(!*addr_spcs_head) {

		/* strict locking order required */
#ifdef PAP_INIT_LOCK_ORDER_STRICT
		mutex_unlock(&pgs->pap->lock);
#endif /* PAP_INIT_LOCK_ORDER_STRICT */

		scid_warn("unable to collect addr spcs from folio");
		folio_put(folio);
		return NULL;
	}

	/* this is needed to ensure lock acquisition ordering */
	list_sort(NULL, *addr_spcs_head, __addr_spc_cmp_by_mm);

	rlock_all_mm_in_addr_spcs(*addr_spcs_head, mmslk, kp);

	/* default case: no strict locking order */
#ifndef PAP_INIT_LOCK_ORDER_STRICT
	KPSLEEPABLE(kp, 
			mutex_lock(&pgs->pap->lock);
	);
#endif /* !PAP_INIT_LOCK_ORDER_STRICT */

	return folio;
}

static void __end_unlock_put_free_aspcs(
		struct page_status *pgs, struct list_head **addr_spcs_head, 
		struct mms_lock_control *mmslk, struct folio *folio, struct kprobe *kp)
{

	/*
	 * the following two preprocessor directives are used to gurantee
	 * the lock acquisition/release pattern
	 */

	/* default case: no strict locking order */
#ifndef PAP_INIT_LOCK_ORDER_STRICT
	mutex_unlock(&pgs->pap->lock);
#endif /* !PAP_INIT_LOCK_ORDER_STRICT */

	unlock_all_mm_in_addr_spcs(*addr_spcs_head, mmslk);

	/* strict locking order required */
#ifdef PAP_INIT_LOCK_ORDER_STRICT
	mutex_unlock(&pgs->pap->lock);
#endif /* PAP_INIT_LOCK_ORDER_STRICT */

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

#	define __debug_pap_common(__name, __strx) \
		scid_infof("debug-pap (%s): task=(%d, %s) -  " __strx, \
				__name, task_pid_nr(current), current->comm

#	define DEBUG_PAP_FMT(name, strf, ...) \
		__debug_pap_common(name, strf), __VA_ARGS__)

#	define DEBUG_PAP(name, str) \
		__debug_pap_common(name, str))

#	define WREX "wrex"
#	define EXONLY "exonly"
#	define NONE "none"
#	define PTE_FIXUP "pte-fixup"

#else

#	define DEBUG_PAP_FMT(name, strf, ...)
#	define DEBUG_PAP(name, str)

#	define WREX NULL
#	define EXONLY NULL
#	define NONE NULL
#	define PTE_FIXUP NULL

#endif

#ifndef DISABLE_PAGE_SNAPSHOT

static inline void do_snapshot(
		struct page_status *pgs, struct snapshot_extras *snapex, 
		enum fault_flag ff, bool irn, __maybe_unused const char* name)
{
	/*
	 * irn stands for "initiated right now". We don't do the snapshot
	 * again if do_snapshot is called by a initializer of ptealtprot
	 * for the specific physical frame. This is because pgsnap already
	 * did the snapshot at the wx-page detection in this control path
	 */
	if(!irn) {
		DEBUG_PAP_FMT(name, "doing snapshot: pfn=%ld, raddr=%px, ff=%x", 
				snapex->pfn, (void*) snapex->raddr, ff);

		make_page_snap(pgs, snapex->pid, snapex->pfn, snapex->raddr, ff);
	}
}

#else
#	define do_snapshot(pgs, snapex, ff, irn, name)
#endif

/* data structures and functions for the "code changes WX page it resides into" case */
struct orig_ip {
	struct task_struct *tsk;
	unsigned long ip;
	struct hlist_node node;
};

/* delayed cleanup work */
struct orig_ip_dwork {
	struct delayed_work dwork;
	bool rearm;
};

static DEFINE_HASHTABLE(ht_orig_ips, 7);
static DEFINE_SPINLOCK(ht_orig_ips_lock);
static struct orig_ip_dwork ht_orig_ips_cleanup_dwork;

static inline void orig_ip_task_add(unsigned long orig_ip)
{
	unsigned long cpu_flags;
	struct orig_ip *oip = kmalloc(sizeof(struct orig_ip), GFP_ATOMIC);

	if(!oip) {
		scid_err("memory exhausted");
		return;
	}

	/* save orig ip */
	oip->ip = orig_ip;
	oip->tsk = current;
	INIT_HLIST_NODE(&oip->node);

	/* 
	 * we may need to inspect task descriptor in cleanup work 
	 * this is completely safe since oip->tsk = current 
	 */
	get_task_struct(oip->tsk);

	spin_lock_irqsave(&ht_orig_ips_lock, cpu_flags);
	hash_add(ht_orig_ips, &oip->node, (u64) oip->tsk);
	spin_unlock_irqrestore(&ht_orig_ips_lock, cpu_flags);
}

static inline void __remove_orig_ip_task(struct orig_ip *entry)
{
	/* 
	 * entry->tsk is current (see stmt above),
	 * put_task_struct cannot trigger any kind of
	 * resource release (no BUG related to sched while atomic)
	 */
	put_task_struct(entry->tsk);
	hash_del(&entry->node);
	kfree(entry);
}

static inline bool orig_ip_task_lookup(unsigned long *ip)
{
	struct orig_ip *entry;
	struct hlist_node *tmp;

	unsigned long cpu_flags;

	spin_lock_irqsave(&ht_orig_ips_lock, cpu_flags);

	hash_for_each_possible_safe(ht_orig_ips, entry, tmp, node, (u64) current) {
		if(entry->tsk == current) {
			/* restore orig ip */
			*ip = entry->ip;

			/* destroy */
			__remove_orig_ip_task(entry);

			/* unlock */
			spin_unlock_irqrestore(&ht_orig_ips_lock, cpu_flags);
			return true;
		}
	}

	/* unlock */
	spin_unlock_irqrestore(&ht_orig_ips_lock, cpu_flags);
	return false;
}

#define FIVE_MINUTES_MS 300000 

static void orig_ip_task_cleanup_dwork(struct work_struct *work)
{
	unsigned long cpu_flags;
	struct hlist_node *node;
	struct hlist_node *tmp;
	struct orig_ip *entry;
	struct orig_ip_dwork *mywork = container_of(work, struct orig_ip_dwork, dwork.work);

	/* take the lock */
	spin_lock_irqsave(&ht_orig_ips_lock, cpu_flags);

	/* iterate on each element of the hashtable */
	hlist_for_each_safe(node, tmp, ht_orig_ips) {
		entry = container_of(node, struct orig_ip, node);

		/* if task has done... */
		if(entry->tsk->exit_state & (EXIT_ZOMBIE | EXIT_DEAD))
			/* ... destroy */
			__remove_orig_ip_task(entry);
	}

	/* release the lock */
	spin_unlock_irqrestore(&ht_orig_ips_lock, cpu_flags);

	/* schedule this work 5 mins from now on the system wq */
	if(smp_load_acquire(&mywork->rearm)) {
		if(!schedule_delayed_work(&ht_orig_ips_cleanup_dwork.dwork, msecs_to_jiffies(FIVE_MINUTES_MS)))
			scid_err("unable to schedule_delayed_work");
	}
}

/* also checks for ip == addr situation, e.g. instr "0x00 0x00" with pointing "rax" */
static void maybe_mkwrite_mypte(bool shdw_write, struct my_pte_info *mpi, struct kprobe *kp)
{
	pte_t pte;
	unsigned long user_ip;
	struct pt_regs *current_pt_regs;

	if(shdw_write && mpi->vma->vm_flags & VM_WRITE) {
		spin_lock(mpi->ptlp);

		pte = ptep_get(mpi->ptep);
		if(INVALID_PTE(pte)) {
			spin_unlock(mpi->ptlp);
			return;
		}

		DEBUG_PAP(WREX, "making my own pte writable and non-exec");

		current_pt_regs = task_pt_regs(current);
		user_ip = instruction_pointer(current_pt_regs);

		pte = pte_mkwrite_novma(pte);

		/* check if the ran instruction is trying to change the same (WX) page it resides into */
		if((user_ip & PAGE_MASK) == (mpi->addr & PAGE_MASK)) {
			if(likely(mpi->vma->vm_flags & VM_EXEC)) {
				/* make it executable */
				pte = pte_mkexec(pte);

				/* enable single stepping for "current" */
				current_pt_regs->flags |= X86_EFLAGS_TF;

				/* add entry to the hashtable */
				orig_ip_task_add(user_ip);
			}
		} else 
			pte = pte_set_flags(pte, _PAGE_NX);

		set_pte(mpi->ptep, pte);
		spin_unlock(mpi->ptlp);
		__scid_flush_tlb_page(mpi->vma, mpi->addr);
	}
}

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
		__maybe_unused struct mms_lock_control *mmslk,
		__maybe_unused struct my_pte_info *mpi,
		__maybe_unused struct snapshot_extras *snapex,
		__maybe_unused struct kprobe *kp)
{

#ifdef DO_PTE_ALT_PROT
	struct folio *folio;
	bool invalid_flags;
	struct list_head *addr_spcs_head;
	DEFINE_INITIATED_RIGHT_NOW();

	DEBUG_PAP_FMT(WREX, "called: addr=%px", (void*) mpi->addr);

	invalid_flags = 
		!ff || 
		(!(ff & FAULT_FLAG_INSTRUCTION) && !(ff & FAULT_FLAG_WRITE)) ||
		((ff & FAULT_FLAG_INSTRUCTION) && (ff & FAULT_FLAG_WRITE));

	DEBUG_PAP_FMT(WREX, "fault_flag infos: invalid=%d, ff=%d, write=%d, instr=%d", 
			invalid_flags, ff, ff & FAULT_FLAG_WRITE, ff & FAULT_FLAG_INSTRUCTION);

	if(unlikely(invalid_flags))
		return;

	folio = __init_collect_aspcs_and_lock(pgs, &addr_spcs_head, mmslk, kp);
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

			DEBUG_PAP_FMT(WREX, "init: pap->write=%d", pgs->pap->write);
			,

			/* on noprot */
			pgs->pap->write = ff & FAULT_FLAG_WRITE;

			DEBUG_PAP_FMT(WREX, "noprot, pap->write=%d", pgs->pap->write);
			,

			/* on regular */
			bool must_alternate;

			must_alternate = 
				(pgs->pap->write && (ff & FAULT_FLAG_INSTRUCTION)) ||
				(!pgs->pap->write && (ff & FAULT_FLAG_WRITE));

			if(must_alternate) {
				DEBUG_PAP_FMT(WREX, "alternate: old pap->write=%d, new pap->write=%d",
						pgs->pap->write, !pgs->pap->write);

				pgs->pap->write = !pgs->pap->write;
			} else {
				DEBUG_PAP_FMT(WREX, "NOT alternating: cur pap->write=%d",
						pgs->pap->write);

				goto __finish;
			}
	);

	do_snapshot(pgs, snapex, ff, initiated_right_now, WREX);

	DEBUG_PAP(WREX, "will walk pte folios:...");

	ptes_walk_from_folio_locked(
			folio, wrex_pte_one, pgs->pap, addr_spcs_head, mpi->ptep, kp);

__finish:
	maybe_mkwrite_mypte(pgs->pap->write, mpi, kp);

	__end_unlock_put_free_aspcs(pgs, &addr_spcs_head, mmslk, folio, kp);

#endif

}

void exonly_ptealtprot(
		__maybe_unused struct page_status *pgs, 
		__maybe_unused struct mms_lock_control *mmslk,
		__maybe_unused struct snapshot_extras *snapex,
		__maybe_unused struct kprobe *kp)
{

#ifdef DO_PTE_ALT_PROT
	struct list_head *addr_spcs_head;
	struct folio *folio;
	DEFINE_INITIATED_RIGHT_NOW();

	DEBUG_PAP(EXONLY, "called");

	folio = __init_collect_aspcs_and_lock(pgs, &addr_spcs_head, mmslk, kp);
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

			DEBUG_PAP_FMT(EXONLY, "init: pap->write=%d", pgs->pap->write);
			,

			/* on noprot */
			/* here we keep write disabled as we are going
			 * to init to allow exec, not write */

			DEBUG_PAP_FMT(EXONLY, "noprot, pap->write=%d", pgs->pap->write);
			,

			/* on regular */
			/* prior call disabled either W or X */
			if(pgs->pap->write) {
				DEBUG_PAP_FMT(EXONLY, "alternate: old pap->write=%d, new pap->write=%d",
						pgs->pap->write, !pgs->pap->write);

				/* ... if disabled X, alternate by disabling W */
				pgs->pap->write = false;
			} else {
				DEBUG_PAP_FMT(EXONLY, "NOT alternating, cur pap->write=%d",
						pgs->pap->write);

				/* ... othw, if disabled W, keep it disabled */
				goto __finish;
			}
	);

	do_snapshot(pgs, snapex, FAULT_FLAG_INSTRUCTION, initiated_right_now, EXONLY);

	DEBUG_PAP(EXONLY, "will walk pte folios:...");

	ptes_walk_from_folio_locked(
			folio, exonly_pte_one, pgs->pap, addr_spcs_head, NULL, kp);

__finish:
	__end_unlock_put_free_aspcs(pgs, &addr_spcs_head, mmslk, folio, kp);
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
		__maybe_unused struct mms_lock_control *mmslk,
		__maybe_unused struct snapshot_extras *snapex,
		__maybe_unused struct kprobe *kp)
{
	bool rv = true;

#ifdef DO_PTE_ALT_PROT
	struct list_head *addr_spcs_head;
	struct folio *folio;
	DEFINE_INITIATED_RIGHT_NOW();

	DEBUG_PAP(NONE, "called");

	folio = __init_collect_aspcs_and_lock(pgs, &addr_spcs_head, mmslk, kp);
	if(!folio) {
		scid_warn("unable to get folio");
		return rv;
	}

	/* if already initiated, don't do anything */
	if(!pgs->pap->init) {
		DEBUG_PAP(NONE, "already inited, returning now");

		rv = false;
		goto __finish;
	}

	DEBUG_PAP_FMT(NONE, "init: pap->write=%d, pap->noprot=%d", 
			pgs->pap->write, pgs->pap->noprot);

	/* this can't be possible, since this is the only
	 * call that does noprot enabling, and disables init
	 * that is, init = true and noprot = true can't be possible
	 */
	BUG_ON(pgs->pap->noprot);
	SET_INITIATED_RIGHT_NOW();

	pgs->pap->init = false;
	pgs->pap->noprot = true;

	DEBUG_PAP(NONE, "init: checks passed, doing noprot on every pte");

	do_snapshot(pgs, snapex, 0, initiated_right_now, NONE);
	
	DEBUG_PAP(NONE, "will walk pte folios:...");

	/* otherwise, zeroprot all the PTEs */
	ptes_walk_from_folio_locked(
			folio, noneprot_pte_one, NULL, addr_spcs_head, NULL, kp);

__finish:
	__end_unlock_put_free_aspcs(pgs, &addr_spcs_head, mmslk, folio, kp);

#endif

	return rv;
}

#ifdef DO_PTE_ALT_PROT

static void __do_pte_fixup(
		pte_t pte, pte_t *ptep, struct vm_area_struct *vma,
		spinlock_t *ptlp, struct page_status *pgs, unsigned long addr)
{
	pte = pte_wrprotect(pte);

	if(pgs->pap->noprot)
		pte = pte_set_flags(pte, _PAGE_NX);
	else {
		if(pgs->pap->write)
			pte = pte_set_flags(pte, _PAGE_NX);
		else {
			if(vma->vm_flags & VM_EXEC)
				pte = pte_mkexec(pte);
		}
	}

	set_pte(ptep, pte);
	spin_unlock(ptlp);
	__scid_flush_tlb_page(vma, addr);
}

#endif /* DO_PTE_ALT_PROT */

/* 
 * the per-VMA or mmap read lock (that protects vma inspection in 
 * pte_fixup_ptealtprot) must be properly held by the caller.
 *
 * Given the usage of this function (change_pte_range's hkrphook)
 * we may risk ***DEADLOCK*** if we do mmap_read_lock(current->mm)
 * on our own. Let the handler do its job.
 */
void pte_fixup_ptealtprot(
		__maybe_unused struct page_status *pgs,
		__maybe_unused struct my_pte_info *mpi,
		__maybe_unused struct kprobe *kp)
{

#ifdef DO_PTE_ALT_PROT
	pte_t pte;

	DEBUG_PAP_FMT(PTE_FIXUP, "called: ptep=%px, vma=%px, ptlp=%px, addr=%px",
			mpi->ptep, mpi->vma, mpi->ptlp, (void*) mpi->addr);

	/* 
	 * the order of lock acquisition is important:
	 *  0) the per-VMA / mmap_read_lock (caller is responsible)
	 *  1) the pap->lock
	 *  2) the page table lock
	 *
	 *  we always do like that, otherwise **DEADLOCK**
	 *
	 *  Recall: user of the subroutine must NOT call us while in
	 *  a critical section involving the page table lock (that is,
	 *  while holding the page table lock) 
	 */
	KPSLEEPABLE(kp,
			mutex_lock(&pgs->pap->lock);
	);

	if(pgs->pap->init) {
		DEBUG_PAP(PTE_FIXUP, "already inited");
		goto __pap_unlock;
	}

	spin_lock(mpi->ptlp);

	pte = ptep_get(mpi->ptep);
	if(INVALID_PTE(pte)) {
		spin_unlock(mpi->ptlp);
		DEBUG_PAP(PTE_FIXUP, "invalid pte");
		goto __pap_unlock;
	}

	__do_pte_fixup(pte, mpi->ptep, mpi->vma, mpi->ptlp, pgs, mpi->addr);

	DEBUG_PAP_FMT(PTE_FIXUP, "fixup: pap->noprot=%d, pap->write=%d, vma has VM_EXEC=%ld",
			pgs->pap->noprot, pgs->pap->write, mpi->vma->vm_flags & VM_EXEC);

__pap_unlock:
	mutex_unlock(&pgs->pap->lock);

#endif

}

#ifdef DO_PTE_ALT_PROT

/* 
 * unluckily, we have to use task_work to run a callback in process context, when
 * thread is about to return to userspace...
 */
struct pte_fixup_sameip_task_work {
	struct callback_head cb_head;
	unsigned long ip;
};

/* tw callback */
static void pte_fixup_sameip_twork(struct callback_head *cb_head)
{
	struct pte_fixup_sameip_task_work *twork = container_of(cb_head, struct pte_fixup_sameip_task_work, cb_head);
	struct page *page;
	pte_t *ptep;
	spinlock_t *ptlp;
	pte_t pte;
	struct page_status *pgs;
	bool pgs_ok;
	struct vm_area_struct *vma;
	unsigned long ip = twork->ip;

	/* free here twork, now, once and for all */
	kfree(twork);

	/* big mmap_lock critical section (othw. we would need to reacquire later and redo vma_lookup?) */
	mmap_read_lock(current->mm);

	/* Do the page table walk to retrieve the ptep and page frame */
	page = user_page_walk_ptep_vma(ip, true, false, &ptep, &ptlp, &vma, NULL);
	if(!page) 
		goto __unlockmm;

	/* Get associated page_status */
	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(page_to_pfn(page));
	pgs_ok = pgs && try_page_status_get(pgs);
	rcu_read_unlock();

	/* if this is true either pgs is NULL or failed to get ref to pgs */
	if(!pgs_ok)
		goto __unlockmm;

	/* mmh... */
	if(!pgs->pap)
		goto __putpgs_unlockmm;

	/* pap->lock critical section */
	mutex_lock(&pgs->pap->lock);

	/* ptl critical section */
	spin_lock(ptlp);

	/* read pte */
	pte = ptep_get(ptep);

	/* Recheck pte under ptl */
	if(INVALID_PTE(pte) || 
			!pte_exec(pte) || !pte_write(pte) || 
			pte_page(pte) != page) {

		spin_unlock(ptlp);
		goto __unlockpap_putpgs_unlockmm;
	}

	/* __do_pte_fixup releases the ptl */
	__do_pte_fixup(pte, ptep, vma, ptlp, pgs, ip);

	/* cleanup... */
__unlockpap_putpgs_unlockmm:
	mutex_unlock(&pgs->pap->lock);
__putpgs_unlockmm:
	page_status_put(pgs);
__unlockmm:
	mmap_read_unlock(current->mm);
}

/* tw cleanup stuff */
static bool __scid_task_work_func_match(struct callback_head *cb, void *data)
{
	return cb->func == data;
}

static struct callback_head *
__scid_task_work_cancel_func(struct task_struct *task, task_work_func_t func)
{
	return THUNK(task_work_cancel_match)(task, __scid_task_work_func_match, func);
}

static void try_to_cancel_tworks(void)
{
	struct task_struct *p, *t;

	rcu_read_lock();
	for_each_process_thread(p, t)
		__scid_task_work_cancel_func(t, pte_fixup_sameip_twork);

	rcu_read_unlock();
}

/* 
 * notifier part of the die_chain, NOT in process context, 
 * we can't do much here other than defer work 
 */
static int pte_fixup_sameip_notify(
		__always_unused struct notifier_block *nb, 
		unsigned long action, void *data)
{
	struct pt_regs *regs;
	unsigned long ip;
	struct pte_fixup_sameip_task_work *twork;

	/* Is this action the one we are looking for? */
	if(action != DIE_DEBUG)
		return NOTIFY_DONE;

	/* Yes, processor thrown #DB exception. */
	regs = ((struct die_args*)data)->regs;

	/* Ensure that when that #DB exception was raised, we were user */
	if(!user_mode(regs))
		return NOTIFY_DONE;

	/* Debug exception caused by trap flag? */
	if(!(regs->flags & X86_EFLAGS_TF))
		return NOTIFY_DONE;

	/* 
	 * Yes, recover the PREVIOUS instruction.
	 *
	 * Refer to Intel SDM, for single stepping,
	 * processor saves ip to the **next** instruction,
	 * we need the previous one: 
	 *
	 * INSTR0 INSTR1 ... INSTRi <SS> INSTR(i+1) ....
	 *                   ^           ^
	 *                   |           |--- processor-saved RIP (regs->ip)
	 *                   |
	 *                   |--------------- what we actually need (orig_ip)
	 *
	 * Furthermore, this answers the question: did we actually
	 * set the single-stepping for current (this) task?
	 */
	if(!orig_ip_task_lookup(&ip))
		return NOTIFY_DONE;

	/* alloc twork, deferred work */
	twork = kmalloc(sizeof(struct pte_fixup_sameip_task_work), GFP_ATOMIC);
	if(!twork) {
		scid_err("memory exhausted");
		return NOTIFY_STOP;
	}

	/* init twork */
	twork->ip = ip;
	init_task_work(&twork->cb_head, pte_fixup_sameip_twork);

	/* 
	 * we need process context at all cost, so defer control when thread
	 * is about to return to userspace
	 */
	if (THUNK(task_work_add)(current, &twork->cb_head, TWA_RESUME)) {
		scid_err("unable to task_work_add");
        kfree(twork);
	}

	/* disable singlestep and STOP anyway, we required "current" to singlestep */
	regs->flags &= ~X86_EFLAGS_TF;
	return NOTIFY_STOP;
}

/* execute the notify handler lastly */
static struct notifier_block pte_fixup_sameip_nb = {
	.notifier_call = pte_fixup_sameip_notify,
	.priority = INT_MIN
};

#endif /* DO_PTE_ALT_PROT */

int setup_ptealtprot(void)
{

#ifdef DO_PTE_ALT_PROT
	unsigned int wq_flgs;

	smp_store_release(&ht_orig_ips_cleanup_dwork.rearm, true);
	INIT_DELAYED_WORK(&ht_orig_ips_cleanup_dwork.dwork, orig_ip_task_cleanup_dwork);
	if(!schedule_delayed_work(&ht_orig_ips_cleanup_dwork.dwork, msecs_to_jiffies(FIVE_MINUTES_MS))) {
		scid_err("unable to schedule_delayed_work");
		return -EBUSY;
	}

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

	register_die_notifier(&pte_fixup_sameip_nb);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)
	wq_flgs = 0;
#else
	wq_flgs = WQ_PERCPU;
#endif

	drop_mm_wq = alloc_workqueue("scid-drop-mm-wq", wq_flgs, 0);
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

	unregister_die_notifier(&pte_fixup_sameip_nb);

	kmem_cache_destroy(pap_cachep);

	smp_store_release(&ht_orig_ips_cleanup_dwork.rearm, false);
	cancel_delayed_work_sync(&ht_orig_ips_cleanup_dwork.dwork);

	try_to_cancel_tworks();
#endif

}
