/* ftm for mremap */
#define _GNU_SOURCE

#include "exampleutils.h"

int main()
{
	char* mem;
	pid_t child_pid;

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

	char *new_mem = __mremap_move_dontunmap(mem, 1);
	*new_mem = x86_opcode_ret;

	child_pid = vfork();
	if(!child_pid) {
		check_scid_bcast_wxwarning(
				new_mem
				,
				if(mprotect(new_mem, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					_exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))new_mem)();
		_exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("vfork");
		return EXIT_FAILURE;
	} else
		wait_for_child(child_pid);

	if(munmap(mem, PAGE_SIZE))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
