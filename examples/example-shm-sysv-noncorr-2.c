#include <sys/shm.h>

#include "exampleutils.h"

#define SHM_KEY 0xdeadbeef
#define SHM_SIZE PAGE_SIZE
#define SHM_FLG 0

int main()
{
	char *mem;
	int shmid;

	shmid = shmget(SHM_KEY, SHM_SIZE, SHM_FLG);
	if(shmid < 0) {
		perror("shmget");
		return EXIT_FAILURE;
	}

	mem = shmat(shmid, NULL, SHM_EXEC);
	if(mem == (void*) -1) {
		perror("shmat");
		shmdt(mem);
		shmctl(shmid, IPC_RMID, NULL);
		return EXIT_FAILURE;
	}

	check_scid_bcast_wxwarning(
			mem
			,
			((void(*)(void))mem)();
			,
			,
	);

	shmdt(mem);
	shmctl(shmid, IPC_RMID, NULL);

	return EXIT_SUCCESS;
}
