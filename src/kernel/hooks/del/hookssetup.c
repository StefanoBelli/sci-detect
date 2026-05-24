#include <hooks/setuputils.h>

static struct kretprobe *krps[] = {

};

static struct kprobe *kps[] = {

};

/* don't touch */

int __setup_del_hooks(void);
void __teardown_del_hooks(void);

__DEFINE_BASE_SETUP_HOOKS_ARGS(bsha, kps, krps);

int __setup_del_hooks(void)
{
	return __base_setup_hooks(&bsha);
}

void __teardown_del_hooks(void)
{
	__base_teardown_hooks(&bsha);
}
