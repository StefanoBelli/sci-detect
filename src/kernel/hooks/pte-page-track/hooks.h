#include <linux/version.h>

/* actually it is something like 6.13.x+ */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,14,0)
#error not supported 
#endif

/* hpr.c */
extern struct kretprobe handle_pte_fault__krp;

/* dap.c */
extern struct kretprobe do_anonymous_page__krp;

/* wpc.c */
extern struct kretprobe wp_page_copy__krp;

/* dwp.c */
extern struct kretprobe do_wp_page__krp;
extern struct kprobe finish_mkwrite_fault__kp;

/* spr.c */
extern struct kretprobe set_pte_range__krp;
extern struct kprobe do_fault__kp;
extern struct kprobe finish_fault__kp;
extern struct kprobe filemap_map_pages__kp;


