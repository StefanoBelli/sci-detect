/* ftm for sync */
#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	char *mem;
	int fd;

	fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	mem = mmap(NULL, 
			PAGE_SIZE, 
			PROT_EXEC, 
			MAP_SHARED, fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return EXIT_FAILURE;
	}

	check_scid_bcast_wxwarning(
			mem
			,
			((void(*)(void))mem)();
			,
	);

	close(fd);

	example_passed();
	return EXIT_SUCCESS;
}
