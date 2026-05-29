#ifndef SCID_TESTING_H
#define SCID_TESTING_H

#include <linux/types.h>

#include <linux/compiler_types.h>

/* let scid subsystems expose kv-pairs via sysfs.
 *
 * This is useful and enabled only when testing (SCID_CONFIG_TESTING)
 */

int setup_testing(void);
void teardown_testing(void);

#ifdef SCID_CONFIG_TESTING

struct kv_ops {
	void (*uquery_value)(
			void __user *dst, 
			const void *value, 
			unsigned long value_size);
	void (*set_value)(
			void *value, 
			const void *args,
			unsigned long value_size);
};

struct subsys_kv_template {
	const char* key;
	unsigned long value_size;
	struct kv_ops kv_ops;
};

struct subsys_regi_args {
	const char* name;
	unsigned long kvt_len;
	struct subsys_kv_template kvt;
};

#else

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
bool testing_register_subsys(struct subsys_regi_args *args);
bool testing_setval(const char *subsys_name, const char *key, void *args);

#endif
