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

	check_scid_bcast_wxwarning(
			mem
			,
			((void(*)(void))mem)();
			,
			,
	);

	shm_unlink(POSIX_SHM_NAME);

	return EXIT_SUCCESS;
}
