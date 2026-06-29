#include "testutils.h"
#include <sys/mman.h>
#include <sys/shm.h>

#define SUBSYS_NAME "pte-page-track-fuf-hook"
#define ENTRY_KEY "entry"

#define TEST_SHM_SYSV_KEY 0xdeadbeef
#define TEST_SHM_SYSV_SIZE (10 * PAGE_SIZE)
#define TEST_SHM_SYSV_FLG (IPC_CREAT | IPC_EXCL)

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY)

int main()
{
	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

	/* PTE is non-mapped - DO NOT expect any freeing */
	{
		int shmid = shmget(
				TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);
		die_if(shmid < 0);

		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(mem);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_eq_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* PTE gets populated with zeropage - DO NOT expect any freeing*/
	{
		int shmid = shmget(
				TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);
		die_if(shmid < 0);

		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		spurious_byte_memread(ch, page_nr(1));

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(mem);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* PTE is non-mapped -- syscall read and fixup code! - DO NOT expect any freeing */
	{
		int shmid = shmget(
				TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);
		die_if(shmid < 0);

		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		die_if(trigger_syscall_pageread(page_nr(1), 10));

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(mem);

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
		int shmid = shmget(
				TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);
		die_if(shmid < 0);

		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		die_if(trigger_syscall_pagewrite(page_nr(1), 10));

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(mem);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* my own page gets mapped - store instruction -
	 * EXPECT the free as explained above */
	{
		int shmid = shmget(
				TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);
		die_if(shmid < 0);

		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		spurious_byte_memwrite(page_nr(1), 'a');

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(mem);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* multiple pages get written - syscall write - 
	 * EXPECT the free */
	{
		int shmid = shmget(
				TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);
		die_if(shmid < 0);

		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		die_if(trigger_syscall_pagewrite(page_nr(1), 10));
		die_if(trigger_syscall_pagewrite(page_nr(2), 10));
		die_if(trigger_syscall_pagewrite(page_nr(5), 30));
		die_if(trigger_syscall_pagewrite(page_nr(7), 40));

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(mem);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_ge_hard(entry, 0);
		}
	}

	RESET_ALL();

	/* multiple pages are written - store instruction -
	 * EXPECT the free as explained above */
	{
		int shmid = shmget(
				TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);
		die_if(shmid < 0);

		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		spurious_byte_memwrite(page_nr(1), 'a');
		spurious_byte_memwrite(page_nr(2), 'a');
		spurious_byte_memwrite(page_nr(5), 'a');
		spurious_byte_memwrite(page_nr(7), 'a');

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(mem);

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
