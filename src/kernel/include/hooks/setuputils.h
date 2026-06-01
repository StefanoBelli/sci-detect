#ifndef SCID_HOOKS_SETUPUTILS_H
#define SCID_HOOKS_SETUPUTILS_H

#include <linux/kprobes.h>

#define __static_array_size(a, x, y) \
	(sizeof(a) / sizeof(typeof(a x)y))

struct __base_setup_hooks_args {
	struct kretprobe **krps;
	int nr_krps;

	struct kprobe **kps;
	int nr_kps;
};

#define __DEFINE_BASE_SETUP_HOOKS_ARGS(_name_, _kps_, _krps_) \
	static struct __base_setup_hooks_args _name_ = { \
		.kps = (_kps_), \
		.nr_kps = __static_array_size((_kps_),,*), \
		\
		.krps = (_krps_), \
		.nr_krps = __static_array_size((_krps_),,*), \
	}

int __base_setup_hooks(struct __base_setup_hooks_args*);
void __base_teardown_hooks(struct __base_setup_hooks_args*);

#ifdef SCID_CONFIG_TESTING

#include <testing/testing.h>
#include <logging.h>

#define __size_type typeof(sizeof(0))

#define GENERATE_SETUP_AND_TEARDOWN_CODE(_hookgroup_, _kps_, _krps_, _tests_arr_) \
	static_assert( \
			__builtin_types_compatible_p( \
				typeof((_tests_arr_)), \
				typeof(struct subsys_regi_args[]))); \
	\
	static inline int __hookgroup_test_regi( \
		const struct subsys_regi_args *tests, \
		__size_type tests_len, \
		const char* hookgroup) \
	{ \
		if(!tests_len) { \
			scid_infof("no tests were found for hookgroup %s (they're %ld)", hookgroup, tests_len); \
			return 0; \
		} \
		\
		for(__size_type i = 0; i < tests_len; i++) { \
			scid_infof("registering %s/%s for testing...", hookgroup, tests[i].name); \
			if(!testing_register_subsys(&tests[i])) \
				return -1; \
		} \
		\
		return 0; \
	} \
	\
	int __setup_##_hookgroup_##_hooks(void); \
	void __teardown_##_hookgroup_##_hooks(void); \
	\
	__DEFINE_BASE_SETUP_HOOKS_ARGS(bsha, kps, krps); \
	\
	int __setup_##_hookgroup_##_hooks(void) \
	{ \
		__size_type nr_tests = __static_array_size((_tests_arr_),[0],); \
		if(__hookgroup_test_regi((_tests_arr_), nr_tests, #_hookgroup_)) \
			return -1; \
		return __base_setup_hooks(&bsha); \
	} \
	void __teardown_##_hookgroup_##_hooks(void) \
	{ \
		__base_teardown_hooks(&bsha); \
	}
#else
#define GENERATE_SETUP_AND_TEARDOWN_CODE(_hookgroup_, _kps_, _krps_, __unused) \
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

#endif
