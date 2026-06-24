/* ftm for ftruncate */
#define _XOPEN_SOURCE 500

#include <sys/stat.h>
#include <fcntl.h>

#include "exampleutils.h"

#define SHM_NAME "example-shm"
#define SHM_OFLAGS O_RDWR | O_CREAT | O_EXCL
#define SHM_MODE 0700

int main()
{
	char *mem;
	int shm_fd;

	shm_fd = shm_open(SHM_NAME, SHM_OFLAGS, SHM_MODE);
	if(shm_fd < 0) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	if(ftruncate(shm_fd, PAGE_SIZE)) {
		perror("ftruncate");
		shm_unlink(SHM_NAME);
		return EXIT_FAILURE;
	}

	mem = mmap(NULL, 
			PAGE_SIZE, PROT_READ | PROT_WRITE, 
			MAP_SHARED, shm_fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		shm_unlink(SHM_NAME);
		return EXIT_FAILURE;
	}

	*mem = x86_opcode_ret;

	return EXIT_SUCCESS;
}
