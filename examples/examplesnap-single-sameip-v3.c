#define _XOPEN_SOURCE 500
#define SIGLONGJMP(x, y) siglongjmp(x, y)

#include "exampleutils.h"

int main()
{
	printf("%d\n", getpid());

	int fd = shm_open(POSIX_SHM_NAME, POSIX_SHM_OFLAGS, POSIX_SHM_MODE);
	if(fd < 0) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	ftruncate(fd, 3 * PAGE_SIZE);

	char *mem = mmap(NULL, 3 * PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_SHARED, fd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	printf("%p\n", mem);

	/* this serves to prove the last fix worked: wrex enforces
	 * pte prot bits when exec shadow prot are on.
	 *
	 * These are 3 consecutive pages mapped: when we jmp/call we
	 * fall through all of them but when we see the log, only the
	 * first page has a sequence of write/ifetch/write/ifetch/...
	 * 
	 * Why is that? Because being the instruction "0x00 0x00", but
	 * "rax" is set to the first page. 
	 */
	catch_sigsegv(
			/* full of "0x00 0x00" with "rax" == "mem" */

			__asm__ __volatile__(
				"movq %0, %%rax;"
				"callq *%%rax;"
				:: "r"(mem) 
				: "rax", "memory"
			);
	);

	*mem = x86_opcode_ret;

	check_scid_bcast_snapshot(
			mem
			,
			4097
			,
			SNAPSHOT_IFETCH_FAULT
			,
			((void(*)(void))mem)();
			,
	);

	shm_unlink(POSIX_SHM_NAME);

	return 0;
}
