#ifndef SCID_UTILS_UPW_H
#define SCID_UTILS_UPW_H

#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/kprobes.h>

/**
 * user_page_walk - traverse the page tables of current
 *
 * Doesn't use GUP.
 *
 * @addr: the virtual addr
 * @quiet: whether to tell if pte is mapped or not
 * @rlkmm: whether to acquire the mmap_read_lock or not current->mm
 * @kp: the current kp
 *
 * Returns: the page descriptor if found, NULL othw
 */
struct page* __user_page_walk(
		unsigned long addr, bool quiet, bool rlkmm, struct kprobe *kp);

static inline struct page *user_page_walk(unsigned long addr, bool quiet, 
		struct kprobe *kp)
{
	return __user_page_walk(addr, quiet, true, kp);
}

static inline struct page *user_page_walk_norlkmm(unsigned long addr, bool quiet,
		struct kprobe *kp)
{
	return __user_page_walk(addr, quiet, false, kp);
}

#endif

#endif
