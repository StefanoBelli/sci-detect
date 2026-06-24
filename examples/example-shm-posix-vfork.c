/* ftm for ftruncate */
#define _XOPEN_SOURCE 500

#include <sys/stat.h>
#include <sys/fcntl.h>

#include "exampleutils.h"

#define SHM_NAME "example-shm"
#define SHM_OFLAGS O_RDWR | O_CREAT | O_EXCL
#define SHM_MODE 0700

int main()
{
	char* mem;
	pid_t child_pid;
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

	mem = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		shm_unlink(SHM_NAME);
		return EXIT_FAILURE;
	}

	*mem = x86_opcode_ret;

	child_pid = vfork();
	if(!child_pid) {

		/* here, mprotect may be able to change PTE directly because
		 * the address space is shared! (same mm as parent, no copy)
		 */
		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					shm_unlink(SHM_NAME);
					exit(EXIT_FAILURE);
				}
				,
				,
		);
		
		((void(*)(void))mem)();
		exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("fork");
		shm_unlink(SHM_NAME);
		return EXIT_FAILURE;
	} else {
		shm_unlink(SHM_NAME);
		wait_for_child(child_pid);
	}

	if(munmap(mem, PAGE_SIZE))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
