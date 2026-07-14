#include <linux/slab.h>
#include <linux/xarray.h>
#include <linux/mm.h>
#include <linux/types.h>

#include <pgtrack.h>
#include <netlink/pgtrack/events.h>
#include <logging.h>

#define __pgtrack_log_err_code(err)  \
	do { \
		int myerr = (err); \
		if(myerr == -EBUSY) \
			scid_err("double store of same pfn, this may be a bug"); \
		else if(myerr == -ENOMEM) \
			scid_err("memory exhausted"); \
		else if(myerr) \
			scid_errf("unknown error: %d", err); \
	} while(0)

#define build_perms(w, e) \
({ \
	 perm_type perm = 0; \
	 \
	 if((w)) \
	 	perm = PERM_WRITE_BIT; \
	 \
	 if((e)) \
	 	perm |= PERM_EXEC_BIT; \
	 \
	 perm; \
})

static struct xarray pages;
static struct kmem_cache *page_status_cachep;
static struct kmem_cache *page_wxwarn_cachep;

static void free_pgs(struct page_status *pgs);

int setup_pgtrack(void)
{
	xa_init(&pages);

	page_status_cachep = kmem_cache_create(
			"scid__page_status_cache", 
			sizeof(struct page_status), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);

	if(!page_status_cachep)
		return -ENOMEM;

	page_wxwarn_cachep = kmem_cache_create(
			"scid__page_wxwarn_cache",
			sizeof(struct page_wxwarn),
			0,
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT,
			NULL);

	if(!page_wxwarn_cachep) {
		kmem_cache_destroy(page_status_cachep);
		return -ENOMEM;
	}

	return 0;
}

/* 
 * no need to do any kind of synchronization here,
 * fixed the initialization and teardown sequence,
 * when this gets executed no more netlink cmds or
 * hooks can be run (we can't advance page perm 
 * state, so we don't need to do any new bcast) 
 * or are in-flight.
 */

void teardown_pgtrack(void)
{
	unsigned long pfn;
	struct page_status *entry;

	xa_for_each(&pages, pfn, entry)
		free_pgs(entry);

	kmem_cache_destroy(page_status_cachep);
	kmem_cache_destroy(page_wxwarn_cachep);
	xa_destroy(&pages);
}

static inline struct page_wxwarn *new_page_wxwarn(
		unsigned long pfn, unsigned long va, pid_t pid)
{
	struct page_wxwarn *wxw;

	wxw = kmem_cache_alloc(page_wxwarn_cachep, GFP_ATOMIC);
	if(!wxw) {
		scid_err("memory exhausted");
		return NULL;
	}

	wxw->pfn = pfn;
	wxw->pid = pid;
	wxw->va = va;

	return wxw;
}

void del_page_wxwarn(struct page_wxwarn *wxw)
{
	kmem_cache_free(page_wxwarn_cachep, wxw);
}

static void free_pgs(struct page_status *pgs)
{
	free_page_snap_from_pgs(pgs);
	free_ptealtprot(pgs);
	kmem_cache_free(page_status_cachep, pgs);
}

struct page_status *lookup_pfn_pgtrack(unsigned long pfn)
{
	return xa_load(&pages, pfn);
}

void foreach_pfn_pgtrack(unsigned long start, foreach_pfn_cb cb, void* args)
{
	unsigned long cur_idx;
	struct page_status *cur_pgs;

	xa_for_each_start(&pages, cur_idx, cur_pgs, start) {
		if(!cb(cur_idx, cur_pgs, args))
			return;
	}
}

/*
 * ---WHY NO KREF OR RCU IN pg_track CODE?---
 *
 *  * pg_track is being used by every hook that adds/changes page state,
 * all those hooks are called in process context, and it is either 
 * in the page fault handler or a normal syscall execution.
 *
 *  * pg_untrack is used *only* by the free_unref_folio hook, same 
 * preconditions.
 *
 * When a batch of folios reaches usage counter = 0, free_unref_folios
 * is called. THIS CAN'T HAPPEN WHILE the "pg_track hooks" are being
 * run, why? Well, if hooks are currently executing, this means that
 * process is currently in kernel mode (whether it is a entry handler 
 * or not, or whatever type of k[ret]probe), and doing stuff regarding
 * the mapping of page frames. 
 *
 * Hence, at least one user for the folio is there and can't be freed.
 * If "pg_track hooks" are in execution, process didn't call munmap()
 * and will not, until we exit to user mode, so we have at least 1 user
 * and no free_unref_folios will be called while doing operations on the
 * xarrays with pg_track. REMEMBER: THE ISSUE ***WOULD*** BE WITH FREEING
 * MEMORY, AND NOT "CONCURRENCY".
 *
 * pte_offset_map_lock protects PTEs (see PTE split locks): suppose
 * we have two threads: A and B of one process (that is, sharing the same
 * address space)
 *
 * RECALL: pg_track is called with PTE split lock held by calling thread.
 *
 * A enters the page fault handler, regarding page P.
 * While B, in parallel does a munmap on that specific page P.
 *
 * Since the various kprobes in the PF handler ensure that they have the
 * PTE lock, **and** zap_pte_range acquries that same lock we can substantially
 * have 2 situations:
 *
 *  - thread A gets the lock first, the PTE is still valid, tracks page successfully,
 *  when thread B, later on, gets the lock, it invalidates the PTE and decrements the
 *  folio refcount, in the end, free_unref_folios will get called, causing the page
 *  untracking
 *
 *  -thread B gets the lock first, does zap the PTE entry. Thread A then gets the lock and
 *  sees the PTE entry being invalid, gives up, no page tracking happens.
 *
 * ---ON THE CREAT FLAG---
 *
 * every hook that uses pg_track will set creat = true, but the hook that
 * hooks into change_pte_range: that will set creat = false, that is,
 * if page doesn't exist in the xarray, don't create a new entry and put
 * it in. This works because if the page that interests the PTE has not yet
 * been inserted in the xarray, this means that it wasn't captured by other
 * "initial lifecycle" hooks (e.g. user page fault) and are of no interest 
 * for us when mprotecting a non-existant page in the xarray, they will 
 * get a chance when returned to the buddy allocator...
 *
 * ---ONE MORE THING---
 *
 * Furthermore, when free_unref_folios is called, those folios are still
 * not returned to the zoned/buddy allocator. That is, no one will try to
 * manipulate PTEs pointing to those pages, because no one will be able 
 * to use them until they actually get freed (see kernel source)
 * so we can safely do the pg_untrack without worries.
 */
void pg_track(struct page *page, bool has_write, bool has_exec, 
		bool creat, unsigned long va, enum fault_flag flags)
{
	unsigned long pfn = page_to_pfn(page);
	struct page_status *pgs;

__retry:
	pgs = xa_load(&pages, pfn);
	if(!pgs) {
		if(!creat)
			return;

		/* create new page_status, not visible until xa_inserted */
		struct page_status *new_pgs = kmem_cache_alloc(page_status_cachep, GFP_ATOMIC);
		if(!new_pgs) {
			__pgtrack_log_err_code(-ENOMEM);
			return;
		}

		/* init */
		new_pgs->page = page;
		atomic64_set(&new_pgs->perms, 0);
		kref_init(&new_pgs->kref);

#ifndef DISABLE_PAGE_SNAPSHOT
		new_pgs->snapshot = NULL;
		spin_lock_init(&new_pgs->snapshot_lock);
#endif /* DISABLE_PAGE_SNAPSHOT */

		/* try to publish it */
		int err = xa_insert(&pages, pfn, new_pgs, GFP_ATOMIC);

		/* some error happened, let's check */
		if(err) {
			free_pgs(new_pgs);

			/* someone published same pfn under us, retry lookup */
			if(err == -EBUSY)
				goto __retry;

			/* whatever other error happened, log it and return now */
			__pgtrack_log_err_code(err);
			return;
		}
		
		/* we published it! */
		pgs = new_pgs;
	}

	perm_type new_perms = build_perms(has_write, has_exec);

	perm_type old_perms = atomic64_fetch_or(new_perms, &pgs->perms);
	if(old_perms == PERM_BITS)
		return;

	/* one thread only will get to execute this */
	if((new_perms | old_perms) == PERM_BITS) {
		pid_t pid = task_pid_vnr(current);
		struct page_wxwarn *wxw = new_page_wxwarn(pfn, va, pid);
		if(!wxw)
			/* fallback to dmesg */
			scid_warnf("WXWARN for pfn=%ld, va=%ld, pid=%d", pfn, va, pid);
		else {
			bcast_pgtrack_event_wxwarning(wxw);
			make_page_snap(pgs, pid, pfn, va, flags);
			new_ptealtprot(pgs);
		}
	}
}

/* this is way simpler than the pg_track function, and it should be ok.
 *
 * Why? This is called from one place only (free_unref_folios hook),
 * the folio/page has reached refcount of 0 (nobody is using it) and
 * page is still not returned to the buddy allocator. This means that
 * the page won't be used again, until freed.
 *
 * Also refer to the pg_track comment.
 */
bool pg_untrack(struct page *page)
{
	unsigned long pfn = page_to_pfn(page);
	struct page_status *pgstatus;

	pgstatus = xa_erase(&pages, pfn);
	if(!pgstatus)
		return false;

	/* we may put this before this last xa_erase... */
	page_status_put(pgstatus);

	return true;
}

static void __page_status_free_rcuh_fn(struct rcu_head *rcu)
{
	struct page_status *pgs = container_of(rcu, struct page_status, rcu);
	free_pgs(pgs);
}

void __page_status_release_fn(struct kref *kref)
{
	struct page_status *pgs = container_of(kref, struct page_status, kref);
	call_rcu(&pgs->rcu, __page_status_free_rcuh_fn);
}
