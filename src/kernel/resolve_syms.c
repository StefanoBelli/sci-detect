#include <linux/kprobes.h>

#include <resolve_syms.h>

#include <resolve_syms/pte_offset_map_lock.h>
#include <resolve_syms/rmap_walk.h>
#include <resolve_syms/page_vma_mapped_walk.h>
#include <resolve_syms/flush_tlb_mm_range.h>
#include <resolve_syms/walk_page_range.h>
#include <resolve_syms/task_work_add.h>

#ifdef CONFIG_KALLSYMS
#	include <resolve_syms/kallsyms_lookup_name.h>
#endif

#define __expand_tos(x) #x

#define __INIT_SYMPAIR(_sym, _dontfail) \
	[sympair_nr(_sym)] = { \
		.addr = NULL, \
		.sym = __expand_tos(_sym), \
		.dontfail = (_dontfail), \
	}

#define INIT_SYMPAIR(_sym) \
	__INIT_SYMPAIR(_sym, false)

#define INIT_SYMPAIR_DONTFAIL(_sym) \
	__INIT_SYMPAIR(_sym, true)

struct sympair sp[NR_SYMPAIRS] = {
	INIT_SYMPAIR(pte_offset_map_lock),
	INIT_SYMPAIR(rmap_walk),
	INIT_SYMPAIR(flush_tlb_mm_range),
	INIT_SYMPAIR(page_vma_mapped_walk),
	INIT_SYMPAIR(walk_page_range),

#ifdef CONFIG_KALLSYMS
	INIT_SYMPAIR_DONTFAIL(kallsyms_lookup_name),
#endif

	INIT_SYMPAIR(task_work_add),
};

#undef INIT_SYMPAIR
#undef __expand_tos

/* prototype */
void *resolve_sym(const char*);

void *resolve_sym(const char* sym)
{
	WARN_ON(in_atomic());

	struct kprobe kp;
	void *resolved_addr;

	memset(&kp, 0, sizeof(kp));

	kp.symbol_name = sym;
	register_kprobe(&kp);

	resolved_addr = kp.addr;

	unregister_kprobe(&kp);
	return resolved_addr;
}

int setup_resolve_all_syms(void)
{
	for(size_t i = 0; i < NR_SYMPAIRS; i++) {
		if(sp[i].sym) {
			sp[i].addr = resolve_sym(sp[i].sym);
			if(!sp[i].addr) {
				scid_errf("unable to resolve %s", sp[i].sym);
				if(!sp[i].dontfail)
					return -ENODATA;
			}
		}
	}

	return 0;
}

