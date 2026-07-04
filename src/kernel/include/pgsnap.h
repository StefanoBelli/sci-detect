#ifndef SCID_PGSNAP_H
#define SCID_PGSNAP_H

#include <linux/types.h>
#include <linux/mm_types.h>
#include <linux/time64.h>

/* fwd decl, see pgtrack.h header */
struct page_status;

/* why the snapshot happened? */
enum page_snap_fault : u32 {
	PAGE_SNAP_NO_FAULT,
	PAGE_SNAP_WRITE_FAULT,
	PAGE_SNAP_IFETCH_FAULT,
};

/* snapshot info */
struct page_snap {
	char *buffer;
	unsigned long va;
	time64_t datetime;
	u64 seq;
	unsigned long pfn;
	pid_t pid;
	enum page_snap_fault fault;
};

/**
 * make_page_snap - make the snapshot of a page, known its page_status descriptor
 * and fault reason.
 *
 * Takes care of doing the snapshot, broadcast it, and correct update of the
 * page_status descriptor's "snapshot" field. If pgs's "snapshot" field is NULL,
 * it creates one.
 *
 * Can be run in atomic context.
 *
 * This is called essentially:
 *  - from the page fault handler hook if a wx-page is involved
 *  - from the pg_track function when state machine for a page reaches wx status
 *  and flags are forwarded by the pg_track caller
 *
 * @pgs: the page_status descriptor
 * @pid: the pid
 * @pfn: the pfn
 * @va: the va
 * @flags: the "flags" in struct vm_fault, or 0 if not from a fault 
 */
void make_page_snap(
		struct page_status *pgs, pid_t pid, 
		unsigned long pfn, unsigned long va, enum fault_flag flags);

/**
 * free_page_snap_from_pgs - free pgs' snapshot field
 *
 * Don't try to do this on your own, we know how to do this :)
 *
 * You may also pass pgs = NULL, we check for it.
 */
void free_page_snap_from_pgs(struct page_status *pgs);

/**
 * del_page_snap - free page_snap
 *
 * @snap: the page_snap
 */
void del_page_snap(struct page_snap *snap);

/**
 * setup_page_snap - setup page snapshot mechanism
 */
int setup_page_snap(void);

/**
 * teardown_page_snap - teardown page snapshot mechanism
 */
void teardown_page_snap(void);

#endif
