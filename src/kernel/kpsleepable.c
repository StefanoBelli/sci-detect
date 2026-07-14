#include <linux/percpu.h>
#include <linux/compiler.h>

#include <kpsleepable.h>
#include <logging.h>

/*
 * pointer to the per-cpu variable "current_kprobe" (of type struct kprobe*)
 *
 * the pointer is itself per-cpu
 */
static DEFINE_PER_CPU(struct kprobe **, current_kprobe_addr);

/*
 * starting point for the lookup, dummy value
 */
static DEFINE_PER_CPU(unsigned long, starting_point);

/* this may lead to confusing pointer hell */
static inline struct kprobe **lookup_current_kprobe(struct kprobe *kp)
{
	/* define the starting address */
	void *tmp = this_cpu_ptr(&starting_point);
	
	while(tmp) {
		/* decrease the starting address (point. arith.) */
		tmp--;

		/* dereference the address and do check */
		if(*(struct kprobe**)tmp == kp)
			break;
	}

	/* tmp is a pointer to a pointer to a kprobe */
	if(likely(tmp))
		__this_cpu_write(current_kprobe_addr, (struct kprobe**) tmp);

	return (struct kprobe **) tmp;
}

struct kprobe **locate_pcp_ckp_addr(struct kprobe *kp)
{
	struct kprobe **ckp_addr = __this_cpu_read(current_kprobe_addr);
	if(unlikely(!ckp_addr)) { 
		ckp_addr = lookup_current_kprobe((kp)); 
		if(unlikely(!ckp_addr)) { 
			scid_err("unable to locate current_kprobe!");
			BUG();
		}

#ifdef __SCID_INFO_LOCATED_CKA
		scid_infof("on cpu #%d, found:"
				"\n --> current_kprobe_addr=%px,"
				"\n --> pcp var ptr=%px,"
				"\n --> pcp var content=%px,"
				"\n --> kp=%px,"
				"\n --> deref cka (kp)=%px", 
				smp_processor_id(), 
				ckp_addr, 
				this_cpu_ptr(&current_kprobe_addr),
				__this_cpu_read(current_kprobe_addr),
				kp,
				*ckp_addr);
#endif /* __SCID_INFO_LOCATED_CKA */

	}

	return ckp_addr;
}

