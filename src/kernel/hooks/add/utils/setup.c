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

static const struct subsys_regi_args add_tests[] = {
	{
		.name = "add-dap-hook",
		.kvt = {
			ATOMICALLY_INCREMENTED_KEY("entry"),
			ATOMICALLY_INCREMENTED_KEY("zero-page"),
			ATOMICALLY_INCREMENTED_KEY("return-ok"),
			ATOMICALLY_INCREMENTED_KEY("materialize-page"),

			END_OF_KVS
		}
	},
};

#endif 

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(add, kps, krps, add_tests);
