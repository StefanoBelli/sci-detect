#include "testutils.h"
#include <sys/mman.h>
#include <sys/shm.h>

#define SUBSYS_NAME "pte-page-track-cpr-hook"
#define ENTRY_KEY "entry"
#define RETURNOK_KEY "return-ok"
#define PAGESOK_KEY "pages-ok"

#define TEST_SHM_SYSV_KEY 0xdeadbeef
#define TEST_SHM_SYSV_SIZE (10 * PAGE_SIZE)
#define TEST_SHM_SYSV_FLG (IPC_CREAT | IPC_EXCL)

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY); \

int main()
{
	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	start_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	start_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

	int shmid = shmget(
			TEST_SHM_SYSV_KEY, TEST_SHM_SYSV_SIZE, TEST_SHM_SYSV_FLG);

	die_if(shmid < 0);

	/* do nothing - cpr must not be called */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 0);
			test_int_eq_hard(returnok, 0);
			test_int_eq_hard(pagesok, 0);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access read - cpr must not be called */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			spurious_byte_memread(ch, page_nr(1));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 0);
			test_int_eq_hard(returnok, 0);
			test_int_eq_hard(pagesok, 0);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access write - cpr must not be called */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			spurious_byte_memwrite(page_nr(1), 'a');

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 0);
			test_int_eq_hard(returnok, 0);
			test_int_eq_hard(pagesok, 0);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access via read syscall - cpr must not be called */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 0);
			test_int_eq_hard(returnok, 0);
			test_int_eq_hard(pagesok, 0);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access via write syscall - cpr must not be called */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 0);
			test_int_eq_hard(returnok, 0);
			test_int_eq_hard(pagesok, 0);
		}

		shmdt(mem);
	}

	/* do nothing, so do mprotect - cpr will be called, but nothing changes (no mapped page) */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			die_if(mprotect(page_nr(1), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_ge_hard(entry, 0);
			test_int_eq_hard(returnok, 0);
			test_int_eq_hard(pagesok, 0);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access read - cpr will be called, change takes effect (zeropage is mapped)*/
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			spurious_byte_memread(ch, page_nr(1));
			die_if(mprotect(page_nr(1), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 1);
			test_int_eq_hard(returnok, 1);
			test_int_eq_hard(pagesok, 1);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access write - cpr will be called, change takes effect (own page is mapped) */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			spurious_byte_memwrite(page_nr(1), 'a');
			die_if(mprotect(page_nr(1), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 1);
			test_int_eq_hard(returnok, 1);
			test_int_eq_hard(pagesok, 1);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access via read syscall - cpr will be called but nothing changes 
	 * syscall fixup code optimizations: just return 0, no PTE will be setup (no zeropage)*/
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			die_if(trigger_syscall_pageread(page_nr(1), 10));
			die_if(mprotect(page_nr(1), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_ge_hard(entry, 0);
			test_int_eq_hard(returnok, 0);
			test_int_eq_hard(pagesok, 0);
		}

		shmdt(mem);
	}

	RESET_ALL();

	/* first access via write syscall - cpr will be called, PTE must be setup */
	{
		char *mem = shmat(shmid, NULL, 0);
		die_if(mem == (void*) -1);

		{
			die_if(trigger_syscall_pagewrite(page_nr(1), 10));
			die_if(mprotect(page_nr(1), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int pagesok = query_int_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);

			test_int_eq_hard(entry, 1);
			test_int_eq_hard(returnok, 1);
			test_int_eq_hard(pagesok, 1);
		}

		shmdt(mem);
	}

	RESET_ALL();

	test_passed();

__finish:
	shmctl(shmid, IPC_RMID, NULL);
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}
