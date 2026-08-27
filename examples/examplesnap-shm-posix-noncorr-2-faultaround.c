/* ftm for ftruncate */
#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	char *mem;
	int shm_fd;

	shm_fd = shm_open(POSIX_SHM_NAME, O_RDWR, POSIX_SHM_MODE);
	if(shm_fd < 0) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	mem = mmap(NULL, 
			3 * PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_SHARED, shm_fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		shm_unlink(POSIX_SHM_NAME);
		return EXIT_FAILURE;
	}

	/* the following read will most likely trigger
	 * fault around, check if that's on
	 *
	 * # cat /sys/kernel/debug/fault_around_bytes
	 */
	printf("%d\n", *mem);

	check_scid_bcast_snapshot_post(
			mem + 2 * PAGE_SIZE
			,
			2
			,
			SNAPSHOT_IFETCH_FAULT
			,
			((void(*)(void))(mem + 2 * PAGE_SIZE))();
			,
	);

	check_scid_bcast_snapshot_post(
			mem + PAGE_SIZE
			,
			2
			,
			SNAPSHOT_IFETCH_FAULT
			,
			((void(*)(void))(mem + PAGE_SIZE))();
			,
	);

	check_scid_bcast_snapshot_post(
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
	return EXIT_SUCCESS;
}
