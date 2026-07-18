#ifndef SCID_RESOLVE_SYMS_WALK_PAGE_RANGE_H
#define SCID_RESOLVE_SYMS_WALK_PAGE_RANGE_H

#include <linux/mm.h>
#include <linux/pagewalk.h>

#include <resolve_syms.h>

#define walk_page_range_SYMPAIR_INDEX 4

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(walk_page_range),

		/* the fn return type */
		int
		, 

		/* symbol name to resolve */
		walk_page_range
		,

		/* ... if symbol cannot be resolved */
		return -1; 
		, 

		/* ... if symbol is resolved */
		return symaddr(mm, start, end, ops, private);
		,

		/* fn args */
		struct mm_struct *mm,
		unsigned long start,
		unsigned long end,
		const struct mm_walk_ops *ops,
		void *private
);

#endif
