/* ftruncate ftm */
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

	child_pid = fork();
	if(!child_pid) {

		__maybe_mlock_all_addr_space();

#ifdef EXAMPLE_MLOCK_ALL

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
#else
		/* with shm (with all VM_SHARED vmas?) kernel will do a
		 * "lazy protection bits adjustment" (essentially leaving
		 * them off and turning them on when faulting if VMA prot 
		 * allows to do so)
		 */
		if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
			perror("mprotect");
			shm_unlink(POSIX_SHM_NAME);
			_exit(EXIT_FAILURE);
		}

		/* once execution happens for real it gets detected :) */
		check_scid_bcast_wxwarning(
				mem
				,
				((void(*)(void))mem)();
				,
		);
#endif

		_exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("fork");
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
