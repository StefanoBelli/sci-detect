#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "add-spr-hook"

#define CALLER_FMP_KEY "caller-fmp"
#define CALLER_DF_KEY "caller-df"
#define CALLER_FF_KEY "caller-ff"
#define ENTRY_OK_KEY "entry-ok"
#define RETURN_OK_KEY "return-ok"
#define PAGES_OK_KEY "pages-ok"

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

	/* small-file (single-page-sized file): first access by load instruction */
	{
		int fd = open("res/small-file", O_RDONLY, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ, 
				MAP_PRIVATE, 
				fd, 0);

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

		RESET_ALL();

		/* test first read access -- may, or may not, fault around (filemap_map_pages) */
		{
			spurious_byte_memread(ch, page_nr(1));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_eq_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_eq_hard(entry_ok, 1);
			test_int_eq_hard(return_ok, 1);
			test_int_eq_hard(pages_ok, 1);
		}

		RESET_ALL();

		/* test second read access -- do nothing */
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

		/* test third read access via syscall -- do nothing */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

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

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* small-file (single-page-sized file): first access by syscall */
	{
		int fd = open("res/small-file", O_RDONLY, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, PAGE_SIZE, 
				PROT_READ, 
				MAP_PRIVATE, 
				fd, 
				0);

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

		RESET_ALL();

		/* test first read access via syscall -- i
		 * kernel will probably directly read from page cache without faulting */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

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

		/* test second read access -- here we actually setup PTEs */
		{
			spurious_byte_memread(ch, page_nr(1));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_eq_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_eq_hard(entry_ok, 1);
			test_int_eq_hard(return_ok, 1);
			test_int_eq_hard(pages_ok, 1);
		}

		RESET_ALL();

		/* test third read access -- do nothing */
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

		/* test fourth read access via syscall */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

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

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): first access by load instruction */
	{
		int fd = open("res/big-file", O_RDONLY, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ, 
				MAP_PRIVATE, 
				fd, 0);

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

		RESET_ALL();

		/* test first read access --
		 * may, or may not, fault around (filemap_map_pages)
		 * to set multiple PTEs for the file (a part, or all of it) */
		{
			spurious_byte_memread(ch, page_nr(1));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_eq_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, 1);
			test_int_ge_hard(return_ok, 1);
			test_int_ge_hard(pages_ok, 1);
		}

		RESET_ALL();

		/* test second read access -- do nothing for sure */
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

		/* test third read access via syscall -- do nothing for sure */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

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

		/* test first read access on third page -- what this does depends on what kernel 
		 * did earlier: 
		 *  * it may do nothing at all (faulting around paid off)
		 *  * the #PF handler may be called for this unmapped pag
		 *   - do map only this page
		 *   - faulting around mapping whole file
		 *   - faulting around mapping part of the file
		 *
		 * it depends, not our chioce...
		 */
		{
			spurious_byte_memread(ch, page_nr(3));

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

		/* again, what this does is randomic, see desc. above, same thing. */
		{
			spurious_byte_memread(ch, page_nr(7));

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

		munmap(mem, PAGE_SIZE);
		close(fd);
	}

	/* big-file (10-page-sized file): first access by syscall */
	{
		int fd = open("res/big-file", O_RDONLY, 0);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 
				10 * PAGE_SIZE, 
				PROT_READ, 
				MAP_PRIVATE, 
				fd, 
				0);

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

		RESET_ALL();

		/* test first read access via syscall -- 
		 * kernel will probably directly read from page cache without faulting/setting any PTEs */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

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

		/* test second read access with load instr 
		 * -- faulting around makes this unpredictable (however, at least one PTE being 
		 *  set by one kernel control path or the other...) */
		{
			spurious_byte_memread(ch, page_nr(1));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_eq_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, 1);
			test_int_ge_hard(return_ok, 1);
			test_int_ge_hard(pages_ok, 1);
		}

		RESET_ALL();

		/* test third read access -- do nothing 
		 * (because we're on the same page that was FOR SURE mapped earlier) */
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

		/* test fourth read access via syscall -- again on same page => do nothing */
		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

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

		/* test read on third page -- may do fault around, unpredictable */
		{
			spurious_byte_memread(ch, page_nr(3));

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

		/* again, what this does is randomic, see desc. above, same thing. */
		{
			spurious_byte_memread(ch, page_nr(7));

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

		munmap(mem, PAGE_SIZE);
		close(fd);
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

