#include <hooks/setuputils.h>

#include "../hooks.h"

static struct kretprobe *krps[] = {

};

static struct kprobe *kps[] = {

};

#ifdef SCID_CONFIG_TESTING
static struct subsys_regi_args del_tests[] = { };
#endif

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(del, kps, krps, del_tests);
