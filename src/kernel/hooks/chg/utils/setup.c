#include <hooks/setuputils.h>

#include "../hooks.h"

static struct kretprobe *krps[] = {
	&change_pte_range__krp,
};

static struct kprobe *kps[] = {

};

#ifdef SCID_CONFIG_TESTING
static struct subsys_regi_args chg_tests[] = { };
#endif

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(chg, kps, krps, chg_tests);
