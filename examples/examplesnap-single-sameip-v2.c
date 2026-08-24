#define _XOPEN_SOURCE 500
#define SIGLONGJMP(x, y) siglongjmp(x, y)

#include "exampleutils.h"

int main()
{
	char *mem = mmap(NULL, PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	/* wx detection */
	*mem = 0;
	*(mem + 1) = 0;
	*(mem + 2) = x86_opcode_ret;

	check_scid_bcast_snapshot(
			mem
			,
			2
			,
			SNAPSHOT_IFETCH_FAULT
			,
			((void(*)(void))mem)();
			,
	);

	/* seq:3 was write by 0x00 0x00
	 * seq:4 was exec by 0xc3 
	 */
	check_scid_bcast_snapshot(
			mem
			,
			5
			,
			SNAPSHOT_WRITE_FAULT
			,
			*mem = x86_opcode_ret;
			,
	);

	return 0;
}
