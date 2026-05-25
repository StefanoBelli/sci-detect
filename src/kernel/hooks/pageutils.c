#include <logging.h>
#include <hooks/pageutils.h>

/*
 * TODO may need to extend support to get multiple pages
 * for files, for example.
 */
struct page *get_one_page_from_pte(pte_t pte) {
	struct page *page = pte_page(pte);
	if(!page) {
		scid_err("NULL page descriptor");
		return NULL;
	}

	/* every page is part of a folio */
	struct folio *folio = page_folio(page);
	if(!folio)
		return NULL;

	if(!folio_try_get(folio)) {
		scid_err("unable to get a folio reference");
		return NULL;
	}

	unsigned long nr_pages = folio_nr_pages(folio);

	folio_put(folio);

	if(nr_pages > 1) {
		scid_err("corresponding folio has more than 1 pages");
		return NULL;
	}

	return page;
}
