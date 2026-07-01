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

static struct xarray good_pages;
static struct xarray bad_pages;
static struct kmem_cache *page_status_cachep;

int setup_pgtrack(void)
{
	xa_init(&good_pages);
	xa_init(&bad_pages);

	page_status_cachep = kmem_cache_create(
			"page_status_cachep", 
			sizeof(struct page_status), 
			0, 0, NULL);

	if(!page_status_cachep)
		return -ENOMEM;

	return 0;
}

void teardown_pgtrack(void)
{
	unsigned long pfn;
	struct page_status *entry;

	xa_for_each(&good_pages, pfn, entry)
		kmem_cache_free(page_status_cachep, entry);

	kmem_cache_destroy(page_status_cachep);

	xa_destroy(&good_pages);
	xa_destroy(&bad_pages);
}

static inline struct page_status *__lookup_pfn(unsigned long pfn, struct xarray **lookedup_from)
{
	struct page_status *status = NULL;

	status = xa_load(&bad_pages, pfn);
	*lookedup_from = &bad_pages;

	if(!status) {
		status = xa_load(&good_pages, pfn);
		*lookedup_from = &good_pages;
	}

	return status;
}

struct page_status *lookup_pfn_pgtrack(unsigned long pfn)
{
	return xa_load(&good_pages, pfn);
}

struct page_status *lookup_bad_pfn_pgtrack(unsigned long pfn)
{
	return xa_load(&bad_pages, pfn);
}

static inline void __foreach_pgtrack(
		struct xarray *xa, unsigned long start_idx, 
		foreach_pfn_cb cb, void *uargs)
{
	unsigned long cur_idx;
	struct page_status *cur_pgs;

	xa_for_each_start(xa, cur_idx, cur_pgs, start_idx) {
		if(!cb(cur_idx, cur_pgs, uargs))
			return;
	}
}

void foreach_pfn_pgtrack(unsigned long start, foreach_pfn_cb cb, void* args)
{
	__foreach_pgtrack(&good_pages, start, cb, args);
}

void foreach_bad_pfn_pgtrack(unsigned long start, foreach_pfn_cb cb, void* args)
{
	__foreach_pgtrack(&bad_pages, start, cb, args);
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
 * if page doesn't exist in any xarray, don't create a new entry and put
 * it in. This works because if the page that interests the PTE has not yet
 * been inserted in the xarray, this means that it wasn't captured by other
 * "initial lifecycle" hooks (e.g. user page fault) and are of no interest 
 * for us when mprotecting a non-existant page in the xarrays, they will 
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
void pg_track(struct page *page, bool has_write, bool has_exec, bool creat, unsigned long va)
{
	struct xarray *lookedxa;
	unsigned long pfn = page_to_pfn(page);
	struct page_status *pgstatus;

	/* NOT SURE this may be optimized, leave it like this for now...*/
__retry:
	pgstatus = __lookup_pfn(pfn, &lookedxa);

	/* an entry was found */
	if(pgstatus) {

		/* if it is already in the "bad pages", nothing to do */
		if(lookedxa == &bad_pages)
			return;

		/* otherwise, check permissions */
		perm_type new_perms = build_perms(has_write, has_exec);

		/* read the current value of perms */
		perm_type old_perms = atomic64_read(&pgstatus->perms);

		/* we either have nothing to do if new_perms == old_perms or old_perms already 
		 * equals PERM_BITS. This is because perms has been updated but the page_status
		 * didn't get stored yet in bad_pages */
		if(new_perms == old_perms || old_perms == PERM_BITS)
			return;

		/* do an atomic_fetch_or with new perms:
		 *  - if the aof is neq orig then someone changed perms from under us, nothing to do.
		 *  - otherwise, since we previously enforced new_perms to be different from old_perms 
		 *  and old_perms != PERM_BITS then if aof == old_perms then for sure we changed it, and
		 *  we have to do the xa_store eventually.
		 */
		if(atomic64_fetch_or(new_perms, &pgstatus->perms) != old_perms)
			return;

		/* check if we changed value to PERM_BITS, oh well... bad page */
		if(atomic64_read(&pgstatus->perms) == PERM_BITS) {
			bcast_pgtrack_event_wxwarning(pfn, task_pid_vnr(current), va);
			int err = xa_insert(&bad_pages, pfn, pgstatus, GFP_ATOMIC);
			__pgtrack_log_err_code(err);
		}
	
	/* unable to find any entry, we must create it */
	} else {
		/* this works because... see notes above */
		if(!creat)
			return;

		/* create new page_status, not visible until xa_inserted */
		struct page_status *new_pgstatus = kmem_cache_alloc(page_status_cachep, GFP_ATOMIC);
		if(!new_pgstatus) {
			__pgtrack_log_err_code(-ENOMEM);
			return;
		}

		new_pgstatus->page = page;
		atomic64_set(&new_pgstatus->perms, 0);

		/* visible from now on */
		int err = xa_insert(&good_pages, pfn, new_pgstatus, GFP_ATOMIC);

		/* someone xa_inserted before us... retry, adjust permissions, 
		 * but free our private allocated memory */
		if(err) {
			kmem_cache_free(page_status_cachep, new_pgstatus);
			if(err == -EBUSY)
				goto __retry;
			
			__pgtrack_log_err_code(err);

		/* we published it correctly, retry to adjust permissions */
		} else {
			/* new_pgstatus is held */
			kref_init(&new_pgstatus->kref);
			goto __retry;
		}
	}
}

/* this is way simpler than the pg_track function, and it should be ok.
 *
 * Why? This is called from one place only (free_unref_folios hook),
 * the folio/page has reached refcount of 0 (nobody is using it) and
 * page is still not returned to the buddy allocator. This means that
 * the page won't be used again, until freed. So we can do the 2-steps 
 * xa_erase """non-atomically"""
 *
 * Also refer to the pg_track comment.
 */
bool pg_untrack(struct page *page)
{
	unsigned long pfn = page_to_pfn(page);
	struct page_status *pgstatus;

	pgstatus = xa_erase(&good_pages, pfn);
	if(!pgstatus)
		return false;

	xa_erase(&bad_pages, pfn);

	/* we may put this before this last xa_erase... */
	page_status_put(pgstatus);

	return true;
}

static void __page_status_free_rcuh_fn(struct rcu_head *rcu)
{
	struct page_status *pgs = container_of(rcu, struct page_status, rcu);

	kmem_cache_free(page_status_cachep, pgs);
}

void __page_status_release_fn(struct kref *kref)
{
	struct page_status *pgs = container_of(kref, struct page_status, kref);

	call_rcu(&pgs->rcu, __page_status_free_rcuh_fn);
}
