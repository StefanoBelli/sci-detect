#ifndef SCID_KPSLEEPABLE_H
#define SCID_KPSLEEPABLE_H

#include <linux/preempt.h>
#include <linux/kprobes.h>

struct kprobe **locate_pcp_ckp_addr(struct kprobe *kp);

#define __enable_sleep(kp) \
	do { \
		\
		struct kprobe **ckp_addr; \
		\
		ckp_addr = locate_pcp_ckp_addr((kp)); \
		*ckp_addr = NULL; \
		\
		preempt_enable(); \
	} while(0)

#define __disable_sleep(kp) \
	do { \
		\
		struct kprobe **ckp_addr; \
		\
		preempt_disable(); \
		\
		ckp_addr = locate_pcp_ckp_addr((kp)); \
		*ckp_addr = (kp); \
	} while(0)

#ifdef CONFIG_KRETPROBE_ON_RETHOOK
#	define kpat(__krp, unused) (&(__krp).kp)
#else
#	define kpat(unused, __krpi) (&(__krpi)->rp->kp)
#endif

#endif
