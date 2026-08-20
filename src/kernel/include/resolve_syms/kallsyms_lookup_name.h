#ifndef SCID_RESOLVE_SYMS_KALLSYMS_LOOKUP_NAME_H
#define SCID_RESOLVE_SYMS_KALLSYMS_LOOKUP_NAME_H

#include <resolve_syms.h>

#define kallsyms_lookup_name_SYMPAIR_INDEX 5

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(kallsyms_lookup_name),

		/* the fn return type */
		unsigned long
		, 

		/* symbol name to resolve */
		kallsyms_lookup_name
		,

		/* ... if symbol cannot be resolved */
		return 0; 
		, 

		/* ... if symbol is resolved */
		return symaddr(name);
		,

		/* fn args */
		const char* name
);

#endif
