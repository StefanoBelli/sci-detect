#include <linux/kprobes.h>
#include <linux/mm.h>

#include <vmfs.h>
#include <logging.h>

#define wp_page_reuse__symbol "wp_page_reuse"

static int wp_page_reuse__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) {

	struct vm_fault *vmf = (struct vm_fault*) regs->di;

	/* are we on the right kernel control path? */
	if(!got_this_vmf(vmf))
		return 1;

	return 0;
}

static int wp_page_reuse__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) {

	return 0;
}

struct kretprobe wp_page_reuse__krp = {
	.entry_handler = wp_page_reuse__ehkrphook,
	.handler = wp_page_reuse__hkrphook,
	.kp.symbol_name = wp_page_reuse__symbol,
	.data_size = sizeof(struct vm_fault*),
};
