#include <hooks/setuputils.h>

static struct kretprobe *krps[] = {

};

static struct kprobe *kps[] = {

};

/* don't touch */
GENERATE_SETUP_AND_TEARDOWN_CODE(chg, kps, krps);
