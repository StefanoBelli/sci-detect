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
static struct subsys_regi_args add_tests[] = {
	{
		.name = "key",
		.kvt_len = 1,
		.kvt = {
			.key = "real-key",
			.value_size = sizeof(int),
			.kv_ops = {
				.uquery_value = NULL,
				.set_value = NULL
			}
		}
	},
	{
		.name = "key1",
		.kvt_len = 1,
		.kvt = {
			.key = "real-key",
			.value_size = sizeof(int),
			.kv_ops = {
				.uquery_value = NULL,
				.set_value = NULL
			}
		}
	},
};
#endif 

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(add, kps, krps, add_tests);
