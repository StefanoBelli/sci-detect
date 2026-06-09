#include "testutils.h"
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SUBSYS_NAME "add-spr-hook"

#define CALLER_FMP_KEY "caller-fmp"
#define CALLER_DF_KEY "caller-df"
#define CALLER_FF_KEY "caller-ff"
#define ENTRY_OK_KEY "entry-ok"
#define RETURN_OK_KEY "return-ok"
#define PAGES_OK_KEY "pages-ok"

#define TEST_SHM_POSIX_NAME "testshmposix"
#define TEST_SHM_POSIX_OFLAG O_RDWR | O_CREAT
#define TEST_SHM_POSIX_MODE S_IRUSR | S_IWUSR

#define TEST_SHM_POSIX_NR_PAGES 30

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY)

static int chld5_dosyswrite_present(char *mem)
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

static int chld4_dosysread_present(char *mem)
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

static int chld3_dowrite_present(char *mem)
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
	test_int_eq_hard(caller_df, 1);
	test_int_eq_hard(caller_ff, 1);
	test_int_eq_hard(entry_ok, 1);
	test_int_eq_hard(return_ok, 1);
	test_int_eq_hard(pages_ok, 1);
__finish:
	return rv;
}

static int chld2_doread_present(char *mem)
{
	int rv = EXIT_SUCCESS;

	spurious_byte_memread(ch, mem);

	int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

	/* this means "fault around" -- driven by filemap_map_pages */
	test_int_eq_hard(caller_fmp, 1);
	test_int_eq_hard(caller_df, 1);
	test_int_eq_hard(caller_ff, 0);
	test_int_eq(entry_ok, 1);
	test_int_eq(return_ok, 1);
	test_int_eq(pages_ok, 1);
__finish:
	return rv;
}

static int chld1_donothing_present(__unused char *mem)
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

	int fd = shm_open(
			TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

	die_if(fd < 0);

	die_if(ftruncate(fd, TEST_SHM_POSIX_NR_PAGES * PAGE_SIZE));

	/* PREPARING: do the mmap */
	char *mem = (char*) mmap(
			NULL, 
			TEST_SHM_POSIX_NR_PAGES * PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_SHARED, 
			fd, 0);
	die_if(mem == MAP_FAILED);

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

	/* TEST child process shares a present page, does nothing on it */
	{
		spurious_byte_memread(ch, page_nr(1));

		int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
		int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
		int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
		int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
		int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
		int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

		test_int_eq_hard(caller_fmp, 1);
		test_int_eq_hard(caller_df, 1);
		test_int_eq_hard(caller_ff, 1);
		test_int_eq_hard(entry_ok, 1);
		test_int_eq_hard(return_ok, 1);
		test_int_eq_hard(pages_ok, 1);

		__test_fork_and_wait(__child_base(chld1_donothing_present, page_nr(1)));
	}

	RESET_ALL();

	/* TEST child process shares a present page, does a read on it */
	{
		{
			spurious_byte_memread(ch, page_nr(2));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_eq_hard(caller_fmp, 1);
			test_int_eq_hard(caller_df, 1);
			test_int_eq_hard(caller_ff, 1);
			test_int_eq_hard(entry_ok, 1);
			test_int_eq_hard(return_ok, 1);
			test_int_eq_hard(pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(chld2_doread_present, page_nr(2)));

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

		/* do a read on the previously-written-page after the fork */
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
			test_int_eq_hard(caller_df, 1);
			test_int_eq_hard(caller_ff, 1);
			test_int_eq_hard(entry_ok, 1);
			test_int_eq_hard(return_ok, 1);
			test_int_eq_hard(pages_ok, 1);
		}

	}

	/* TEST child process shares a present page, writes it */
	{
		RESET_ALL();

		{
			spurious_byte_memread(ch, page_nr(5));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_eq_hard(caller_fmp, 1);
			test_int_eq_hard(caller_df, 1);
			test_int_eq_hard(caller_ff, 1);
			test_int_eq_hard(entry_ok, 1);
			test_int_eq_hard(return_ok, 1);
			test_int_eq_hard(pages_ok, 1);
		}

		__test_fork_and_wait(__child_base(chld3_dowrite_present, page_nr(5)));

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
			test_int_eq_hard(caller_df, 0);
			test_int_eq_hard(caller_ff, 0);
			test_int_eq_hard(entry_ok, 0);
			test_int_eq_hard(return_ok, 0);
			test_int_eq_hard(pages_ok, 0);
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

	/* TEST child process shares a present page, reads it with a syscall */
	{
		RESET_ALL();

		{
			spurious_byte_memwrite(page_nr(6), 'a');

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

		__test_fork_and_wait(__child_base(chld4_dosysread_present, page_nr(6)));

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
			test_int_eq_hard(caller_df, 0);
			test_int_eq_hard(caller_ff, 0);
			test_int_eq_hard(entry_ok, 0);
			test_int_eq_hard(return_ok, 0);
			test_int_eq_hard(pages_ok, 0);
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

	/* TEST child process shares a present page, writes it with a syscall */
	{
		RESET_ALL();

		{
			die_if(trigger_syscall_pagewrite(page_nr(7), 10));

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

		__test_fork_and_wait(__child_base(chld5_dosyswrite_present, page_nr(7)));

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
			test_int_eq_hard(caller_df, 0);
			test_int_eq_hard(caller_ff, 0);
			test_int_eq_hard(entry_ok, 0);
			test_int_eq_hard(return_ok, 0);
			test_int_eq_hard(pages_ok, 0);
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

	shm_unlink(TEST_SHM_POSIX_NAME);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}

