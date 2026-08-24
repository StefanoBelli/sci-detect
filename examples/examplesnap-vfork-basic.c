#define _GNU_SOURCE

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

	child = vfork();

	if(!child) {
		printf("%d %p\n", getpid(), mem);

		/* split perm, with the vfork the wxwarning happens here due to 
		 * the page table being already setup*/

		check_scid_bcast_wxwarning(
				mem
				,
				mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC /* | PROT_WRITE */ );
				,
		);

		/* at this point we have noprot, we can either do the write or exec
		 * (if enabled in vma)
		 */
#if 1
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
#else /* remember to enable PROT_WRITE */
		check_scid_bcast_snapshot(
				mem
				,
				2
				,
				SNAPSHOT_WRITE_FAULT
				,
				*mem = x86_opcode_ret;
				,
		);
#endif

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	example_passed();
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
