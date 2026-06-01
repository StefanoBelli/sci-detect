#ifndef SCID_TESTING_DEFAULT_KVOPS_H
#define SCID_TESTING_DEFAULT_KVOPS_H

#ifdef SCID_CONFIG_TESTING

#include <testing/testing.h>

/* you'll have to set these manually into your kvops */

bool atomic_inc_init_kvop(void* value, unsigned long value_size);
ssize_t atomic_inc_uquery_kvop(
		void *dst, size_t dst_max_size, const void *value, unsigned long value_size);
void atomic_inc_set_kvop(void *value, const void *args, unsigned long value_size);
void atomic_inc_reset_kvop(void *value, unsigned long value_size);

#define __atomic_inc_valuesize sizeof(atomic_t)

#define __atomic_inc_kv_ops \
	(struct kv_ops) { \
		.init_value = atomic_inc_init_kvop, \
		.set_value = atomic_inc_set_kvop, \
		.reset_value = atomic_inc_reset_kvop, \
		.uquery_value = atomic_inc_uquery_kvop \
	}

#define ATOMICALLY_INCREMENTED_KEY(key) \
	__base_kv(key, __atomic_inc_valuesize, __atomic_inc_kv_ops)

#endif

#endif
