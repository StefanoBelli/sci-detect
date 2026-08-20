#include "exampleutils.h"

int main()
{
	pid_t child;
	char *mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_SHARED | MAP_ANONYMOUS, 
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

	/* here we get the third one */
	/* this never fails: no CoW since mapping is shared */
	check_scid_bcast_snapshot(
			/* the virtual address */
			mem
			,
			/* the expected seq num */
			3
			,
			/* the expected fault */
			SNAPSHOT_WRITE_FAULT
			,
			/* the snapshot-triggering operation */
			*mem = x86_opcode_ret;
			,
	);

	child = fork();

	if(!child) {
		/* The instruction fetch TRIGGERS the snapshot! CoW not broken */
		check_scid_bcast_snapshot_post(
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

	/* this should not trigger the snapshot */
	((void(*)(void))mem)();

	/* expect snapshot at write here */
	check_scid_bcast_snapshot(
			mem
			,
			5
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
