#include "testutils.h"
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define SUBSYS_NAME "add-spr-hook"

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

static int chld5_dosyswrite_nonpresent(char *mem)
{
	int rv = EXIT_SUCCESS;

	die_if(trigger_syscall_pagewrite(mem, 10));

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
__finish:
	return rv;
}

static int chld4_dosysread_nonpresent(char *mem)
{
	int rv = EXIT_SUCCESS;

	die_if(trigger_syscall_pageread(mem, 10));

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
__finish:
	return rv;
}

static int chld3_dowrite_nonpresent(char *mem)
{
	int rv = EXIT_SUCCESS;

	spurious_byte_memwrite(mem, 'a');

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
__finish:
	return rv;
}

static int chld2_doread_nonpresent(char *mem)
{
	int rv = EXIT_SUCCESS;

	spurious_byte_memread(ch, mem);

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
__finish:
	return rv;
}

static int chld1_donothing_nonpresent(__unused char *mem)
{
	int rv = EXIT_SUCCESS;

	int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	test_int_ge_hard(caller_fmp, 0);
	test_int_ge_hard(caller_df, 0);
	test_int_eq_hard(caller_ff, 0);
	test_int_ge_hard(entry_ok, 0);
	test_int_ge_hard(return_ok, 0);
	test_int_ge_hard(pages_ok, 0);
__finish:
	return rv;
}

int __child_base(int (*fnchld)(char*), char *mem) 
{
	int rv;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	start_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	start_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	start_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	start_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* doing this to flush values */
	query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	RESET_ALL();

	rv = fnchld(mem);

	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}

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

	/* TEST no initial access - something strange happens - keep this */
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

	/* TEST child process shares a non-present page, does nothing on it */
	{
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

		__test_fork_and_wait(__child_base(chld1_donothing_nonpresent, page_nr(1)));
	}

	RESET_ALL();

	/* TEST child process shares a non-present page, does a read on it */
	{
		{
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

		__test_fork_and_wait(__child_base(chld2_doread_nonpresent, page_nr(1)));

		RESET_ALL();

		/* do nothing after the fork */
		{
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

		/* do a write on a page after the fork */
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

		/* do a read on the previously-written-page after the fork */
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

		/* do a read first on another page after the fork */
		{
			spurious_byte_memread(ch, page_nr(2));

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

		/* do a write secondly, on another page after the fork */
		{
			spurious_byte_memwrite(page_nr(2), 'a');

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

		/* do a write with a syscall on a third page after the fork */
		{
			die_if(trigger_syscall_pagewrite(page_nr(3), 10));

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

		/* do a read on the previously-written-page, with a syscall, after the fork */
		{
			spurious_byte_memread(ch, page_nr(3));

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

		/* do a read first with a syscall, on a fourth page after the fork */
		{
			die_if(trigger_syscall_pageread(page_nr(4), 10));

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

		/* do a write secondly, on a fourth page, after the fork */
		{
			spurious_byte_memwrite(page_nr(4), 'a');

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

	}

	/* TEST child process shares a non-present page, writes it */
	{
		__test_fork_and_wait(__child_base(chld3_dowrite_nonpresent, page_nr(5)));

		RESET_ALL();

		/* do a write on a page after the fork */
		{
			spurious_byte_memwrite(page_nr(5), 'a');

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

		/* do a read on the previously-written-page after the fork */
		{
			spurious_byte_memread(ch, page_nr(5));

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

	}

	/* TEST child process shares a non-present page, reads it with a syscall */
	{
		__test_fork_and_wait(__child_base(chld4_dosysread_nonpresent, page_nr(6)));

		RESET_ALL();

		/* do a write on a page after the fork */
		{
			spurious_byte_memwrite(page_nr(6), 'a');

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

		/* do a read on the previously-written-page after the fork */
		{
			spurious_byte_memread(ch, page_nr(6));

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
	}

	/* TEST child process shares a non-present page, writes it with a syscall */
	{
		__test_fork_and_wait(__child_base(chld5_dosyswrite_nonpresent, page_nr(7)));

		RESET_ALL();

		/* do a read on the previously-written-page after the fork */
		{
			spurious_byte_memread(ch, page_nr(7));

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

		/* do a write on a page after the fork */
		{
			spurious_byte_memwrite(page_nr(7), 'a');

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

