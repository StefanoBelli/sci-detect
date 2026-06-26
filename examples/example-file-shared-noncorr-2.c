/* ftm for sync */
#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	char *mem;
	int fd;

	__maybe_mlock_all_addr_space();

	fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

#ifdef EXAMPLE_MLOCK_ALL
	check_scid_bcast_wxwarning(
			mem
			,
			mem = mmap(NULL, 
				PAGE_SIZE, 
				PROT_EXEC, 
				MAP_SHARED, fd, 0);
			,
	);
	if(mem == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return EXIT_FAILURE;
	}

	((void(*)(void))mem)();
#else
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
#endif

	close(fd);

	example_passed();
	return EXIT_SUCCESS;
}
