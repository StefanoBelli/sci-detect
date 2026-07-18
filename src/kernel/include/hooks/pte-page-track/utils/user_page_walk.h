#ifndef SCID_UTILS_UPW_H
#define SCID_UTILS_UPW_H

#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

/**
 * user_page_walk - traverse the page tables of current
 *
 * Doesn't use GUP.
 *
 * @addr: the virtual addr
 * @quiet: whether to tell if pte is mapped or not
 *
 * Returns: the page descriptor if found, NULL othw
 */
struct page* user_page_walk(unsigned long addr, bool quiet);

#endif

#endif
