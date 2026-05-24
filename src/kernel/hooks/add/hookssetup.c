#include <hooks/setuputils.h>

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

/* don't touch */
DEFINE_SETUP_AND_TEARDOWN_CODE(add, kps, krps);
