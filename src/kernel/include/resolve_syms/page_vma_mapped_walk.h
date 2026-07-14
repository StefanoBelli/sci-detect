#ifndef SCID_RESOLVE_SYMS_PAGE_VMA_MAPPED_WALK_H
#define SCID_RESOLVE_SYMS_PAGE_VMA_MAPPED_WALK_H

#include <linux/mm.h>
#include <linux/rmap.h>

#include <resolve_syms.h>

#define page_vma_mapped_walk_SYMPAIR_INDEX 3

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(page_vma_mapped_walk),

		/* the fn return type */
		bool
		, 

		/* symbol name to resolve */
		page_vma_mapped_walk
		,

		/* ... if symbol cannot be resolved */
		return false; 
		, 

		/* ... if symbol is resolved */
		return symaddr(pvmw);
		,

		/* fn args */
		struct page_vma_mapped_walk *pvmw
);

#endif
