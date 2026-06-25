#include "exampleutils.h"

int main()
{
	char *mem;
	int shmid;

	shmid = shmget(SYSV_SHM_KEY, SYSV_SHM_SIZE, SYSV_SHM_FLG & SYSV_NO_EXCL_CREAT);
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
