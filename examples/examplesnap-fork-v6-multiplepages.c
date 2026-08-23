#define _XOPEN_SOURCE 500
#define SIGLONGJMP(x, y) siglongjmp(x, y)

#include "exampleutils.h"

int main()
{
	pid_t child;
	char *mem = mmap(
			NULL, 
			5 * PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_SHARED | MAP_ANONYMOUS, 
			-1, 0);

	*mem = x86_opcode_ret;
	*(mem + 8192) = x86_opcode_ret;

	child = fork();

	if(!child) {
		mprotect(mem, PAGE_SIZE, PROT_EXEC | PROT_WRITE | PROT_READ);

		/* here we get the second one: if we accessed memory earlier from
		 * this child process, it would have been second access (snapshot only)
		 */
		check_scid_bcast_wxwarning(
				mem
				,
				((void(*)(void))mem)();
				,
		);

		printf("child: %d %c\n", getpid(), *mem);

		*(mem + 4096) = x86_opcode_ret;

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	catch_sigsegv(
			((void(*)(void))mem)();
	);

	check_scid_bcast_snapshot(
			mem
			,
			2
			,
			SNAPSHOT_WRITE_FAULT
			,
			*mem = x86_opcode_ret;
			,
	);

	example_passed();
	munmap(mem, PAGE_SIZE);
	shm_unlink(POSIX_SHM_NAME);

	return EXIT_SUCCESS;
}
