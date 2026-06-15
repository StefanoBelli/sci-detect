#ifndef SCID_RESOLVE_SYMS_H
#define SCID_RESOLVE_SYMS_H

#define sympair_nr(sym) \
	sym##_SYMPAIR_INDEX

#include <logging.h>

struct sympair {
	const char* sym;
	void* addr;
};

#define NR_SYMPAIRS 256

extern struct sympair sp[NR_SYMPAIRS];

#define FPTR_TYPE(sym) __scid_sym_##sym##_fptype
#define THUNK(sym) __scid_resolved__##sym

#define DEFINE_RESOLVED_THUNK(symnr, rvtype, sym, on_error, on_sym_resolved, ...) \
	\
	typedef rvtype (*FPTR_TYPE(sym))(__VA_ARGS__); \
	\
	static inline rvtype THUNK(sym)(__VA_ARGS__); \
	\
	static inline rvtype THUNK(sym)(__VA_ARGS__) \
	{ \
		FPTR_TYPE(sym) symaddr = ((FPTR_TYPE(sym)) sp[(symnr)].addr); \
		if(!symaddr) { \
			scid_err(#sym " needs to be resolved: " \
					"will *not* do this now, fix your code"); \
			on_error \
		} \
		\
		on_sym_resolved \
	}

int setup_resolve_all_syms(void);

#endif
