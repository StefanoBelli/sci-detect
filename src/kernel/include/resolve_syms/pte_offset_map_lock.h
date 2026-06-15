#ifndef SCID_RESOLVE_SYMS_PTE_OFFSET_MAP_LOCK_H
#define SCID_RESOLVE_SYMS_PTE_OFFSET_MAP_LOCK_H

#include <linux/pgtable.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/version.h>

#include <resolve_syms.h>

/*
 * versions prior to 7.0.0 had a wrapper around the core
 * __pte_offset_map_lock (due to static analysis, nothing we care about)
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(7,0,0)
#	define pte_offset_map_lock __pte_offset_map_lock
#	define __pte_offset_map_lock_SYMPAIR_INDEX 0
#else
#	define pte_offset_map_lock pte_offset_map_lock
#	define pte_offset_map_lock_SYMPAIR_INDEX 0
#endif

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
