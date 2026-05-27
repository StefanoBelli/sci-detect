#include <hooks/setuputils.h>

#include "../hooks.h"

static struct kretprobe *krps[] = {
	&change_pte_range__krp,
};

static struct kprobe *kps[] = {

};

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(chg, kps, krps);
