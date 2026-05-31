#include <hooks/setuputils.h>

#include "../hooks.h"

static struct kretprobe *krps[] = {
	&handle_pte_fault__krp,
	&do_anonymous_page__krp,
	&wp_page_copy__krp,
	&do_wp_page__krp,
	&set_pte_range__krp,
};

static struct kprobe *kps[] = {
	&do_fault__kp,
	&finish_fault__kp,
	&filemap_map_pages__kp,
	&finish_mkwrite_fault__kp,
};

#ifdef SCID_CONFIG_TESTING

#include <testing/default-kvops.h>

static struct subsys_regi_args add_tests[] = {
	{
		.name = "add-dap-hook",
		.kvt_len = 1,
		.kvt = {
			.key = "success",
			.value_size = sizeof(int),
			.kv_ops = {
				.init_value = atomic_inc_init_kvop,
				.set_value = atomic_inc_set_kvop,
				.reset_value = atomic_inc_reset_kvop,
				.uquery_value = atomic_inc_uquery_kvop
			}
		}
	},
};

#endif 

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(add, kps, krps, add_tests);
