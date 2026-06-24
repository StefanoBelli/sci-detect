/* ftm for vfork */
#define _XOPEN_SOURCE 500

#include <sys/shm.h>
#include "exampleutils.h"

#define SHM_KEY 0xdeadbeef
#define SHM_SIZE PAGE_SIZE
#define SHM_FLG IPC_CREAT | IPC_EXCL

int main()
{
	char* mem;
	pid_t child_pid;
	int shmid;

	shmid = shmget(SHM_KEY, SHM_SIZE, SHM_FLG);
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

	child_pid = vfork();
	if(!child_pid) {
		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					shmdt(mem);
					shmctl(shmid, IPC_RMID, NULL);
					exit(EXIT_FAILURE);
				}
				,
				,
		);

		((void(*)(void))mem)();

		exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("fork");
		shmdt(mem);
		shmctl(shmid, IPC_RMID, NULL);
		return EXIT_FAILURE;
	} else {
		shmdt(mem);
		shmctl(shmid, IPC_RMID, NULL);
		wait_for_child(child_pid);
	}

	example_passed();
	return EXIT_SUCCESS;
}
