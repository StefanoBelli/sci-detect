#include "exampleutils.h"

int main()
{
	pid_t child;
	char *mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_SHARED | MAP_ANONYMOUS, 
			-1, 0);

	*mem = x86_opcode_ret;

	child = fork();

	if(!child) {
		printf("%d %p\n", getpid(), mem);

		/* split perm*/
		mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC);

		/* the post variant doesn't trigger the wxwarning due to
		 * initial read access instead of actual ifetch
		 */
		check_scid_bcast_wxwarning_post(
				mem
				,
				((void(*)(void))mem)();
				,
		);

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	example_passed();
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
