#include "testutils.h"
#include <sys/mman.h>
#include <sys/wait.h>

/* set_pte_range */
#define SPR_SUBSYS_NAME "add-spr-hook"

#define SPR_CALLER_FMP_KEY "caller-fmp"
#define SPR_CALLER_DF_KEY "caller-df"
#define SPR_CALLER_FF_KEY "caller-ff"
#define SPR_ENTRY_OK_KEY "entry-ok"
#define SPR_RETURN_OK_KEY "return-ok"
#define SPR_PAGES_OK_KEY "pages-ok"

/* do_wp_page */
#define DWP_SUBSYS_NAME "add-dwp-hook"

#define DWP_ENTRY_KEY "entry"
#define DWP_WPR_PATH_TAKEN_KEY "wpr-path-taken"
#define DWP_WPR_MKWRITE_KEY "wpr-caused-by-mkwrite-fault"
#define DWP_WPR_SHARED_KEY "wpr-caused-by-shared"
#define DWP_WPR_ANONEXCL_KEY "wpr-caused-by-anon-excl"
#define DWP_WPR_PRIOR_CHECKS_PASS_KEY "wpr-prior-checks-pass"
#define DWP_WPR_PAGE_OK_KEY "wpr-page-ok"

/* wp_page_copy */
#define WPC_SUBSYS_NAME "add-wpc-hook"

#define WPC_ENTRY_KEY "entry"
#define WPC_ENTRY_CHECKS_PASS_KEY "entry-checks-pass"
#define WPC_RETURN_OK_KEY "return-ok"
#define WPC_COW_DONE_KEY "cow-done-and-page-added"

#define RESET_ALL() \
	reset_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY); \
	reset_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY); \
	reset_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY); \
	reset_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY); \
	reset_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY); \
	reset_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY); \
	\
	reset_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY); \
	reset_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY); \
	reset_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY); \
	reset_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY); \
	reset_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY); \
	reset_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY); \
	reset_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY); \
	\
	reset_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY); \
	reset_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY); \
	reset_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY); \
	reset_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY)

int main()
{
	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SPR_SUBSYS_NAME);
	start_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
	start_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
	start_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
	start_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
	start_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
	start_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

	enable_testing_for_me(DWP_SUBSYS_NAME);
	start_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
	start_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
	start_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
	start_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
	start_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
	start_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
	start_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

	enable_testing_for_me(WPC_SUBSYS_NAME);
	start_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
	start_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
	start_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
	start_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

	/* small-file (single-page-sized file): first access by load instruction */
	{
		int fd = open("res/small-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access -- may, or may not, fault around (filemap_map_pages) */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second write access -- do private CoW, make my own copy */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing. CoW done. */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* small-file (single-page-sized file): first access by store instruction */
	{
		int fd = open("res/small-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first write access -- do private CoW, make my own copy 
		 * (will *not* fault around, this is a write access) */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second read access */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing. CoW done. */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* small-file (single-page-sized file): first access by read syscall */
	{
		int fd = open("res/small-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access via syscall -- do nothing on PTEs, kernel optimizes... */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test second read access */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test third write access -- do CoW. */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. 
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}*/

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* small-file (single-page-sized file): first access by write syscall */
	{
		int fd = open("res/small-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first write access via syscall -- do CoW now. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second read access */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* small-file (single-page-sized file): CoW by write syscall, after PTE being setupped */
	{
		int fd = open("res/small-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access - setup PTEs */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second write access via syscall -- do CoW now. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* small-file (single-page-sized file): CoW by write syscall, after PTE **not** being setupped */
	{
		int fd = open("res/small-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access via syscall */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test second write access via syscall -- do CoW now. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test third write access -- do nothing */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-paged-sized file): first access by load instruction */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access -- may, or may not, fault around (filemap_map_pages) */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second write access -- do private CoW, make my own copy */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing. CoW done. */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): first access by store instruction */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first write access -- do private CoW, make my own copy 
		 * (will *not* fault around, this is a write access) */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second read access */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing. CoW done. */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): first access by read syscall */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access via syscall -- do nothing on PTEs, kernel optimizes... */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test second read access - fault around, most likely */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test third write access -- do CoW. */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): first access by write syscall */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first write access via syscall -- do CoW now. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second read access */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): CoW by write syscall, after PTE being setupped */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access - setup PTEs, fault around */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second write access via syscall -- do CoW now. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test third write access -- do nothing */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): CoW by write syscall, after PTE **not** being setupped */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			/* hmm... fmp > 0 */
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access via syscall */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test second write access via syscall -- do CoW now. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test third write access -- do nothing */
		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fourth write access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test fifth read access (via syscall) -- do nothing. CoW done. */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file) */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first write access via syscall -- do CoW now. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test second write access via store instr, another page. -- do CoW now. 
		 * No fault around earlier, no fault around now (write access).
		 * PTE is missing. */
		{
			spurious_byte_memwrite(page_nr(6), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test third write access via syscall, on a third page. -- do CoW now. 
		 * No fault around earlier, no fault around now (write access).
		 * PTE is missing. */
		{
			die_if(trigger_syscall_pagewrite(page_nr(7), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_eq_hard(spr_caller_ff, 1);
			test_int_eq_hard(spr_entry_ok, 1);
			test_int_eq_hard(spr_return_ok, 1);
			test_int_eq_hard(spr_pages_ok, 1);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): try to cause the fault-around */
	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		/* TEST no initial access - something strange happens */
		{
			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access via load instr -- this will cause fault around */
		{
			spurious_byte_memread(ch, page_nr(3));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		RESET_ALL();

		/* test first read access on another page, via load instr -- nothing (should) happen */
		{
			spurious_byte_memread(ch, page_nr(1));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access on another page, via load instr -- nothing (should) happen */
		{
			spurious_byte_memread(ch, page_nr(10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_ge_hard(spr_caller_df, 0);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 0);
			test_int_ge_hard(spr_return_ok, 0);
			test_int_ge_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first write access on another page (we **assume** faulted around earlier here), via store instr -- 
		 * do CoW from page cache and setup PTE (write enable), no fault around */
		{
			spurious_byte_memwrite(page_nr(9), 'a');

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first write access on another page (we **assume** faulted around earlier here), via syscall -- 
		 * do CoW from page cache and setup PTE (write enable), no fault around */
		{
			die_if(trigger_syscall_pagewrite(page_nr(8), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 1);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 1);
			test_int_eq_hard(wpc_entry_check_pass, 1);
			test_int_eq_hard(wpc_return_ok, 1);
			test_int_eq_hard(wpc_cow_done, 1);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}

		RESET_ALL();

		/* test first read access on another page via syscall, anyway nothing should happen here
		{
			die_if(trigger_syscall_pageread(page_nr(2), 10));

			int dwp_entry = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
			int dwp_wpr_taken = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
			int dwp_wpr_mkwrite = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
			int dwp_wpr_shared = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
			int dwp_wpr_anonexcl = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
			int dwp_wpr_prior_checks_pass = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
			int dwp_wpr_page_ok = query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

			int wpc_entry = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
			int wpc_entry_check_pass = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
			int wpc_return_ok = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
			int wpc_cow_done = query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

			int spr_caller_fmp = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
			int spr_caller_df = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
			int spr_caller_ff = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
			int spr_entry_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
			int spr_return_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
			int spr_pages_ok = query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

			test_int_eq_hard(dwp_entry, 0);
			test_int_eq_hard(dwp_wpr_taken, 0);
			test_int_eq_hard(dwp_wpr_mkwrite, 0);
			test_int_eq_hard(dwp_wpr_shared, 0);
			test_int_eq_hard(dwp_wpr_anonexcl, 0);
			test_int_eq_hard(dwp_wpr_prior_checks_pass, 0);
			test_int_eq_hard(dwp_wpr_page_ok, 0);

			test_int_eq_hard(wpc_entry, 0);
			test_int_eq_hard(wpc_entry_check_pass, 0);
			test_int_eq_hard(wpc_return_ok, 0);
			test_int_eq_hard(wpc_cow_done, 0);
	
			test_int_eq_hard(spr_caller_fmp, 0);
			test_int_eq_hard(spr_caller_df, 0);
			test_int_eq_hard(spr_caller_ff, 0);
			test_int_eq_hard(spr_entry_ok, 0);
			test_int_eq_hard(spr_return_ok, 0);
			test_int_eq_hard(spr_pages_ok, 0);
		}*/

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	test_passed();

__finish:
	stop_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
	stop_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
	stop_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
	stop_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
	stop_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
	stop_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);
	disable_testing_for_me(SPR_SUBSYS_NAME);

	stop_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
	stop_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
	stop_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
	stop_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
	stop_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
	stop_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
	stop_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);
	disable_testing_for_me(DWP_SUBSYS_NAME);

	stop_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
	stop_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
	stop_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
	stop_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);
	disable_testing_for_me(WPC_SUBSYS_NAME);

	return rv;
}

