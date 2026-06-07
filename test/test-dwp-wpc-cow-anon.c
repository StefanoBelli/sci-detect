#include "testutils.h"
#include <sys/mman.h>
#include <sys/wait.h>

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



int child1_tests(char* mem)
{
	int rv = EXIT_SUCCESS;

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

	/* TEST after initial read access of non-previously-wrote page */
	{
		spurious_byte_memread(ch, mem + (2 * 4096));

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
	}

	RESET_ALL();

	/* TEST after initial read access of previously-wrote page */
	{
		spurious_byte_memread(ch, mem + 4096);

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
	}

	RESET_ALL();

	/* TEST after initial write access of non-previously-wrote page (CoW of zeropage) */
	{
		spurious_byte_memwrite(mem + (2 * 4096), 'a');

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
	}

	RESET_ALL();

	/* TEST after initial write access of previously-wrote page (CoW of "owned" page) */
	{
		spurious_byte_memwrite(mem + 4096, 'a');

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
	}
__finish:
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

	/* PREPARING: do the mmap */
	char *mem = (char*) mmap(
			NULL, 4 * 4096, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	die_if(mem == MAP_FAILED);

	/* TEST without doing the initial access */
	{
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
	}

	RESET_ALL();

	/* TEST after initial read access */
	{
		spurious_byte_memread(ch, mem + 4096);

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
	}

	RESET_ALL();

	/* TEST after initial write access, CoW of zeropage */
	{
		spurious_byte_memwrite(mem + 4096, 'a');

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
	}

	RESET_ALL();

	/* TEST after second write access, nothing should happen */
	{
		spurious_byte_memwrite(mem + 4096, 'a');

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
	}

	/* TEST fork() CoW */
	{
		RESET_ALL();

		{
			spurious_byte_memwrite(mem + 4096, 'a');

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
		}

		pid_t child_pid = fork();
		if(!child_pid)
			exit(child1_tests(mem));
		
		int status;
		die_if(child_pid < 0);
		die_if(waitpid(child_pid, &status, 0) < 0);
		die_if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS);

		RESET_ALL();

		/* TEST read after fork() of wrote page (no need to reuse, do it when trying to write...) */
		{
			spurious_byte_memread(ch, mem + 4096);

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
		}

		RESET_ALL();

		/* TEST write after fork() of wrote page (reuse RO-d page to actuate CoW) */
		{
			spurious_byte_memwrite(mem + 4096, 'a');

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

			/* this fails sometime... */
			test_int_eq(dwp_entry, 1);
			test_int_eq(dwp_wpr_taken, 1);
			test_int_eq(dwp_wpr_mkwrite, 0);
			test_int_eq(dwp_wpr_shared, 0);
			test_int_eq(dwp_wpr_anonexcl, 1);
			test_int_eq(dwp_wpr_prior_checks_pass, 1);
			test_int_eq(dwp_wpr_page_ok, 1);

			test_int_eq(wpc_entry, 0);
			test_int_eq(wpc_entry_check_pass, 0);
			test_int_eq(wpc_return_ok, 0);
			test_int_eq(wpc_cow_done, 0);
		}

		RESET_ALL();

		/* TEST read after fork() of non-wrote page (no need to reuse, do it when trying to write...) */
		{
			spurious_byte_memread(ch, mem);

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
		}

		RESET_ALL();

		/* TEST write after fork() of non wrote page (CoW of zeropage, no reuse of "owned" page) */
		{
			spurious_byte_memwrite(mem, 'a');

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
		}

	}

	test_passed();

__finish:
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

