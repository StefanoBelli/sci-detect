#ifdef SCID_CONFIG_TESTING

#include <testing/default-kvops.h>

#include <linux/compiler.h>
#include <linux/sprintf.h>

bool atomic_inc_init_kvop(
		void* value, 
		__always_unused unsigned long value_size)
{
	atomic_set((atomic_t*) value, 0);
	return true;
}

ssize_t atomic_inc_uquery_kvop(
		void *dst, 
		size_t dst_max_size,
		const void *value, 
		__always_unused unsigned long value_size)
{
	return snprintf(dst, dst_max_size, "%d", 
			atomic_read((atomic_t*) value));
}

void atomic_inc_set_kvop(
		void *value, 
		__always_unused const void *args, 
		__always_unused unsigned long value_size)
{
	atomic_inc((atomic_t*) value);
}

void atomic_inc_reset_kvop(
		void *value, 
		__always_unused unsigned long value_size)
{
	atomic_set((atomic_t*) value, 0);
}

#endif
