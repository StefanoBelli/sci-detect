#ifndef SCID_PGTRACK_H
#define SCID_PGTRACK_H

#include <linux/mm_types.h>

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
 * Concurrency handled internally. Can be used in atomic context
 *
 * @page: the page
 * @has_write: is it a write-enabled page?
 * @has_exec: is it a exec-enabled page?
 */
void pg_track(struct page *page, bool has_write, bool has_exec);

/**
 * pg_untrack - stop tracking a page, use carefully
 *
 * @page: the page
 *
 * Concurrency handled internally. Can be used in atomic context.
 *
 * Returns: true if the page was removed, false otherwise 
 * (didn't exist, check your code)
 */
bool pg_untrack(struct page *page);

#endif
