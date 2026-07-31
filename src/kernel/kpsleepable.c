#include <linux/percpu.h>
#include <linux/compiler.h>
#include <linux/smp.h>

#include <kpsleepable.h>
#include <logging.h>

#ifdef CONFIG_KALLSYMS
#	include <resolve_syms/kallsyms_lookup_name.h>
#endif

//#define DEBUG_PRINTS_PCP_CKP_ADDR

/*
 * pointer to the per-cpu variable "current_kprobe" (of type struct kprobe*)
 *
 * the pointer is itself per-cpu
 */
static DEFINE_PER_CPU(struct kprobe **, current_kprobe_addr);

/*
 * starting point for the fallback code lookup, dummy value
 */
static DEFINE_PER_CPU(void*, starting_point);

/* this may lead to confusing pointer hell */
static inline struct kprobe **lookup_current_kprobe(struct kprobe *kp, const char **strategy)
{
	void *ptr = NULL;

#ifdef CONFIG_KALLSYMS
	struct kprobe __percpu* cka_kallsyms;

	cka_kallsyms = (struct kprobe __percpu*) THUNK(kallsyms_lookup_name)("current_kprobe");
	if(unlikely(!cka_kallsyms)) {
		scid_warn("kallsyms_lookup_name unable to resolve current_kprobe percpu var");
		goto fallback;
	}

	*strategy = "kallsyms";

	ptr = this_cpu_ptr(cka_kallsyms);
	__this_cpu_write(current_kprobe_addr, (struct kprobe**) ptr);

	return (struct kprobe **) ptr;

fallback:
#else
#	warning CONFIG_KALLSYMS is not enabled, lookup_current_kprobe will run fallback code
#endif 

	scid_warn("lookup_current_kprobe running fallback code!");

	/* define the starting address */
	void* __percpu *tmp = &starting_point;
	
	while(tmp) {
		/* decrease the starting address (point. arith.) */
		tmp--;

		/* dereference the address and do check */
		if((struct kprobe*) __this_cpu_read(*tmp) == kp)
			break;
	}

	/* tmp is a pointer to a pointer to a kprobe */
	if(likely(tmp)) {
		*strategy = "fallback lookup";
		ptr = (struct kprobe **) this_cpu_ptr(tmp);
		__this_cpu_write(current_kprobe_addr, ptr);
	}

	return (struct kprobe **) ptr;
}

struct kprobe **locate_pcp_ckp_addr(struct kprobe *kp)
{
	__maybe_unused const char* locate_strategy;
	struct kprobe **ckp_addr = __this_cpu_read(current_kprobe_addr);

	if(unlikely(!ckp_addr)) { 
		ckp_addr = lookup_current_kprobe(kp, &locate_strategy); 
		if(unlikely(!ckp_addr)) { 
			scid_errf("unable to locate current_kprobe! strategy=%s", locate_strategy);
			BUG();
		}

#ifdef DEBUG_PRINTS_PCP_CKP_ADDR
		scid_infof("on cpu #%d, found (via strategy=%s):"
				"\n --> current_kprobe_addr=%px,"
				"\n --> pcp var ptr=%px,"
				"\n --> pcp var content=%px,"
				"\n --> kp=%px,"
				"\n --> deref cka (kp)=%px", 
				smp_processor_id(), 
				locate_strategy,
				ckp_addr, 
				this_cpu_ptr(&current_kprobe_addr),
				__this_cpu_read(current_kprobe_addr),
				kp,
				*ckp_addr);
#else
		scid_infof("on cpu #%d, resolved current_kprobe"
				" via strategy=%s", smp_processor_id(), 
				locate_strategy);
#endif /* DEBUG_PRINTS_PCP_CKP_ADDR */

	}

	return ckp_addr;
}

