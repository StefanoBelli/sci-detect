#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "pte-page-track-fuf-hook"
#define ENTRY_KEY "entry"

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY)

int main()
{
	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

	/* PTE is non-mapped - DO NOT expect any freeing */
	{
		int fd = open("res/big-file", O_RDWR, 0700);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		close(fd);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_eq_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* PTE gets populated with zeropage - DO NOT expect any freeing*/
	{
		int fd = open("res/big-file", O_RDWR, 0700);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		spurious_byte_memread(ch, page_nr(1));

		close(fd);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* PTE is non-mapped -- syscall read and fixup code! - DO NOT expect any freeing */
	{
		int fd = open("res/big-file", O_RDWR, 0700);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		die_if(trigger_syscall_pageread(page_nr(1), 10));

		close(fd);
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
		int fd = open("res/big-file", O_RDWR, 0700);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		die_if(trigger_syscall_pagewrite(page_nr(1), 10));

		close(fd);
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
		int fd = open("res/big-file", O_RDWR, 0700);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		spurious_byte_memwrite(page_nr(1), 'a');

		close(fd);
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
		int fd = open("res/big-file", O_RDWR, 0700);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		die_if(trigger_syscall_pagewrite(page_nr(1), 10));
		die_if(trigger_syscall_pagewrite(page_nr(2), 10));
		die_if(trigger_syscall_pagewrite(page_nr(5), 30));
		die_if(trigger_syscall_pagewrite(page_nr(7), 40));

		close(fd);
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
		int fd = open("res/big-file", O_RDWR, 0700);
		die_if(fd < 0);

		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE, 
				fd, 0);
		die_if(mem == MAP_FAILED);

		spurious_byte_memwrite(page_nr(1), 'a');
		spurious_byte_memwrite(page_nr(2), 'a');
		spurious_byte_memwrite(page_nr(5), 'a');
		spurious_byte_memwrite(page_nr(7), 'a');

		close(fd);
		munmap(mem, 10 * PAGE_SIZE);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();
	test_passed();

__finish:
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}
