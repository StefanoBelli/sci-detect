#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	/* if other examples/tests tested under the same file, page cache frames
	 * are already being tracked by the kernel module and this example cannot
	 * succeed... So we need to flush page cache.
	 */
	flush_page_cache();

	pid_t child;
	int fd = open("res/file", O_RDWR, 0);
	if(fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	char *mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_SHARED, 
			fd, 0);

	*mem = x86_opcode_ret;
	*mem = x86_opcode_ret;

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

	child = fork();
	if(!child) {
		/* 
		 * correctly intercepted the write even after
		 * lazy PTE reconstruction of hw PTE assoc. to non-anon VMA!
		 */
		check_scid_bcast_snapshot(
				mem
				,
				3
				,
				SNAPSHOT_WRITE_FAULT
				,
				*mem = x86_opcode_ret;
				,
		);

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	example_passed();
	close(fd);
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
