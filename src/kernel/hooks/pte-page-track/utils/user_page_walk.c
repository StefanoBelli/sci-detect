#include <hooks/pte-page-track/utils/user_page_walk.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/pagewalk.h>
#include <linux/mm.h>

#include <logging.h>

/* don't use GUP, as it will alter PTEs, cause faultins, ... */
struct page *user_page_walk(unsigned long addr, bool quiet)
{
	return NULL;
}


#endif
