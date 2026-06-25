#include "exampleutils.h"

int main()
{
	char *mem;
	int shmid;

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
	
	shmdt(mem);
	return EXIT_SUCCESS;
}
