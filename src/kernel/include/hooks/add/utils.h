#ifndef SCID_HOOK_ADD_UTILS_H
#define SCID_HOOK_ADD_UTILS_H

#include <linux/mm.h>
#include <linux/pgtable.h>

/* user can provide a callback to evaluate pte */
typedef bool (*more_checks_pte_fp)(pte_t pte, int rw, int exec, void* args);

/*
 * add_one_page - add one page to track, derive it from the ptep
 *
 * @ptep: the ptep
 * @checks: user-provided callback to do specific checks on pte
 * @args: the user-provided args to pass to checks
 * @page: can be NULL, set to the retrieved page, you init your local ptr
 *
 * Returns: true if did it, false otherwise
 */
bool add_one_page(
		pte_t* ptep, more_checks_pte_fp checks, 
		void* args, struct page **page);

/*
 * add_pages_byfolio - add potentially multiple pages to track.
 * Derive the first page from ptep, get its folio and so folio_nr_pages.
 * Go through the next "folio_nr_pages" PTEs to get their page descriptors
 *
 * This assumes that consecutive VAs were assigned to all pages in that folio,
 * pages in a folio are contiguous. 
 *
 * Caller responsibility to ensure this is the che case.
 *
 * @ptep: the starting ptep
 * @checks: user-provided callback to do specific checks on pte
 * @args: the user-provided args to pass to checks
 * @onepg: if true, the folio must contain one page, give a warning and return if not
 * @nr_pages: can be NULL, set to the nr of tracked pages, you init your local ptr
 *
 * Returns: true if everything ok, false othw
 */
bool add_pages_byfolio(
		pte_t* ptep, more_checks_pte_fp checks, void *args, 
		bool onepg, unsigned long *nr_pages);


/*
 * add_pages_bynr - add nr pages to track
 *
 * @ptep: the starting ptep
 * @checks: user-provided callback to do specific checks on pte
 * @args: the user-provided args to pass to checks
 * @nr: the number of consecutive PTEs to look starting from ptep
 *
 * Returns: the number of tracked pages
 */
unsigned long add_pages_bynr(
		pte_t *ptep, more_checks_pte_fp checks, 
		void* args, unsigned long nr);

#endif
