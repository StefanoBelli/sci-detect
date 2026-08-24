/* ftm for ftruncate */
#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	char *mem;
	int shm_fd;

	__maybe_mlock_all_addr_space();

	shm_fd = shm_open(POSIX_SHM_NAME, POSIX_SHM_OFLAGS, POSIX_SHM_MODE);
	if(shm_fd < 0) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	if(ftruncate(shm_fd, PAGE_SIZE)) {
		perror("ftruncate");
		shm_unlink(POSIX_SHM_NAME);
		return EXIT_FAILURE;
	}

	mem = mmap(NULL, 
			PAGE_SIZE, PROT_READ | PROT_WRITE, 
			MAP_SHARED, shm_fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		shm_unlink(POSIX_SHM_NAME);
		return EXIT_FAILURE;
	}

	*mem = x86_opcode_ret;

	return EXIT_SUCCESS;
}
