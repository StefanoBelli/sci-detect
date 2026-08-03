#define _POSIX_C_SOURCE 200809L
#define SIGLONGJMP(x, y) siglongjmp(x, y)

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
				SCID_PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0); 

		*mem = x86_opcode_ret;

		/* wx page detection + page snap, prot bits are zeroed (both wr and ex) */
		if(mprotect(mem, SCID_PAGE_SIZE, PROT_READ | PROT_EXEC)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		/* here we get the second one */
		check_scid_bcast_snapshot(
				/* the virtual address */
				mem
				,
				/* the expected seq num */
				2
				,
				/* the expected fault */
				SNAPSHOT_IFETCH_FAULT
				,
				/* the snapshot-triggering operation */
				((void(*)(void))mem)();
				,
				);

		/* the page is tracked as wx, but last mprotect removed PROT_WRITE */
		catch_sigsegv(
				/* this will not go through, expect SIGSEGV */
				*mem = x86_opcode_ret; 
				);

		example_passed();
		munmap(mem, SCID_PAGE_SIZE);
	}

	/* example 2 */
	{
		PRINT_EXAMPLE_NR();

		char *mem = mmap(
				NULL, 
				SCID_PAGE_SIZE, 
				PROT_READ | PROT_WRITE | PROT_EXEC, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0); 

		/* wx page detection */
		*mem = x86_opcode_ret;

		if(mprotect(mem, SCID_PAGE_SIZE, PROT_READ | PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		catch_sigsegv(
				((void(*)(void))mem)();
		);

		example_passed();
		munmap(mem, SCID_PAGE_SIZE);
	}

	/* example 3 */
	{
		PRINT_EXAMPLE_NR();

		char *mem = mmap(
				NULL, 
				SCID_PAGE_SIZE, 
				PROT_READ | PROT_WRITE | PROT_EXEC, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0); 

		/* wx page detection */
		*mem = x86_opcode_ret;

		if(mprotect(mem, SCID_PAGE_SIZE, PROT_READ | PROT_EXEC)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		catch_sigsegv(
				*mem = x86_opcode_ret;
		);

		check_scid_bcast_snapshot(
				/* the virtual address */
				mem
				,
				/* the expected seq num */
				2
				,
				/* the expected fault */
				SNAPSHOT_IFETCH_FAULT
				,
				/* the snapshot-triggering operation */
				((void(*)(void))mem)();
				,
		);

		catch_sigsegv(
				*mem = x86_opcode_ret;
		);

		if(mprotect(mem, SCID_PAGE_SIZE, PROT_READ | PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		check_scid_bcast_snapshot(
				/* the virtual address */
				mem
				,
				/* the expected seq num */
				3
				,
				/* the expected fault */
				SNAPSHOT_WRITE_FAULT
				,
				/* the snapshot-triggering operation */
				*mem = x86_opcode_ret;
				,
		);

		example_passed();
		munmap(mem, SCID_PAGE_SIZE);
	}

	return EXIT_SUCCESS;
}
