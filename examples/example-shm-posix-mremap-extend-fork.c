/* mremap ftm */
#define _GNU_SOURCE

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

	if(ftruncate(shm_fd, 3 * PAGE_SIZE)) {
		perror("ftruncate");
		shm_unlink(POSIX_SHM_NAME);
		return EXIT_FAILURE;
	}

	char *new_mem = __mremap_shrink_or_extend(mem, 1, 3);

	child_pid = fork();
	if(!child_pid) {

		__maybe_mlock_all_addr_space();

#ifdef EXAMPLE_MLOCK_ALL
		check_scid_bcast_wxwarning(
				new_mem
				,
				if(mprotect(new_mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					shm_unlink(POSIX_SHM_NAME);
					_exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))new_mem)();

		check_scid_bcast_wxwarning(
				new_mem + (2 * PAGE_SIZE)
				,
				if(mprotect(new_mem + (2 * PAGE_SIZE), PAGE_SIZE, PROT_WRITE | PROT_EXEC)) {
					perror("mprotect");
					shm_unlink(POSIX_SHM_NAME);
					_exit(EXIT_FAILURE);
				}
				,
		);

		*(new_mem + (2 * PAGE_SIZE)) = x86_opcode_ret;
#else
		/* with shm (with all VM_SHARED vmas?) kernel will do a
		 * "lazy protection bits adjustment" (essentially leaving
		 * them off and turning them on when faulting if VMA prot 
		 * allows to do so)
		 */
		if(mprotect(new_mem, PAGE_SIZE, PROT_EXEC)) {
			perror("mprotect");
			shm_unlink(POSIX_SHM_NAME);
			_exit(EXIT_FAILURE);
		}

		/* once execution happens for real it gets detected :) */
		check_scid_bcast_wxwarning(
				new_mem
				,
				((void(*)(void))new_mem)();
				,
		);

		if(mprotect(new_mem + (2 * PAGE_SIZE), PAGE_SIZE, PROT_WRITE | PROT_EXEC)) {
			perror("mprotect");
			shm_unlink(POSIX_SHM_NAME);
			_exit(EXIT_FAILURE);
		}

		check_scid_bcast_wxwarning(
				new_mem + (2 * PAGE_SIZE)
				,
				*(new_mem + (2 * PAGE_SIZE)) = x86_opcode_ret;
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
