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

	/* do nothing - cpr must not be called */
	{
		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, 10 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE | MAP_ANONYMOUS, 
				-1, 0);
		die_if(mem == MAP_FAILED);

		spurious_byte_memwrite(mem, 'a');

		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);

			test_int_eq_hard(entry, 0);
		}

		munmap(mem, 10 * PAGE_SIZE);
	}

	RESET_ALL();

	test_passed();

__finish:
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}
