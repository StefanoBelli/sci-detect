#include <logging.h>
#include <hooks/add/utils/addpages.h>

bool add_one_page(
		pte_t* ptep, more_checks_pte_fp checks, 
		void* args, struct page **page)
{
	pte_t pte = ptep_get(ptep);

	if(pte_none(pte)) {
		scid_err("pte is none");
		return false;
	}

	if(!pte_present(pte)) {
		scid_err("pte is not present");
		return false;
	}

	int has_wr = pte_write(pte);
	int has_exec = pte_exec(pte);

	if(checks && !checks(pte, has_wr, has_exec, args)) {
		scid_err("user checks on pte failed");
		return false;
	}

	struct page *pg = pte_page(pte);
	if(!pg)
		return false;

	if(page)
		*page = pg;

	//add to the XArray / radix tree
	//
	return true;
}

bool add_pages_byfolio(
		pte_t *ptep, more_checks_pte_fp checks, 
		void* args, bool onepg, unsigned long *nr_pages)
{
	struct page *page = NULL;

	if(!add_one_page(ptep, checks, args, &page))
		return false;

	if(nr_pages)
		(*nr_pages)++;
	
	struct folio *folio = page_folio(page);

	/* this should not happen */
	if(unlikely(!folio)) {
		scid_err("unable to get the associated folio");
		return false;
	}

	if(!folio_try_get(folio)) {
		scid_err("unable to get ref to the folio");
		return false;
	}

	unsigned long fnr_pages = folio_nr_pages(folio);

	folio_put(folio);

	/* this should not happen */
	if(unlikely(fnr_pages == 0)) {
		scid_err("nr_pages is 0 ???");
		BUG();
		return false;
	}

	if(onepg && fnr_pages > 1) {
		scid_warn("folio is more than 1 pages!");
		BUG();
		return false; 
	}

	while(--fnr_pages) {
		if(!add_one_page(++ptep, checks, args, NULL))
			return false;

		if(nr_pages)
			(*nr_pages)++;
	}

	return true;
}

unsigned long add_pages_bynr(
		pte_t *ptep, more_checks_pte_fp checks, 
		void* args, unsigned long nr)
{
	unsigned long tracked_pages = 0;

	if(nr == 0)
		return 0;

	while(nr--) {
		if(!add_one_page(ptep++, checks, args, NULL))
			return tracked_pages;

		tracked_pages++;
	}

	return tracked_pages;
}
