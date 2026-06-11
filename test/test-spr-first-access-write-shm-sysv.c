#include "testutils.h"
#include <sys/mman.h>
#include <sys/shm.h>

#define SUBSYS_NAME "pte-page-track-spr-hook"

#define CALLER_FMP_KEY "caller-fmp"
#define CALLER_DF_KEY "caller-df"
#define CALLER_FF_KEY "caller-ff"
#define ENTRY_OK_KEY "entry-ok"
#define RETURN_OK_KEY "return-ok"
#define PAGES_OK_KEY "pages-ok"

#define TEST_SHMGET_SYSV_KEY 0xdeadbeef
#define TEST_SHMGET_SYSV_SIZE 30 * PAGE_SIZE
#define TEST_SHMGET_SYSV_FLG IPC_CREAT | IPC_EXCL

#define TEST_SHMAT_SYSV_FLG 0

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY)

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

	int shmid = shmget(
			TEST_SHMGET_SYSV_KEY, 
			TEST_SHMGET_SYSV_SIZE, 
			TEST_SHMGET_SYSV_FLG);
	die_if(shmid < 0);

	char *mem = (char*) shmat(shmid, NULL, TEST_SHMAT_SYSV_FLG);
	die_if(mem == (void*) -1);

	/* TEST no initial access - something strange happens */
	{
		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_ge_hard(caller_fmp, 0);
		test_int_ge_hard(caller_df, 0);
		test_int_ge_hard(caller_ff, 0);
		test_int_ge_hard(entry_ok, 0);
		test_int_ge_hard(return_ok, 0);
		test_int_ge_hard(pages_ok, 0);
	}

	RESET_ALL();

	/* TEST initial write access */
	{
		spurious_byte_memwrite(page_nr(1), 'a');

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq(caller_df, 1);
		test_int_eq_hard(caller_ff, 1);
		test_int_eq_hard(entry_ok, 1);
		test_int_eq_hard(return_ok, 1);
		test_int_eq_hard(pages_ok, 1);
	}

	RESET_ALL();

	/* TEST second write access */
	{
		spurious_byte_memwrite(page_nr(1), 'a');

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

	RESET_ALL();

	/* TEST third read access */
	{
		spurious_byte_memread(ch, page_nr(1));

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

	RESET_ALL();

	/* TEST initial write access on another page */
	{
		spurious_byte_memwrite(page_nr(2), 'a');

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 0);
		test_int_eq(caller_df, 1);
		test_int_eq_hard(caller_ff, 1);
		test_int_eq_hard(entry_ok, 1);
		test_int_eq_hard(return_ok, 1);
		test_int_eq_hard(pages_ok, 1);
	}

	RESET_ALL();

	/* TEST second read access on another page */
	{
		spurious_byte_memread(ch, page_nr(2));

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

	RESET_ALL();

	/* TEST first write access with a syscall, on another page */
	{
		die_if(trigger_syscall_pagewrite(page_nr(2), 'a'));

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

	RESET_ALL();

	/* TEST first write access with a syscall, on a third page */
	{
		die_if(trigger_syscall_pagewrite(page_nr(3), 'a'));

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

	RESET_ALL();

	/* TEST second write access with a syscall, on a third page */
	{
		die_if(trigger_syscall_pagewrite(page_nr(3), 'a'));

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

	RESET_ALL();

	/* TEST third read access with a syscall, on a third page */
	{
		die_if(trigger_syscall_pageread(page_nr(3), 10));

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
	shmdt(mem);
	shmctl(shmid, IPC_RMID, NULL);

	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}

