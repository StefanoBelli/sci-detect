#include "exampleutils.h"

int main()
{
	/* CoW stuff */
	pid_t child;
	char *mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_ANONYMOUS | MAP_PRIVATE, 
			-1, 0);

	*mem = x86_opcode_ret;

	check_scid_bcast_wxwarning(
			mem
			,
			mprotect(mem, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
			,
	);

	child = fork();

	if(!child) {
		/* first read: enforce noneprot */
		printf("%d\n", *mem);

		check_scid_bcast_snapshot(
				mem
				,
				2
				,
				SNAPSHOT_IFETCH_FAULT
				,
				((void(*)(void))mem)();
				,
		);

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
