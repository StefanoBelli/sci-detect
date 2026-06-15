#ifndef SCID_RESOLVE_SYMS_PTE_OFFSET_MAP_LOCK_H
#define SCID_RESOLVE_SYMS_PTE_OFFSET_MAP_LOCK_H

#include <linux/pgtable.h>
#include <linux/spinlock.h>
#include <linux/mm.h>

#include <resolve_syms.h>

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(pte_offset_map_lock),

		/* the fn return type */
		pte_t*
		, 

		/* symbol name to resolve */
		pte_offset_map_lock
		,

		/* ... if symbol cannot be resolved */
		return NULL; 
		, 

		/* ... if symbol is resolved */
		return symaddr(mm, pmd, addr, ptlp);
		,

		/* fn args */
		struct mm_struct * mm,
		pmd_t *pmd,
		unsigned long addr,
		spinlock_t **ptlp
);

#endif
