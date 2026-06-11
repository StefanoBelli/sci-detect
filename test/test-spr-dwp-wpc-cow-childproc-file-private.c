#include "testutils.h"
#include <sys/mman.h>
#include <sys/wait.h>

/* set_pte_range */
#define SPR_SUBSYS_NAME "pte-page-track-spr-hook"

#define SPR_CALLER_FMP_KEY "caller-fmp"
#define SPR_CALLER_DF_KEY "caller-df"
#define SPR_CALLER_FF_KEY "caller-ff"
#define SPR_ENTRY_OK_KEY "entry-ok"
#define SPR_RETURN_OK_KEY "return-ok"
#define SPR_PAGES_OK_KEY "pages-ok"

/* do_wp_page */
#define DWP_SUBSYS_NAME "pte-page-track-dwp-hook"

#define DWP_ENTRY_KEY "entry"
#define DWP_WPR_PATH_TAKEN_KEY "wpr-path-taken"
#define DWP_WPR_MKWRITE_KEY "wpr-caused-by-mkwrite-fault"
#define DWP_WPR_SHARED_KEY "wpr-caused-by-shared"
#define DWP_WPR_ANONEXCL_KEY "wpr-caused-by-anon-excl"
#define DWP_WPR_PRIOR_CHECKS_PASS_KEY "wpr-prior-checks-pass"
#define DWP_WPR_PAGE_OK_KEY "wpr-page-ok"

/* wp_page_copy */
#define WPC_SUBSYS_NAME "pte-page-track-wpc-hook"

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

static int __cow_via_syscall(char *mem)
{
	return trigger_syscall_pagewrite(mem, 10);
}

static int __cow_via_store_instr(char *mem)
{
	spurious_byte_memwrite(mem, 'a');
	return 0;
}

/* do copy-on-write of common page "shared" (ROd) by:
 *  - "the page cache"
 *  - parent
 *  - me (the child)
 *
 * If this is not the case, that is:
 *  - page is not "originally" from the page cache, OR
 *  - A page from the page cache is already written by the parent,
 *   the cowed page is now anonymous
 *
 * ... there are other tests for these cases
 */
static int chld1_docow_pagecache_or_none(char *mem, void *args)
{
	int rv = EXIT_SUCCESS;

	int (*cowfn)(char*) = args;

	die_if(cowfn(mem));

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

	test_int_eq(dwp_entry, 0);
	test_int_eq(dwp_wpr_taken, 0);
	test_int_eq(dwp_wpr_mkwrite, 0);
	test_int_eq(dwp_wpr_shared, 0);
	test_int_eq(dwp_wpr_anonexcl, 0);
	test_int_eq(dwp_wpr_prior_checks_pass, 0);
	test_int_eq(dwp_wpr_page_ok, 0);

	test_int_eq(wpc_entry, 0);
	test_int_eq(wpc_entry_check_pass, 0);
	test_int_eq(wpc_return_ok, 0);
	test_int_eq(wpc_cow_done, 0);

	test_int_eq(spr_caller_fmp, 0);
	test_int_eq(spr_caller_df, 1);
	test_int_eq(spr_caller_ff, 1);
	test_int_eq(spr_entry_ok, 1);
	test_int_eq(spr_return_ok, 1);
	test_int_eq(spr_pages_ok, 1);

__finish:
	return rv;
}

int __child_base(int (*fnchld)(char*, void*), char *mem, void *args) 
{
	int rv;

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

	query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_ENTRY_KEY);
	query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PATH_TAKEN_KEY);
	query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_MKWRITE_KEY);
	query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_SHARED_KEY);
	query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_ANONEXCL_KEY);
	query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PRIOR_CHECKS_PASS_KEY);
	query_int_value_testing_for_me(DWP_SUBSYS_NAME, DWP_WPR_PAGE_OK_KEY);

	query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_KEY);
	query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_ENTRY_CHECKS_PASS_KEY);
	query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_RETURN_OK_KEY);
	query_int_value_testing_for_me(WPC_SUBSYS_NAME, WPC_COW_DONE_KEY);

	query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FMP_KEY);
	query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_DF_KEY);
	query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_CALLER_FF_KEY);
	query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_ENTRY_OK_KEY);
	query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_RETURN_OK_KEY);
	query_int_value_testing_for_me(SPR_SUBSYS_NAME, SPR_PAGES_OK_KEY);

	RESET_ALL();

	rv = fnchld(mem, args);

	RESET_ALL();

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

	/* big-file (10-page-sized file): fork - nonpresent, store instr, read after */
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

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_store_instr));

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): fork - nonpresent, syscall, write after */
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

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_syscall));

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_eq(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_eq(spr_caller_ff, 1);
			test_int_eq(spr_entry_ok, 1);
			test_int_eq(spr_return_ok, 1);
			test_int_eq(spr_pages_ok, 1);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): fork - present (read before), syscall, read after */
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

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_syscall));

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_eq(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 0);
			test_int_eq(spr_caller_ff, 0);
			test_int_eq(spr_entry_ok, 0);
			test_int_eq(spr_return_ok, 0);
			test_int_eq(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): fork - present (read before), store instr, read after */
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

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_store_instr));

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_eq(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 0);
			test_int_eq(spr_caller_ff, 0);
			test_int_eq(spr_entry_ok, 0);
			test_int_eq(spr_return_ok, 0);
			test_int_eq(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): fork - present (read before), syscall, syscall write after */
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

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_syscall));

		RESET_ALL();

		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 'a'));

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

			test_int_eq(dwp_entry, 1);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 1);
			test_int_eq(wpc_entry_check_pass, 1);
			test_int_eq(wpc_return_ok, 1);
			test_int_eq(wpc_cow_done, 1);
	
			test_int_eq(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 0);
			test_int_eq(spr_caller_ff, 0);
			test_int_eq(spr_entry_ok, 0);
			test_int_eq(spr_return_ok, 0);
			test_int_eq(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): fork - present (read before), syscall, write after */
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

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_syscall));

		RESET_ALL();

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

			test_int_eq(dwp_entry, 1);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 1);
			test_int_eq(wpc_entry_check_pass, 1);
			test_int_eq(wpc_return_ok, 1);
			test_int_eq(wpc_cow_done, 1);
	
			test_int_eq(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 0);
			test_int_eq(spr_caller_ff, 0);
			test_int_eq(spr_entry_ok, 0);
			test_int_eq(spr_return_ok, 0);
			test_int_eq(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): fork - present (read before), store instr, store write after */
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

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_store_instr));

		RESET_ALL();

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

			test_int_eq(dwp_entry, 1);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 1);
			test_int_eq(wpc_entry_check_pass, 1);
			test_int_eq(wpc_return_ok, 1);
			test_int_eq(wpc_cow_done, 1);
	
			test_int_eq(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 0);
			test_int_eq(spr_caller_ff, 0);
			test_int_eq(spr_entry_ok, 0);
			test_int_eq(spr_return_ok, 0);
			test_int_eq(spr_pages_ok, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): fork - present (read before), store instr, syscall write after */
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

		RESET_ALL();

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

			test_int_eq(dwp_entry, 0);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
	
			test_int_ge_hard(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 1);
			test_int_ge_hard(spr_caller_ff, 0);
			test_int_ge_hard(spr_entry_ok, 1);
			test_int_ge_hard(spr_return_ok, 1);
			test_int_ge_hard(spr_pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(
					chld1_docow_pagecache_or_none, page_nr(1), __cow_via_store_instr));

		RESET_ALL();

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

			test_int_eq(dwp_entry, 1);
			test_int_eq(dwp_wpr_taken, 0);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 0);
			test_int_eq(dwp_wpr_prior_checks_pass, 0);
			test_int_eq(dwp_wpr_page_ok, 0);

			test_int_eq(wpc_entry, 1);
			test_int_eq(wpc_entry_check_pass, 1);
			test_int_eq(wpc_return_ok, 1);
			test_int_eq(wpc_cow_done, 1);
	
			test_int_eq(spr_caller_fmp, 0);
			test_int_eq(spr_caller_df, 0);
			test_int_eq(spr_caller_ff, 0);
			test_int_eq(spr_entry_ok, 0);
			test_int_eq(spr_return_ok, 0);
			test_int_eq(spr_pages_ok, 0);
		}

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

