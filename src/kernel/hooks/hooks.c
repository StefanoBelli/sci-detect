#include <linux/kprobes.h>

#include <hooks/hooks.h>

extern struct kretprobe handle_pte_fault__krp; /* hpf.c */
extern struct kretprobe do_anonymous_page__krp; /* dap.c */
extern struct kretprobe wp_page_copy__krp; /* wpc.c */
extern struct kretprobe do_wp_page__krp; /* dwp.c */

static struct kretprobe *krps[] = {
	&handle_pte_fault__krp,
	&do_anonymous_page__krp,
	&wp_page_copy__krp,
	&do_wp_page__krp,
};

#define ARGS(arr, type) \
	arr, sizeof(arr) / sizeof(type*)

int setup_hooks(void) 
{
	return register_kretprobes(ARGS(krps, struct kretprobe));
}

void teardown_hooks(void) 
{
	unregister_kretprobes(ARGS(krps, struct kretprobe));
}

#undef ARGS
