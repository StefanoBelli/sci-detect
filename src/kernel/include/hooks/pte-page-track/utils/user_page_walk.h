#ifndef SCID_UTILS_UPW_H
#define SCID_UTILS_UPW_H

#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/kprobes.h>
#include <linux/spinlock.h>

/**
 * __user_page_walk - traverse the page tables of current
 *
 * Doesn't use GUP.
 *
 * @addr: the virtual addr
 * @quiet: whether to tell if pte is mapped or not
 * @rlkmm: whether to acquire the mmap_read_lock or not current->mm
 * @ptep: the ptep which points to the page, may be null.
 * @ptlp: the associated page table lock, may be null.
 * @kp: the current kp
 *
 * Returns: the page descriptor if found, NULL othw
 */
struct page* user_page_walk_ptep(
		unsigned long addr, bool quiet, bool rlkmm, 
		pte_t **ptep, spinlock_t **ptlp, struct kprobe *kp);

static inline struct page *user_page_walk(unsigned long addr, bool quiet, 
		bool rlkmm, struct kprobe *kp)
{
	return user_page_walk_ptep(addr, quiet, rlkmm, NULL, NULL, kp);
}

#endif

#endif
