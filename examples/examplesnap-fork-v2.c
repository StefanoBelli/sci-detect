#include "exampleutils.h"

int main()
{
	/* CoW stuff */

	pid_t child;
	char *mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_ANONYMOUS | MAP_PRIVATE, 
			-1, 0);

	/* here the first snapshot happens */
	*mem = x86_opcode_ret;

	/* here we get the second one */
	check_scid_bcast_snapshot(
			/* the virtual address */
			mem
			,
			/* the expected seq num */
			2
			,
			/* the expected fault */
			SNAPSHOT_IFETCH_FAULT
			,
			/* the snapshot-triggering operation */
			((void(*)(void))mem)();
			,
	);

	child = fork();

	if(!child) {
		/* CoW still ok, but the instruction fetch mustn't fail */
		((void(*)(void))mem)();

		/* CoW breaks here, newly created PTE has WX */
		check_scid_bcast_wxwarning(
				mem
				,
				*mem = x86_opcode_ret;
				,
		);

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	printf("%d\n", getpid());

	/* may fail if kernel decides to use an entirely new frame (also for parent) */
	check_scid_bcast_snapshot(
			mem
			,
			3
			,
			SNAPSHOT_WRITE_FAULT
			,
			*mem = x86_opcode_ret;
			,
	);

	example_passed();
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
