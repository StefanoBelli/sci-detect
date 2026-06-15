#include <linux/kprobes.h>
#include <resolve_syms.h>

#define INIT_SYMPAIR(_sym) \
	[sympair_nr(_sym)] = { \
		.addr = NULL, \
		.sym = #_sym, \
	}

struct sympair sp[NR_SYMPAIRS] = {
	INIT_SYMPAIR(pte_offset_map_lock)
};

#undef INIT_SYMPAIR

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
				return -1;
			}
		}
	}

	return 0;
}

