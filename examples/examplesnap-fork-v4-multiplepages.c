#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	int fd = shm_open(POSIX_SHM_NAME, POSIX_SHM_OFLAGS, POSIX_SHM_OFLAGS);
	if (fd < 0) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	ftruncate(fd, 3 * PAGE_SIZE);

	pid_t child;
	char *mem = mmap(
			NULL, 
			3 * PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_SHARED, 
			fd, 0);

	/* here the first snapshot happens */
	*mem = x86_opcode_ret;

	child = fork();

	if(!child) {
		/* here we get the second one */
		check_scid_bcast_snapshot_post(
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

		*(mem + 4096) = x86_opcode_ret;

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	((void(*)(void))mem)();

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

	check_scid_bcast_snapshot_post(
			mem + 4096
			,
			2
			,
			SNAPSHOT_IFETCH_FAULT
			,
			((void(*)(void))(mem + 4096))();
			,
	);

	example_passed();
	munmap(mem, PAGE_SIZE);
	close(fd);
	shm_unlink(POSIX_SHM_NAME);

	return EXIT_SUCCESS;
}
