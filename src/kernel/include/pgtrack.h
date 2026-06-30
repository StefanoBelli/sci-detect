#ifndef SCID_PGTRACK_H
#define SCID_PGTRACK_H

#include <linux/mm_types.h>
#include <linux/kref.h>
#include <linux/rcupdate.h>

/**
 * you may not always need to both define a RCU
 * critical section + increment usage counter.
 *
 *  --> In most of the cases, only defining a RCU
 * critical section is enough, for example
 * when reading data "on-the-fly".
 *
 *  --> But if you need to "carry-around" the
 * descriptor, you need to, as soon as possible,
 * terminate the RCU critical section, and 
 * before doing that, inc. the refcount.
 *
 * -----------------RCU + kref example---------------------
 * rcu_read_lock();
 * pgs = lookup_pfn_pgstatus(pfn);
 * if(pgs && !try_page_status_get(pgs)) {
 *  the pgs refcount got down to 0,
 *  but thanks to the RCU critical section
 *  you don't risk a use-after-free (UAF) bug.
 * } else {
 *  use your thing (e.g. transfer to deferred work)
 *  Don't forget to page_status_put(pgs) when done
 * }
 * rcu_read_unlock();
 *
 * -----------------RCU only example------------------------
 * rcu_read_lock();
 * pgs = lookup_pfn_pgstatus(pfn);
 * if(pgs)
 *  copy_perms(dest, pgs);
 * rcu_read_unlock();
 */
struct page_status {
	struct page *page;
	atomic64_t perms;

	struct kref kref;
	struct rcu_head rcu;
};

#define PERM_WRITE_BIT 1
#define PERM_EXEC_BIT 2
#define PERM_BITS 3

typedef s64 perm_type;

void __page_status_release_fn(struct kref *);

/**
 * try_page_status_get - try to increase page_status's refcount.
 * You must do this while in a rcu critical section.
 *
 * Returns: true if refcnt incremented, false otherwise
 */
static inline bool try_page_status_get(struct page_status* pgs)
{
	return kref_get_unless_zero(&pgs->kref);
}

/**
 * page_status_put - decrement page_status's refcount.
 */
static inline void page_status_put(struct page_status* pgs)
{
	kref_put(&pgs->kref, __page_status_release_fn);
}

/**
 * lookup_pfn_pgtrack - lookup a pfn and get its descriptor
 * Define a RCU critical section around this call
 * 
 * @pfn: the pfn to lookup for
 *
 * Returns: the page_status descriptor or NULL
 */
struct page_status *lookup_pfn_pgtrack(unsigned long pfn);

/**
 * setup_pgtrack - setup page tracking
 *
 * Returns: 0 if ok, -1 otherwise
 */
int setup_pgtrack(void);

/**
 * teardown_pgtrack - teardown page tracking
 */
void teardown_pgtrack(void);

/**
 * pg_track - track a page
 *
 * This never fails: if a page is not tracked, we start tracking it,
 * if a page is already being tracked, we increment permissions (if needed)
 *
 * Concurrency handled internally.
 *
 * Could be used in atomic context, but we need current.
 *
 * Ensure this is called with the PTE lock taken.
 *
 * @page: the page
 * @has_write: is it a write-enabled page?
 * @has_exec: is it a exec-enabled page?
 * @creat: create new entry if it doesn't exist?
 * @va: the starting va that caused state change (via fault or whatever)
 *
 */
void pg_track(struct page *page, bool has_write, bool has_exec, bool creat, unsigned long va);

/**
 * pg_untrack - stop tracking a page, use carefully
 *
 * @page: the page
 *
 * Concurrency handled internally. Can be used in atomic context.
 *
 * Returns: true if the page was removed, false otherwise 
 */
bool pg_untrack(struct page *page);

#endif
