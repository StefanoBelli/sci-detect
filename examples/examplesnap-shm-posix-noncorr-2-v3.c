#include "exampleutils.h"

int main()
{
	char *mem;
	int shm_fd;

	shm_fd = shm_open(
			POSIX_SHM_NAME, 
			POSIX_SHM_OFLAGS & POSIX_NO_EXCL_CREAT, 
			POSIX_SHM_MODE);

	if(shm_fd < 0) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	mem = mmap(NULL, 
			PAGE_SIZE, PROT_READ | PROT_EXEC, 
			MAP_SHARED, shm_fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		shm_unlink(POSIX_SHM_NAME);
		return EXIT_FAILURE;
	}

	/* this should enforce exec protection.
	 * If we do the first access read, and PTE is setup
	 * we must enforce protection
	 */

	check_scid_bcast_wxwarning(
			mem
			,
			printf("%d %d\n", getpid(), *mem);
			,
	);

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

	shm_unlink(POSIX_SHM_NAME);

	example_passed();
	return EXIT_SUCCESS;
}
