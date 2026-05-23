#include <linux/kprobes.h>

#include <hooks.h>
#include <logging.h>

/* all mentioned source files in hooks/ subdir */

/* hpr.c */
extern struct kretprobe handle_pte_fault__krp;

/* dap.c */
extern struct kretprobe do_anonymous_page__krp;

/* wpc.c */
extern struct kretprobe wp_page_copy__krp;

/* dwp.c */
extern struct kretprobe do_wp_page__krp;

/* spr.c */
extern struct kretprobe set_pte_range__krp;
extern struct kprobe do_fault__kp;
extern struct kprobe finish_fault__kp;
extern struct kprobe filemap_map_pages__kp;

static struct kretprobe *krps[] = {
	&handle_pte_fault__krp,
	&do_anonymous_page__krp,
	&wp_page_copy__krp,
	&do_wp_page__krp,
	&set_pte_range__krp,
};

static struct kprobe *kps[] = {
	&do_fault__kp,
	&finish_fault__kp,
	&filemap_map_pages__kp,
};

#define __static_array_size(a) \
	(sizeof(a) / sizeof(typeof(a)*))

#define ARGS(arr) \
	arr, __static_array_size(arr)

int setup_hooks(void) 
{
	int rv;

	scid_infof("registering %ld kretprobes", __static_array_size(krps));
	rv = register_kretprobes(ARGS(krps));
	if(rv) {
		scid_err("unable to register kretprobes");
		return rv;
	}

	scid_infof("registering %ld kprobes", __static_array_size(kps));
	rv = register_kprobes(ARGS(kps));
	if(rv) {
		scid_err("unable to register kprobes");
		unregister_kretprobes(ARGS(krps));
		return rv;
	}

	return 0;
}

void teardown_hooks(void) 
{
	unregister_kprobes(ARGS(kps));
	unregister_kretprobes(ARGS(krps));
}

#undef ARGS
#undef __static_array_size
