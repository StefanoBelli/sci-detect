#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <linux/highmem.h>

#include <pgtrack.h>
#include <logging.h>

static struct kmem_cache *page_snap_cachep;

static enum page_snap_fault build_page_snap_fault(enum fault_flag flags)
{
	if(flags & FAULT_FLAG_WRITE)
		return PAGE_SNAP_WRITE_FAULT;

	if(flags & FAULT_FLAG_INSTRUCTION)
		return PAGE_SNAP_IFETCH_FAULT;

	return PAGE_SNAP_NO_FAULT;
}

#define __checked_dynamic_alloc(__var, __allok_kall, ...) \
	do { \
		if(unlikely(!(__var))) { \
			(__var) = (__allok_kall); \
			if(unlikely(!(__var))) { \
				scid_err("memory exhausted"); \
				goto __unlock; \
			} \
			__VA_ARGS__ \
		} \
	} while(0)

void make_page_snap(struct page_status *pgs, pid_t pid, unsigned long va, enum fault_flag flags)
{
	void *src;

	/* 
	 * critical section may be big, especially on first time,
	 * lets try to keep IRQs enabled for system responsiveness...
	 * Generally the contention on this lock should be low
	 */
	spin_lock(&pgs->snapshot_lock);

	__checked_dynamic_alloc(
			pgs->snapshot, kmem_cache_alloc(page_snap_cachep, GFP_ATOMIC)
			,
			memset(pgs->snapshot, 0, sizeof(struct page_snap));
	);

	__checked_dynamic_alloc(
			pgs->snapshot->buffer, (char*) __get_free_page(GFP_ATOMIC));

	src = kmap_local_page(pgs->page);
	memcpy(pgs->snapshot->buffer, src, PAGE_SIZE);
	kunmap_local(src);

	pgs->snapshot->seq++;
	pgs->snapshot->va = va;
	pgs->snapshot->pid = pid;
	pgs->snapshot->datetime = ktime_get_real_seconds();
	pgs->snapshot->fault = build_page_snap_fault(flags);

__unlock:
	spin_unlock(&pgs->snapshot_lock);
}

#undef __checked_dynamic_alloc

void free_page_snap(struct page_status *pgs)
{
	/* no need to hold the pgs->snapshot_lock here,
	 * in case it comes out it is, remember that 
	 * this is called from a RCU callback (softirq/bh)
	 * LOCKDEP will complain about deadlock risk. _bh
	 * variant of spin_lock_* shall be used in
	 * make_page_snap
	 */

	if(!pgs || !pgs->snapshot)
		return;

	if(unlikely(pgs->snapshot->buffer)) {
		free_page((unsigned long) pgs->snapshot->buffer);
		pgs->snapshot->buffer = NULL;
	}

	kmem_cache_free(page_snap_cachep, pgs->snapshot);
	pgs->snapshot = NULL;
}

int setup_page_snap(void)
{
	page_snap_cachep = kmem_cache_create(
			"page_snap_cache",
			sizeof(struct page_snap),
			0,
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT,
			NULL);

	if(!page_snap_cachep) {
		scid_err("unable to create a new kmem_cache for pgsnap");
		return -ENOMEM;
	}
	
	return 0;
}

void teardown_page_snap(void)
{
	kmem_cache_destroy(page_snap_cachep);
}
