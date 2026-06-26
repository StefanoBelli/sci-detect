/* ftm for mremap */
#define _GNU_SOURCE

#include "exampleutils.h"

int main()
{
	__maybe_mlock_all_addr_space();

	int fd;
	char* mem;
	pid_t child_pid;

	fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE, 
			fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	char *new_mem = __mremap_move(mem, 1);
	*new_mem = x86_opcode_ret;

	/* at this point, the page is a regular private+anonymous one */

	child_pid = fork();
	if(!child_pid) {
		__maybe_mlock_all_addr_space();

		check_scid_bcast_wxwarning(
				new_mem
				,
				if(mprotect(new_mem, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))new_mem)();
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
