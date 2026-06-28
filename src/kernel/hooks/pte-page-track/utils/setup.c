#include <hooks/setuputils.h>

#include "../hooks.h"

static struct kretprobe *krps[] = {
	&handle_pte_fault__krp,
	&do_anonymous_page__krp,
	&wp_page_copy__krp,
	&do_wp_page__krp,
	&set_pte_range__krp,
	&change_pte_range__krp,
};

static struct kprobe *kps[] = {
	&do_fault__kp,
	&finish_fault__kp,
	&filemap_map_pages__kp,
	&finish_mkwrite_fault__kp,
	&free_unref_folios__kp,

#ifdef SCID_CONFIG_TESTING
	&free_pages_and_swap_cache__kp,
#endif

};

#ifdef SCID_CONFIG_TESTING

#include <testing/default-kvops.h>

static const struct subsys_regi_args ppt_suts[] = {
	{
		.name = "pte-page-track-dap-hook",
		.kvt = {
			ATOMICALLY_INCREMENTED_KEY("entry"),
			ATOMICALLY_INCREMENTED_KEY("zero-page"),
			ATOMICALLY_INCREMENTED_KEY("return-ok"),
			ATOMICALLY_INCREMENTED_KEY("materialize-page"),

			END_OF_KVS
		}
	},
	{
		.name = "pte-page-track-dwp-hook",
		.kvt = {
			ATOMICALLY_INCREMENTED_KEY("fmw-entry"),
			ATOMICALLY_INCREMENTED_KEY("entry"),
			ATOMICALLY_INCREMENTED_KEY("wpr-path-taken"),
			ATOMICALLY_INCREMENTED_KEY("wpr-caused-by-mkwrite-fault"),
			ATOMICALLY_INCREMENTED_KEY("wpr-caused-by-shared"),
			ATOMICALLY_INCREMENTED_KEY("wpr-caused-by-anon-excl"),
			ATOMICALLY_INCREMENTED_KEY("wpr-prior-checks-pass"),
			ATOMICALLY_INCREMENTED_KEY("wpr-page-ok"),

			END_OF_KVS
		}
	},
	{
		.name = "pte-page-track-wpc-hook",
		.kvt = {
			ATOMICALLY_INCREMENTED_KEY("entry"),
			ATOMICALLY_INCREMENTED_KEY("entry-checks-pass"),
			ATOMICALLY_INCREMENTED_KEY("return-ok"),
			ATOMICALLY_INCREMENTED_KEY("cow-done-and-page-added"),

			END_OF_KVS
		}
	},
	{
		.name = "pte-page-track-spr-hook",
		.kvt = {
			ATOMICALLY_INCREMENTED_KEY("caller-fmp"),
			ATOMICALLY_INCREMENTED_KEY("caller-df"),
			ATOMICALLY_INCREMENTED_KEY("caller-ff"),
			ATOMICALLY_INCREMENTED_KEY("entry-ok"),	
			ATOMICALLY_INCREMENTED_KEY("return-ok"),
			ATOMICALLY_INCREMENTED_KEY("pages-ok"),

			END_OF_KVS
		}
	},
	{
		.name = "pte-page-track-cpr-hook",
		.kvt = {
			ATOMICALLY_INCREMENTED_KEY("entry"),
			ATOMICALLY_INCREMENTED_KEY("return-ok"),
			ATOMICALLY_INCREMENTED_KEY("pages-ok"),

			END_OF_KVS
		}
	},
	{
		.name = "pte-page-track-fuf-hook",
		.kvt = {
			ATOMICALLY_INCREMENTED_KEY("entry"),

			END_OF_KVS
		}
	}
};

extern void free_all_fuf_test_list(void);

static const struct post_testing_teardown_action ttd_actions[] = {
	TTD_ACTION(free_all_fuf_test_list),
};

#endif 

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(pte_page_track, kps, krps, ppt_suts, ttd_actions);
