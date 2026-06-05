#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "add-spr-hook"

#define CALLER_FMP_KEY "caller-fmp"
#define CALLER_DF_KEY "caller-df"
#define CALLER_FF_KEY "caller-ff"
#define ENTRY_OK_KEY "entry-ok"
#define RETURN_OK_KEY "return-ok"
#define PAGES_OK_KEY "pages-ok"

int main()
{
	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	start_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	start_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	start_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	start_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* PREPARING: do the mmap */
	char *mem = (char*) mmap(
			NULL, 30 * 4096, 
			PROT_READ | PROT_WRITE, 
			MAP_SHARED | MAP_ANONYMOUS, 
			-1, 0);
	die_if(mem == MAP_FAILED);

	/* TEST no initial access - something strange happens */
	{
		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_ge_hard(caller_df, 0);
		test_int_ge_hard(caller_ff, 0);
		test_int_ge_hard(entry_ok, 0);
		test_int_ge_hard(return_ok, 0);
		test_int_ge_hard(pages_ok, 0);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST initial write access */
	{
		spurious_byte_memwrite(mem, 'a');

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 1);
		test_int_eq_hard(caller_ff, 1);
		test_int_eq_hard(entry_ok, 1);
		test_int_eq_hard(return_ok, 1);
		test_int_eq_hard(pages_ok, 1);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST second write access */
	{
		spurious_byte_memwrite(mem, 'a');

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 0);
		test_int_eq_hard(caller_ff, 0);
		test_int_eq_hard(entry_ok, 0);
		test_int_eq_hard(return_ok, 0);
		test_int_eq_hard(pages_ok, 0);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST third read access */
	{
		spurious_byte_memread(ch, mem);

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 0);
		test_int_eq_hard(caller_ff, 0);
		test_int_eq_hard(entry_ok, 0);
		test_int_eq_hard(return_ok, 0);
		test_int_eq_hard(pages_ok, 0);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST initial write access on another page */
	{
		spurious_byte_memwrite(mem + 4097, 'a');

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 1);
		test_int_eq_hard(caller_ff, 1);
		test_int_eq_hard(entry_ok, 1);
		test_int_eq_hard(return_ok, 1);
		test_int_eq_hard(pages_ok, 1);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST second read access on another page */
	{
		spurious_byte_memread(ch, mem + 4099);

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 0);
		test_int_eq_hard(caller_ff, 0);
		test_int_eq_hard(entry_ok, 0);
		test_int_eq_hard(return_ok, 0);
		test_int_eq_hard(pages_ok, 0);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST first write access with a syscall, on another page */
	{
		die_if(trigger_syscall_pagewrite(mem + 4100, 'a'));

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 0);
		test_int_eq_hard(caller_ff, 0);
		test_int_eq_hard(entry_ok, 0);
		test_int_eq_hard(return_ok, 0);
		test_int_eq_hard(pages_ok, 0);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST first write access with a syscall, on a third page */
	{
		die_if(trigger_syscall_pagewrite(mem + 8194, 'a'));

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 1);
		test_int_eq_hard(caller_ff, 1);
		test_int_eq_hard(entry_ok, 1);
		test_int_eq_hard(return_ok, 1);
		test_int_eq_hard(pages_ok, 1);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST second write access with a syscall, on a third page */
	{
		die_if(trigger_syscall_pagewrite(mem + 8194, 'a'));

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 0);
		test_int_eq_hard(caller_ff, 0);
		test_int_eq_hard(entry_ok, 0);
		test_int_eq_hard(return_ok, 0);
		test_int_eq_hard(pages_ok, 0);
	}

	/* this case needs more clarity, we avoid accumulating values */
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* TEST third read access with a syscall, on a third page */
	{
		die_if(trigger_syscall_pageread(mem + 8194, 10));

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq_hard(caller_df, 0);
		test_int_eq_hard(caller_ff, 0);
		test_int_eq_hard(entry_ok, 0);
		test_int_eq_hard(return_ok, 0);
		test_int_eq_hard(pages_ok, 0);
	}

	test_passed();

__finish:

	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}

