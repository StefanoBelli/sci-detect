#define SOFT_FAIL_TOLERANCE 2

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

#define NUM_PAGES 22

#define TEST_SHM_POSIX_NAME "testshmposix"
#define TEST_SHM_POSIX_OFLAG O_RDWR | O_CREAT
#define TEST_SHM_POSIX_MODE S_IRUSR | S_IWUSR

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
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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
			test_int_eq(caller_df, NUM_PAGES);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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
			test_int_eq(caller_df, NUM_PAGES);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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
			test_int_eq(caller_df, NUM_PAGES);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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
			test_int_eq(caller_df, NUM_PAGES);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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
			test_int_eq(caller_df, NUM_PAGES);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES);
			test_int_ge_hard(return_ok, NUM_PAGES);
			test_int_ge_hard(pages_ok, NUM_PAGES);	
		}

		RESET_ALL();

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

		{	
			spurious_byte_memwrite(page_nr(NUM_PAGES >> 1), 'a');

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

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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
			test_int_eq(caller_df, NUM_PAGES >> 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES >> 1);
			test_int_ge_hard(return_ok, NUM_PAGES >> 1);
			test_int_ge_hard(pages_ok, NUM_PAGES >> 1);	
		}		

		RESET_ALL();

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) + 1));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_eq(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, 1);
			test_int_ge_hard(return_ok, 1);
			test_int_ge_hard(pages_ok, 1);	
		}

		RESET_ALL();

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) + 1));

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) + 2));

			int caller_fmp = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FMP_KEY);
			int caller_df = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_DF_KEY);
			int caller_ff = query_int_value_testing_for_me(SUBSYS_NAME, CALLER_FF_KEY);
			int entry_ok = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_OK_KEY);
			int return_ok = query_int_value_testing_for_me(SUBSYS_NAME, RETURN_OK_KEY);
			int pages_ok = query_int_value_testing_for_me(SUBSYS_NAME, PAGES_OK_KEY);

			test_int_ge_hard(caller_fmp, 0);
			test_int_eq(caller_df, 1);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, 1);
			test_int_ge_hard(return_ok, 1);
			test_int_ge_hard(pages_ok, 1);
		}

		RESET_ALL();

		{
			die_if(trigger_syscall_pageread(page_nr((NUM_PAGES >> 1) + 2), 10));

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

		{
			die_if(trigger_syscall_pageread(page_nr((NUM_PAGES >> 1) + 3), 10));

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

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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
			test_int_eq(caller_df, NUM_PAGES >> 2);
			test_int_ge_hard(caller_ff, 0);
			test_int_ge_hard(entry_ok, NUM_PAGES >> 2);
			test_int_ge_hard(return_ok, NUM_PAGES >> 2);
			test_int_ge_hard(pages_ok, NUM_PAGES >> 2);	
		}		

		RESET_ALL();

		{
			die_if(trigger_syscall_pageread(page_nr(NUM_PAGES >> 1), 10));

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1)));

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

		{
			spurious_byte_memwrite(page_nr((NUM_PAGES >> 1)), 'a');

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

		{
			die_if(trigger_syscall_pagewrite(page_nr((NUM_PAGES >> 1) + 1), 10));

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) + 1));

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

		{
			spurious_byte_memwrite(page_nr((NUM_PAGES >> 1) + 1), 'a');

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

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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

		RESET_ALL();

		{
			spurious_byte_memwrite(page_nr((NUM_PAGES >> 1) - 1), 'a');

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

		{
			spurious_byte_memwrite(page_nr((NUM_PAGES >> 1) - 1), 'a');

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) - 1));

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) + (NUM_PAGES >> 2) + 1));

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

		{
			spurious_byte_memwrite(page_nr((NUM_PAGES >> 1) + (NUM_PAGES >> 2) + 1), 'a');

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) + (NUM_PAGES >> 2) + 1));

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

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
	}

	RESET_ALL();

	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, 
				TEST_SHM_POSIX_OFLAG, 
				TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, NUM_PAGES * PAGE_SIZE));

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

		RESET_ALL();

		{
			die_if(trigger_syscall_pageread(page_nr((NUM_PAGES >> 1) - 1), 'a'));

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

		{
			spurious_byte_memwrite(page_nr((NUM_PAGES >> 1) - 1), 'a');

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) - 1));

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

		{
			die_if(trigger_syscall_pagewrite(page_nr((NUM_PAGES >> 1) + (NUM_PAGES >> 2) + 1), 10));

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

		{
			spurious_byte_memwrite(page_nr((NUM_PAGES >> 1) + (NUM_PAGES >> 2) + 1), 'a');

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

		{
			spurious_byte_memread(ch, page_nr((NUM_PAGES >> 1) + (NUM_PAGES >> 2) + 1));

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

		munmap(mem, NUM_PAGES * PAGE_SIZE);
		shm_unlink(TEST_SHM_POSIX_NAME);
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

