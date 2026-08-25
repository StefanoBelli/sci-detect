#define _XOPEN_SOURCE 500
#define SIGLONGJMP(x, y) siglongjmp(x, y)

#include "exampleutils.h"

int main()
{
	char *mem = mmap(NULL, PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

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
			4096
			,
			SNAPSHOT_IFETCH_FAULT
			,
			((void(*)(void))mem)();
			,
	);

	return 0;
}
