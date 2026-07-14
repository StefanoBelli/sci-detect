#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/kprobes.h>

#define force_sig_fault__symbol "force_sig_fault"

static int force_sig_fault__phkphook(
		struct kprobe *kp, struct pt_regs *regs)
{
	return 0;
}

struct kprobe force_sig_fault__kp = {
	.symbol_name = force_sig_fault__symbol,
	.pre_handler = force_sig_fault__phkphook,
};

#endif /* DO_PTE_ALT_PROT */
