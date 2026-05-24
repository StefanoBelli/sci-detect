#ifndef SCID_HOOKS_SETUPUTILS_H
#define SCID_HOOKS_SETUPUTILS_H

#include <linux/kprobes.h>

#define __static_array_size(a) \
	(sizeof(a) / sizeof(typeof(a)*))

struct __base_setup_hooks_args {
	struct kretprobe **krps;
	int nr_krps;

	struct kprobe **kps;
	int nr_kps;
};

#define __DEFINE_BASE_SETUP_HOOKS_ARGS(_name_, _kps_, _krps_) \
	static struct __base_setup_hooks_args _name_ = { \
		.kps = (_kps_), \
		.nr_kps = __static_array_size((_kps_)), \
		\
		.krps = (_krps_), \
		.nr_krps = __static_array_size((_krps_)), \
	}

int __base_setup_hooks(struct __base_setup_hooks_args*);
void __base_teardown_hooks(struct __base_setup_hooks_args*);

#define DEFINE_SETUP_AND_TEARDOWN_CODE(_hookgroup_, _kps_, _krps_) \
	int __setup_##_hookgroup_##_hooks(void); \
	void __teardown_##_hookgroup_##_hooks(void); \
	\
	__DEFINE_BASE_SETUP_HOOKS_ARGS(bsha, kps, krps); \
	\
	int __setup_##_hookgroup_##_hooks(void) \
	{ \
		return __base_setup_hooks(&bsha); \
	} \
	void __teardown_##_hookgroup_##_hooks(void) \
	{ \
		__base_teardown_hooks(&bsha); \
	}

#endif
