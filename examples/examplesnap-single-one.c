#include "exampleutils.h"

#define PRINT_EXAMPLE_NR() \
	printf("\n\texample nr %d\n\n", ++exnr)

int main()
{
	int exnr = 0;

	/* example 1 */
	{
		PRINT_EXAMPLE_NR();

		char *mem = mmap(
				NULL, 
				PAGE_SIZE, 
				PROT_READ | PROT_WRITE | PROT_EXEC, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0);

		/* here the first snapshot happens */
		*mem = 0xc3;

		/* here we get the second one */
		check_scid_bcast_snapshot(
				/* the virtual address */
				mem
				,
				/* the expected seq num */
				2
				,
				/* the snapshot-triggering operation */
				((void(*)(void))mem)();
				,
		);

		munmap(mem, PAGE_SIZE);
	}

	return 0;
}
