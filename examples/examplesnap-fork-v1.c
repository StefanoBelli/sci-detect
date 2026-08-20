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
		/* CoW breaks, but newly created PTE has WX */
		check_scid_bcast_wxwarning(
				/* the virtual address */
				mem
				,
				/* the snapshot-triggering operation */
				*mem = x86_opcode_ret;
				,
		);

		/* here we get the second one */
		check_scid_bcast_snapshot_post(
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

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);
	example_passed();
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
