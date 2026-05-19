#ifndef SCID_PAGEUTILS_H
#define SCID_PAGEUTILS_H

#include <linux/pgtable.h>
#include <linux/mm.h>

struct page *get_one_page_from_pte(pte_t);

#endif
