#ifndef SCID_RESOLVE_SYMS_RMAP_WALK_H
#define SCID_RESOLVE_SYMS_RMAP_WALK_H

#include <linux/mm.h>
#include <linux/rmap.h>

#include <resolve_syms.h>

#define rmap_walk_SYMPAIR_INDEX 1

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(rmap_walk),

		/* the fn return type */
		void
		, 

		/* symbol name to resolve */
		rmap_walk
		,

		/* ... if symbol cannot be resolved */
		return; 
		, 

		/* ... if symbol is resolved */
		return symaddr(folio, rwc);
		,

		/* fn args */
		struct folio *folio,
		struct rmap_walk_control *rwc
);

#endif
