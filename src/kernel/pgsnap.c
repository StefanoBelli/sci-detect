#include <linux/compiler.h>

#ifndef DISABLE_PAGE_SNAPSHOT
#	include <linux/slab.h>
#	include <linux/uaccess.h>
#	include <linux/timekeeping.h>
#	include <linux/highmem.h>

#	include <pgtrack.h>
#	include <logging.h>
#	include <netlink/pgtrack/events.h>
#else
#	include <pgsnap.h>
#endif /* DISABLE_PAGE_SNAPSHOT */

#ifndef DISABLE_PAGE_SNAPSHOT

static struct kmem_cache *page_snap_cachep;

#define build_page_snap_fault(flags) \
	({ \
	 	enum page_snap_fault _fault; \
	 	\
	 	if((flags) & FAULT_FLAG_WRITE) \
	 		_fault = PAGE_SNAP_WRITE_FAULT; \
	 	else if((flags) & FAULT_FLAG_INSTRUCTION) \
	 		_fault = PAGE_SNAP_IFETCH_FAULT; \
	 	else \
	 		_fault = PAGE_SNAP_NO_FAULT; \
	 	\
	 	_fault; \
	 })

static struct page_snap* new_page_snap(bool zeroed_snapobj)
{
	struct page_snap *snap;

	snap = kmem_cache_alloc(page_snap_cachep, GFP_ATOMIC);
	if(unlikely(!snap)) {
		scid_err("memory exhausted");
		return NULL;
	}

	if(unlikely(zeroed_snapobj))
		memset(snap, 0, sizeof(struct page_snap));

	snap->buffer = (char*) get_zeroed_page(GFP_ATOMIC);
	if(unlikely(!snap->buffer)) {
		kmem_cache_free(page_snap_cachep, snap);
		scid_err("memory exhausted");
		return NULL;
	}

	return snap;
}

#endif /* DISABLE_PAGE_SNAPSHOT */

void make_page_snap(
		__maybe_unused struct page_status *pgs, 
		__maybe_unused pid_t pid, 
		__maybe_unused unsigned long pfn, 
		__maybe_unused unsigned long va, 
		__maybe_unused enum fault_flag flags)
{

#ifndef DISABLE_PAGE_SNAPSHOT
	void *src;
	struct page_snap *my_snap;
	bool ok = true;

	my_snap = new_page_snap(false);
	if(!my_snap)
		return;

	/* 
	 * critical section may be big, especially on first time,
	 * lets try to keep IRQs enabled for system responsiveness...
	 * Generally the contention on this lock should be low
	 */
	spin_lock(&pgs->snapshot_lock);

	if(!pgs->snapshot) {
		pgs->snapshot = new_page_snap(true);
		if(!pgs->snapshot) {
			ok = false;
			goto __unlock;
		}
	}

	/* some copies are needed, hot hw caches
	 *
	 * 2 * PAGE_SIZE + 6 * sizeof(u64) = 8240 B
	 *
	 * Considering x86-64 and that structs are padded (processor word)
	 */
	src = kmap_local_page(pgs->page);

	memcpy(pgs->snapshot->buffer, src, PAGE_SIZE);
	memcpy(my_snap->buffer, pgs->snapshot->buffer, PAGE_SIZE);

	kunmap_local(src);

	pgs->snapshot->seq++;
	pgs->snapshot->va = va;
	pgs->snapshot->pid = pid;
	pgs->snapshot->datetime = ktime_get_real_seconds();
	pgs->snapshot->fault = build_page_snap_fault(flags);
	pgs->snapshot->pfn = pfn;

	my_snap->seq = pgs->snapshot->seq;
	my_snap->va = pgs->snapshot->va;
	my_snap->pid = pgs->snapshot->pid;
	my_snap->datetime = pgs->snapshot->datetime;
	my_snap->fault = pgs->snapshot->fault;
	my_snap->pfn = pgs->snapshot->pfn;

	/* gurantee strict ordering of broadcasting,
	 * this is fast deferred work!! CS ends very soon...
	 * 
	 * transfer "local" snap obj to work
	 */
	bcast_pgtrack_event_snapshot(my_snap);

__unlock:
	spin_unlock(&pgs->snapshot_lock);

	/* if something doesn't go well, we will need
	 * to take care of page_snap resource freeing
	 */
	if(!ok)
		del_page_snap(my_snap);
#endif /* DISABLE_PAGE_SNAPSHOT */

}

#ifndef DISABLE_PAGE_SNAPSHOT
#	undef build_page_snap_fault
#endif /* DISABLE_PAGE_SNAPSHOT */

#ifndef DISABLE_PAGE_SNAPSHOT

void del_page_snap(struct page_snap *snap)
{

	free_page((unsigned long) snap->buffer);
	snap->buffer = NULL;
	kmem_cache_free(page_snap_cachep, snap);

}

#endif /* DISABLE_PAGE_SNAPSHOT */

/* no need to hold the pgs->snapshot_lock here,
 * in case it comes out it is, remember that 
 * this is called from a RCU callback that may be run
 * from bottom half (bh) context (softirq)
 *
 * So, LOCKDEP will complain about deadlock risk. _bh
 * variant of spin_lock_* shall be used in
 * make_page_snap to disable bh execution while in the
 * make_page_snap critical section
 */
void free_page_snap_from_pgs(__maybe_unused struct page_status *pgs)
{

#ifndef DISABLE_PAGE_SNAPSHOT
	if(!pgs || !pgs->snapshot)
		return;

	del_page_snap(pgs->snapshot);
	pgs->snapshot = NULL;
#endif /* DISABLE_PAGE_SNAPSHOT */

}

int setup_page_snap(void)
{

#ifndef DISABLE_PAGE_SNAPSHOT
	page_snap_cachep = kmem_cache_create(
			"scid__page_snap_cache",
			sizeof(struct page_snap),
			0,
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT,
			NULL);

	if(!page_snap_cachep) {
		scid_err("unable to create a new kmem_cache for pgsnap");
		return -ENOMEM;
	}
#endif /* DISABLE_PAGE_SNAPSHOT */

	return 0;
}

void teardown_page_snap(void)
{

#ifndef DISABLE_PAGE_SNAPSHOT
	kmem_cache_destroy(page_snap_cachep);
#endif /* DISABLE_PAGE_SNAPSHOT */

}
