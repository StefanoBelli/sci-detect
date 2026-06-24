#include <sys/stat.h>
#include <fcntl.h>

#include "exampleutils.h"

#define SHM_NAME "example-shm"
#define SHM_OFLAGS O_RDWR
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

	mem = mmap(NULL, 
			PAGE_SIZE, PROT_READ | PROT_EXEC, 
			MAP_SHARED, shm_fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		shm_unlink(SHM_NAME);
		return EXIT_FAILURE;
	}

	check_scid_bcast_wxwarning(
			mem
			,
			((void(*)(void))mem)();
			,
			,
	);

	shm_unlink(SHM_NAME);

	return EXIT_SUCCESS;
}
