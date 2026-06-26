/* ftm for mremap */
#define _GNU_SOURCE

#include "exampleutils.h"

int main()
{
	char* mem;
	pid_t child_pid;
	int shmid;

	__maybe_mlock_all_addr_space();

	shmid = shmget(SYSV_SHM_KEY, SYSV_SHM_SIZE, SYSV_SHM_FLG);
	if(shmid < 0) {
		perror("shmget");
		return EXIT_FAILURE;
	}

	mem = shmat(shmid, NULL, 0);
	if(mem == (void*) -1) {
		perror("shmat");
		shmctl(shmid, IPC_RMID, NULL);
		return EXIT_FAILURE;
	}

	*mem = x86_opcode_ret;
	char *new_mem = __mremap_move(mem, 1);

	child_pid = fork();
	if(!child_pid) {
		__maybe_mlock_all_addr_space();

#ifdef EXAMPLE_MLOCK_ALL
		check_scid_bcast_wxwarning(
				new_mem
				,
				if(mprotect(new_mem, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					shmdt(new_mem);
					shmctl(shmid, IPC_RMID, NULL);
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))new_mem)();
#else
		/* again, this will just change the VMA, but not the PTE
		 * because there is the shmem_anon vmops (shmem_anon_vm_ops)
		 */
		if(mprotect(new_mem, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
			perror("mprotect");
			shmdt(new_mem);
			shmctl(shmid, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}

		/* lazy PTE change */
		check_scid_bcast_wxwarning(
				new_mem
				,
				((void(*)(void))new_mem)();
				,
		);
#endif

		exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("fork");
		shmdt(new_mem);
		shmctl(shmid, IPC_RMID, NULL);
		return EXIT_FAILURE;
	} else {
		shmdt(new_mem);
		shmctl(shmid, IPC_RMID, NULL);
		wait_for_child(child_pid);
	}

	example_passed();
	return EXIT_SUCCESS;
}
