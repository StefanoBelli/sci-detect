#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "pte-page-track-fuf-hook"
#define ENTRY_KEY "entry"

#define TEST_SHM_POSIX_NAME "example-shm"
#define TEST_SHM_POSIX_OFLAG O_CREAT | O_RDWR
#define TEST_SHM_POSIX_MODE 0700

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY)

int main()
{
	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

	/* PTE is non-mapped - DO NOT expect any freeing */
	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, 10 * PAGE_SIZE));

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		close(fd);
		shm_unlink(TEST_SHM_POSIX_NAME);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_eq_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* PTE gets populated with zeropage - DO NOT expect any freeing*/
	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, 10 * PAGE_SIZE));

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		spurious_byte_memread(ch, page_nr(1));

		close(fd);
		shm_unlink(TEST_SHM_POSIX_NAME);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* PTE is non-mapped -- syscall read and fixup code! - DO NOT expect any freeing */
	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, 10 * PAGE_SIZE));

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		die_if(trigger_syscall_pageread(page_nr(1), 10));

		close(fd);
		shm_unlink(TEST_SHM_POSIX_NAME);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_eq_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* my own page gets mapped - syscall write - 
	 * EXPECT the free either from current thread's 
	 * kernel control path or another thread */
	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, 10 * PAGE_SIZE));

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		die_if(trigger_syscall_pagewrite(page_nr(1), 10));

		close(fd);
		shm_unlink(TEST_SHM_POSIX_NAME);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* my own page gets mapped - store instruction -
	 * EXPECT the free as explained above */
	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, 10 * PAGE_SIZE));

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		spurious_byte_memwrite(page_nr(1), 'a');

		close(fd);
		shm_unlink(TEST_SHM_POSIX_NAME);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* multiple pages get written - syscall write - 
	 * EXPECT the free */
	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, 10 * PAGE_SIZE));

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		die_if(trigger_syscall_pagewrite(page_nr(1), 10));
		die_if(trigger_syscall_pagewrite(page_nr(2), 10));
		die_if(trigger_syscall_pagewrite(page_nr(5), 30));
		die_if(trigger_syscall_pagewrite(page_nr(7), 40));

		close(fd);
		shm_unlink(TEST_SHM_POSIX_NAME);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* multiple pages are written - store instruction -
	 * EXPECT the free as explained above */
	{
		int fd = shm_open(
				TEST_SHM_POSIX_NAME, TEST_SHM_POSIX_OFLAG, TEST_SHM_POSIX_MODE);

		die_if(fd < 0);

		die_if(ftruncate(fd, 10 * PAGE_SIZE));

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		spurious_byte_memwrite(page_nr(1), 'a');
		spurious_byte_memwrite(page_nr(2), 'a');
		spurious_byte_memwrite(page_nr(5), 'a');
		spurious_byte_memwrite(page_nr(7), 'a');

		close(fd);
		shm_unlink(TEST_SHM_POSIX_NAME);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();
	test_passed();

__finish:
	shm_unlink(TEST_SHM_POSIX_NAME);
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}
