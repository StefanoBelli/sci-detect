/* ftm for mremap */
#define _GNU_SOURCE

/* ftm for vfork */
#define _XOPEN_SOURCE 500

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

	char *new_mem = __mremap_shrink_or_extend(mem, 1, 3);
	*new_mem = x86_opcode_ret;

	/* at this point, the page is a regular private+anonymous one */

	child_pid = vfork();
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

		if(mprotect(new_mem + PAGE_SIZE, 2 * PAGE_SIZE, PROT_READ | PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		*(new_mem + (2 * PAGE_SIZE)) = x86_opcode_ret;

		exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("fork");
		return EXIT_FAILURE;
	} else
		wait_for_child(child_pid);

	check_scid_bcast_wxwarning(
			new_mem + (2 * PAGE_SIZE)
			,
			if(mprotect(new_mem + (2 * PAGE_SIZE), PAGE_SIZE, PROT_EXEC)) {
				perror("mprotect");
				exit(EXIT_FAILURE);
			}
			,
	);

	((void(*)(void))(new_mem + (2 * PAGE_SIZE)))();

	if(munmap(mem, PAGE_SIZE))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
