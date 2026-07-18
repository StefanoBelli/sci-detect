#include <hooks/pte-page-track/utils/obtain_user_page.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/mm.h>
#include <logging.h>

struct page *obtain_user_page_from_addr(unsigned long addr)
{
	struct page *pages[1] = { NULL };
	int nr_pages;

	nr_pages = get_user_pages_fast(addr, 1, 0, pages);
	if(nr_pages == 1) {
		put_page(pages[0]);
		return pages[0];
	}

	scid_errf("unable to get user page, err = %d", nr_pages);
	return NULL;
}

#endif
