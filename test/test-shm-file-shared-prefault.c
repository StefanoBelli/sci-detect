/*
 * number of tests is reduced, why?
 * 
 * from man mmap:
 * 
 * MAP_POPULATE (since Linux 2.5.46)
 *             Populate  (prefault)  page tables for a mapping.  For a file map‐
 *             ping, this causes read-ahead on the file.  This will help to  re‐
 *             duce blocking on page faults later.  The mmap() call doesn’t fail
 *             if  the  mapping cannot be populated 
 *  
 * Results of prefaulting is unpredictable with file-backed mapping
 * (focus on "The mmap() call doesn't fail if the mapping cannot be populated")
 */
#include "testutils.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SUBSYS_NAME "pte-page-track-spr-hook"

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

#define NUM_PAGES 10

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

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		RESET_ALL();

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED | MAP_POPULATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		RESET_ALL();

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ, 
				MAP_SHARED | MAP_POPULATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		RESET_ALL();

		die_if(madvise(mem, NUM_PAGES * PAGE_SIZE, MADV_POPULATE_READ));

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		RESET_ALL();

		die_if(madvise(mem, NUM_PAGES * PAGE_SIZE, MADV_POPULATE_READ | MADV_POPULATE_WRITE));

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		RESET_ALL();

		die_if(madvise(mem, NUM_PAGES * PAGE_SIZE, MADV_POPULATE_READ | MADV_POPULATE_WRITE));

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ, 
				MAP_SHARED, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		RESET_ALL();

		die_if(madvise(mem, (NUM_PAGES >> 1) * PAGE_SIZE, MADV_POPULATE_READ));

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES >> 1);
			test_int_ge_hard(return_ok, NUM_PAGES >> 1);
			test_int_ge_hard(pages_ok, NUM_PAGES >> 1);	
		}		

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		RESET_ALL();

		die_if(madvise(
					page_nr(NUM_PAGES >> 1), 
					(NUM_PAGES >> 2) * PAGE_SIZE, 
					MADV_POPULATE_WRITE | MADV_POPULATE_READ));

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES >> 2);
			test_int_ge_hard(return_ok, NUM_PAGES >> 2);
			test_int_ge_hard(pages_ok, NUM_PAGES >> 2);	
		}		

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		RESET_ALL();

		die_if(madvise(
					page_nr(NUM_PAGES >> 1), 
					(NUM_PAGES >> 2) * PAGE_SIZE, 
					MADV_POPULATE_WRITE | MADV_POPULATE_READ));

		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_eq(caller_df, NUM_PAGES >> 2);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES >> 2);
			test_int_ge_hard(return_ok, NUM_PAGES >> 2);
			test_int_ge_hard(pages_ok, NUM_PAGES >> 2);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		close(fd);
	}

	RESET_ALL();

	{
		int fd = open("res/big-file", O_RDWR, 0);
		die_if(fd < 0);

		char *mem = (char*) mmap(
				NULL, NUM_PAGES * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);

		die_if(mem == MAP_FAILED);

		RESET_ALL();

		die_if(madvise(
					page_nr(NUM_PAGES >> 1), 
					(NUM_PAGES >> 2) * PAGE_SIZE, 
					MADV_POPULATE_WRITE | MADV_POPULATE_READ));

		{
			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_ge_hard(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES >> 2);
			test_int_ge_hard(return_ok, NUM_PAGES >> 2);
			test_int_ge_hard(pages_ok, NUM_PAGES >> 2);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
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

