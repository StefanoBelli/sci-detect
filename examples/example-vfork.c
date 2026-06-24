/* feature test macro required for vfork */
#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	char* mem;
	pid_t child_pid;

	mem = mmap(
			NULL, 
			4096, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	*mem = x86_opcode_ret;

	child_pid = vfork();
	if(!child_pid) {
		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, 4096, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					_exit(EXIT_FAILURE);
				}
				,
				,
				);

		((void(*)(void))mem)();
		_exit(EXIT_SUCCESS);
	} else if (child_pid < 0) {
		perror("vfork");
		return EXIT_FAILURE;
	} else
		/* this is not necessary, but to check exit status */
		wait_for_child(child_pid);

	if(munmap(mem, 4096))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
