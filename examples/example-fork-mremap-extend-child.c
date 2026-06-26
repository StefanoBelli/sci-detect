/* ftm for mremap */
#define _GNU_SOURCE

#include "exampleutils.h"

int main()
{
	char* mem;
	pid_t child_pid;

	__maybe_mlock_all_addr_space();

	mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	*mem = x86_opcode_ret;

	child_pid = fork();
	if(!child_pid) {
		__maybe_mlock_all_addr_space();

		char *_addr = __mremap_shrink_or_extend(mem, 1, 3);

		check_scid_bcast_wxwarning(
				_addr
				,
				if(mprotect(_addr, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))_addr)();

		/* trying on the newly-extended area, a fresh new frame... */

		if(mprotect(_addr + PAGE_SIZE, PAGE_SIZE, PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		*(_addr + PAGE_SIZE) = x86_opcode_ret;

		check_scid_bcast_wxwarning(
				_addr + PAGE_SIZE,
				if(mprotect(_addr + PAGE_SIZE, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))_addr)();

		exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("fork");
		return EXIT_FAILURE;
	} else
		wait_for_child(child_pid);

	if(munmap(mem, PAGE_SIZE))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
