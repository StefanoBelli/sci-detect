#ifndef SCID_RESOLVE_SYMS_PTE_OFFSET_MAP_RW_NOLOCK_H
#define SCID_RESOLVE_SYMS_PTE_OFFSET_MAP_RW_NOLOCK_H

#include <linux/pgtable.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/version.h>

#include <resolve_syms.h>

#define pte_offset_map_rw_nolock_SYMPAIR_INDEX 5

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(pte_offset_map_rw_nolock),

		/* the fn return type */
		pte_t*
		, 

		/* symbol name to resolve */
		pte_offset_map_rw_nolock
		,

		/* ... if symbol cannot be resolved */
		return NULL; 
		, 

		/* ... if symbol is resolved */
		return symaddr(mm, pmd, addr, pmdvalp, ptlp);
		,

		/* fn args */
		struct mm_struct * mm,
		pmd_t *pmd,
		unsigned long addr,
		pmd_t *pmdvalp,
		spinlock_t **ptlp
);

#endif
