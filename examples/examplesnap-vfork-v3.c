#define _XOPEN_SOURCE 500
#define SIGLONGJMP(x,y) siglongjmp((x), (y))

#include "exampleutils.h"

int main()
{
	pid_t child;
	char *mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_ANONYMOUS | MAP_SHARED, 
			-1, 0);

	*mem = x86_opcode_ret;

	child = vfork();

	if(!child) {
		/* first snapshot happens here */
		mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC);

		/* second snapshot */
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

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	/* don't trigger snapshot */
	((void(*)(void))mem)();

	/* we actuated the split permission and page tables were the same,
	 * no W bit enabled, expect segfault
	 */
	catch_sigsegv(
			*mem = x86_opcode_ret;
	);

	mprotect(mem, PAGE_SIZE, PROT_WRITE);

	/* we reenabled the write bit, now expect the write */
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

	/* do similar stuff to check that user-set VMA protection is
	 * properly enforced, doesn't matter the shadow perms state
	 */
	catch_sigsegv(
			((void(*)(void))mem)();
	);

	mprotect(mem, PAGE_SIZE, PROT_EXEC);

	check_scid_bcast_snapshot(
			/* the virtual address */
			mem
			,
			/* seqnum */
			4
			,
			/* the expected fault */
			SNAPSHOT_IFETCH_FAULT
			,
			/* the snapshot-triggering operation */
			((void(*)(void))mem)();
			,
	);

	mprotect(mem, PAGE_SIZE, PROT_READ);

	catch_sigsegv(
			((void(*)(void))mem)();
	);

	catch_sigsegv(
			*mem = x86_opcode_ret;
	);

	example_passed();
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
