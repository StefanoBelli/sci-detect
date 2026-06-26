/* ftm for ftruncate */
#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	char* mem;
	pid_t child_pid;
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

	mem = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		shm_unlink(POSIX_SHM_NAME);
		return EXIT_FAILURE;
	}

	*mem = x86_opcode_ret;

	child_pid = vfork();
	if(!child_pid) {

		__maybe_mlock_all_addr_space();

		/* here, mprotect may be able to change PTE directly because
		 * the address space is shared! (same mm as parent, no copy)
		 */
		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					shm_unlink(POSIX_SHM_NAME);
					_exit(EXIT_FAILURE);
				}
				,
		);
		
		((void(*)(void))mem)();
		_exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("vfork");
		shm_unlink(POSIX_SHM_NAME);
		return EXIT_FAILURE;
	} else {
		shm_unlink(POSIX_SHM_NAME);
		wait_for_child(child_pid);
	}

	if(munmap(mem, PAGE_SIZE))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
