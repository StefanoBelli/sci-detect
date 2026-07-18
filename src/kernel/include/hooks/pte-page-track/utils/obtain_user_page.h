#ifndef SCID_UTILS_OUP_H
#define SCID_UTILS_OUP_H

#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

struct page* obtain_user_page_from_addr(unsigned long addr);

#endif

#endif
