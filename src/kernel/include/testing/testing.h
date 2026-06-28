#ifndef SCID_TESTING_H
#define SCID_TESTING_H

#include <linux/types.h>

/* let scid subsystems expose kv-pairs via, e.g. sysfs
 *
 * This is useful and enabled only when testing (SCID_CONFIG_TESTING)
 */

int setup_testing(void);
void teardown_testing(void);

#ifdef SCID_CONFIG_TESTING

struct kv_ops {
	/* called when kvpair is first created */
	bool (*init_value)(
			void *value,
			unsigned long value_size);

	/* called when user is requesting to query value
	 * NOTE: this is mainly built with sysfs support in mind,
	 * dst is a charbuf, user will be able to read, so copy 
	 * printable chars in dst (e.g. integers) */
	ssize_t (*uquery_value)(
			void *dst, 
			size_t dst_max_size,
			const void *value, 
			unsigned long value_size);

	/* called when subsys is requesting to update value */
	void (*set_value)(
			void *value, 
			const void *args,
			unsigned long value_size);

	/* called when user is requesting to reset value */
	void (*reset_value)(
			void *value, 
			unsigned long value_size);
};

struct subsys_kv_template {
	const char* key;
	unsigned long value_size;
	struct kv_ops kv_ops;
};

#define MAX_KVS 16

#define END_OF_KVS (struct subsys_kv_template) { .key = NULL }

#define __base_kv(_key, _value_size, _kv_ops) \
	(struct subsys_kv_template) { \
		.key = (_key), \
		.value_size = (_value_size), \
		.kv_ops = _kv_ops \
	}

struct subsys_regi_args {
	const char* name;
	struct subsys_kv_template kvt[MAX_KVS + 1];
};

#else

/* opaque */
struct subsys_regi_args;

#endif

/*
 * testing_register_subsys - early register of subsys to test
 *
 * Must be called in process context, once at a time :)
 *
 * @args: allocate them statically, as we will refer to them! 
 *
 * Returns: true if everything ok, false otherwise
 */
bool testing_register_subsys(const struct subsys_regi_args *args);

#ifdef SCID_CONFIG_TESTING

#include <logging.h>

#define TESTING_SETVAL_NOTASK -1
#define TESTING_SETVAL_NOSUT -2
#define TESTING_SETVAL_NOINST -3
#define TESTING_SETVAL_NOKEY -4
#define TESTING_SETVAL_NOTSTARTED -5
#define TESTING_SETVAL_NOKVOP -6

/*
 * __do_testing_setval - called when subsys wants to modify a value
 * from kvpair (actually, use "testing_setval")
 *
 * @subsys_name: name of the registered subsys
 * @key: key to change value for
 * @args: optional args to pass to the set_value callback
 *
 * Returns: 0 if ok, < 0 if any error (see above)
 */
int __do_testing_setval(const char *subsys_name, const char *key, void *args);

/**
 * __do_testing_is_sut_key - check if a key of a subsys is under active
 * monitoring. Caller needs to define a RCU critical section.
 *
 * @subsys_name: name of the subsys
 * @key: the key to check for
 * @kvp: ptr to returned kvpair, may be NULL if unused
 *
 * Returns: 0 if ok, < 0 if any error
 */
int __do_testing_is_sut_key(const char *subsys_name, const char* key, void *kvp);

/**
 * __do_testing_is_sut_key_rcu - placed a RCU critical section for you
 *
 * @n: the subsys name
 * @k: the key
 * @v: the kvp ptr
 *
 * Returns: 0 if ok, < 0 if any error
 */
#define __do_testing_is_sut_key_rcu(n, k, v) \
	({ \
	 	int err; \
	 	\
		rcu_read_lock(); \
		err = __do_testing_is_sut_key((n), (k), (v)); \
		rcu_read_unlock(); \
		\
		err; \
	 })

#define __testing_errwrapper(__kall) \
	({ \
		int ret; \
		ret = __kall; \
		if(ret == TESTING_SETVAL_NOTASK) \
			scid_err("not in process context!"); \
		else if(ret == TESTING_SETVAL_NOSUT) \
			scid_err("this subsys was not early-registered!"); \
		else if(ret == TESTING_SETVAL_NOKEY) \
			scid_err("invalid key"); \
		else if(ret == TESTING_SETVAL_NOKVOP) \
			scid_err("set_value kvop is NULL!"); \
		ret; \
	})

#define __testing_setval(n, k, a) \
	__testing_errwrapper( \
			__do_testing_setval((n), (k), (a)))

#define __testing_is_sut_key(n, k, v) \
	__testing_errwrapper( \
			__do_testing_is_sut_key_rcu((n), (k), (v)))

/* since this is called in hooks continuously, avoid the fn call when 
 * testing is disabled, compiler should even optimize the branch
 */
#define testing_setval __testing_setval
#define testing_is_sut_key __testing_is_sut_key
#else
#define testing_setval(x, y, z) ({0;})
#endif

/* helpers */
#define __testing(key) testing_setval(MY_TESTING_SUBSYS_NAME, (key), NULL)
#define __testing_arg(key, arg) testing_setval(MY_TESTING_SUBSYS_NAME, (key), (arg))

#endif
