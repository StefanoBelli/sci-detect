#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
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

	child = vfork();

	if(!child) {
		check_scid_bcast_snapshot(
				/* the virtual address */
				mem
				,
				/* seqnum */
				3
				,
				/* the expected fault */
				SNAPSHOT_WRITE_FAULT
				,
				/* the snapshot-triggering operation */
				*mem = x86_opcode_ret;
				,
		);

		check_scid_bcast_snapshot(
				/* the virtual address */
				mem
				,
				/* the expected seq num */
				4
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

	/* don't trigger snapshot */
	((void(*)(void))mem)();

	check_scid_bcast_snapshot(
			/* the virtual address */
			mem
			,
			/* seqnum */
			5
			,
			/* the expected fault */
			SNAPSHOT_WRITE_FAULT
			,
			/* the snapshot-triggering operation */
			*mem = x86_opcode_ret;
			,
	);

	example_passed();
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
