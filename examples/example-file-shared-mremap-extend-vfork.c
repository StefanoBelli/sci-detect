/* ftm for mremap */
#define _GNU_SOURCE

/* ftm for sync */
#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	int fd;
	char* mem;
	pid_t child_pid;

	flush_page_cache();
	
	__maybe_mlock_all_addr_space();

	fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_SHARED, 
			fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	char *new_mem = __mremap_shrink_or_extend(mem, 1, 2);
	*new_mem = x86_opcode_ret;

	child_pid = vfork();
	if(!child_pid) {
		__maybe_mlock_all_addr_space();

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

	if(mprotect(new_mem + PAGE_SIZE, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC)) {
		perror("mprotect");
		_exit(EXIT_FAILURE);
	}

	check_scid_bcast_wxwarning(
			new_mem + PAGE_SIZE
			,
			*(new_mem + PAGE_SIZE) = x86_opcode_ret;
			,
	);

	if(munmap(mem, PAGE_SIZE))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
