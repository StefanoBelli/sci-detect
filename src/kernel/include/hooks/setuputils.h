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

#define SETUP_SYM(x) \
	__setup_##x##_hooks

#define TEARDOWN_SYM(x) \
	__teardown_##x##_hooks

#define SETUP_HOOKSGROUP_SIGNATURE(x) \
	int SETUP_SYM(x)(void)

#define TEARDOWN_HOOKSGROUP_SIGNATURE(x) \
	void TEARDOWN_SYM(x)(void)

#ifdef SCID_CONFIG_TESTING

struct post_testing_teardown_action {
	const char* name;
	void (*action)(void);
};

#define TTD_ACTION(fun) \
	(struct post_testing_teardown_action) { \
		.name = #fun, \
		.action = fun, \
	}

#include <testing/testing.h>
#include <logging.h>

#define __size_type typeof(sizeof(0))

#define GENERATE_SETUP_AND_TEARDOWN_CODE(_hooksgroup_, _kps_, _krps_, _tests_arr_, _post_ttd_actions_) \
	static_assert( \
			__builtin_types_compatible_p( \
				typeof((_tests_arr_)), \
				typeof(struct subsys_regi_args[]))); \
	\
	static inline int __hooksgroup_test_regi( \
		const struct subsys_regi_args *tests, \
		__size_type tests_len, \
		const char* hooksgroup) \
	{ \
		if(!tests_len) { \
			scid_infof("no tests were found for hooksgroup %s (they're %ld)", hooksgroup, tests_len); \
			return 0; \
		} \
		\
		for(__size_type i = 0; i < tests_len; i++) { \
			scid_infof("registering %s for testing...", tests[i].name); \
			if(!testing_register_subsys(&tests[i])) \
				return -1; \
		} \
		\
		return 0; \
	} \
	\
	static inline void __hooksgroup_post_ttd_actions( \
		const struct post_testing_teardown_action *actions, \
		__size_type nr_actions, \
		const char* hooksgroup) \
	{ \
		if(!nr_actions) { \
			scid_infof("no post-testing-teardown actions found " \
					"for hooksgroup %s (they're %ld)", hooksgroup, nr_actions); \
			return; \
		} \
		\
		for(__size_type i = 0; i < nr_actions; i++) { \
			scid_infof("running post-testing-teardown action " \
					"(%s/%s)", hooksgroup, actions[i].name); \
			actions[i].action(); \
		} \
	} \
	\
	SETUP_HOOKSGROUP_SIGNATURE(_hooksgroup_); \
	TEARDOWN_HOOKSGROUP_SIGNATURE(_hooksgroup_); \
	\
	__DEFINE_BASE_SETUP_HOOKS_ARGS(bsha, kps, krps); \
	\
	SETUP_HOOKSGROUP_SIGNATURE(_hooksgroup_) \
	{ \
		__size_type nr_tests = __static_array_size((_tests_arr_),[0],); \
		if(__hooksgroup_test_regi((_tests_arr_), nr_tests, #_hooksgroup_)) \
			return -1; \
		return __base_setup_hooks(&bsha); \
	} \
	TEARDOWN_HOOKSGROUP_SIGNATURE(_hooksgroup_) \
	{ \
		__base_teardown_hooks(&bsha); \
		__size_type nr_post_ttd_actions = __static_array_size((_post_ttd_actions_),[0],); \
		__hooksgroup_post_ttd_actions((_post_ttd_actions_), nr_post_ttd_actions, #_hooksgroup_); \
	}
#else
#define GENERATE_SETUP_AND_TEARDOWN_CODE(_hooksgroup_, _kps_, _krps_, ...) \
	SETUP_HOOKSGROUP_SIGNATURE(_hooksgroup_); \
	TEARDOWN_HOOKSGROUP_SIGNATURE(_hooksgroup_); \
	\
	__DEFINE_BASE_SETUP_HOOKS_ARGS(bsha, kps, krps); \
	\
	SETUP_HOOKSGROUP_SIGNATURE(_hooksgroup_) \
	{ \
		return __base_setup_hooks(&bsha); \
	} \
	TEARDOWN_HOOKSGROUP_SIGNATURE(_hooksgroup_) \
	{ \
		__base_teardown_hooks(&bsha); \
	}
#endif

#endif
