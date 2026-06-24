#include <sys/shm.h>

#include "exampleutils.h"

#define SHM_KEY 0xdeadbeef
#define SHM_SIZE PAGE_SIZE
#define SHM_FLG IPC_CREAT | IPC_EXCL

int main()
{
	char *mem;
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
	
	shmdt(mem);
	return EXIT_SUCCESS;
}
