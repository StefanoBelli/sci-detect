#ifndef SCID_RESOLVE_SYMS_FLUSH_TLB_MM_RANGE_H
#define SCID_RESOLVE_SYMS_FLUSH_TLB_MM_RANGE_H

#include <linux/mm.h>

#include <resolve_syms.h>

#define flush_tlb_mm_range_SYMPAIR_INDEX 2

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(flush_tlb_mm_range),

		/* the fn return type */
		void
		, 

		/* symbol name to resolve */
		flush_tlb_mm_range
		,

		/* ... if symbol cannot be resolved */
		return; 
		, 

		/* ... if symbol is resolved */
		return symaddr(mm, start, end, stride_shift, freed_tables);
		,

		/* fn args */
		struct mm_struct *mm,
		unsigned long start,
		unsigned long end,
		unsigned int stride_shift,
		bool freed_tables
);

#endif
