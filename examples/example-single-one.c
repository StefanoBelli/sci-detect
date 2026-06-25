#include "exampleutils.h"

int main()
{
	/* example 1 */
	{
		char *mem = mmap(
				NULL, 
				PAGE_SIZE, 
				PROT_READ | PROT_WRITE | PROT_EXEC, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0);

		check_scid_bcast_wxwarning(
				mem
				,
				spurious_byte_memwrite(page_nr(1), 'a');
				,
				);

		munmap(mem, PAGE_SIZE);
	}

	/* example 2 */
	{
		char *mem = mmap(
				NULL, 
				PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0);

		*mem = x86_opcode_ret;

		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
				);

		((void(*)(void)) mem)();

		munmap(mem, PAGE_SIZE);
	}

	/* example 3 */
	{
		char *mem = mmap(
				NULL, 
				PAGE_SIZE, 
				PROT_READ, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0);

		spurious_byte_memread(ch, page_nr(1));

		if(mprotect(page_nr(1), PAGE_SIZE, PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		*mem = x86_opcode_ret;

		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
				);

		((void(*)(void)) mem)();

		munmap(mem, PAGE_SIZE);
	}

	/* example 4 */
	{
		char *mem = mmap(
				NULL, 
				PAGE_SIZE, 
				PROT_EXEC, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0);

		if(mprotect(page_nr(1), PAGE_SIZE, PROT_READ)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		spurious_byte_memread(ch, page_nr(1));

		/* nothing will happen here, zeropage still mapped */
		if(mprotect(mem, PAGE_SIZE, PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		/* from now on, the "real" owned page is mapped (w-) */
		*mem = x86_opcode_ret;

		/* wxwarning triggered because mprotect changes 
		 * in this control path, directly, the mapping perm (-x)
		 * summing up, the same page was (w-) + (-x) = (wx) */
		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
				);

		((void(*)(void)) mem)();

		munmap(mem, PAGE_SIZE);
	}

	example_passed();
	return EXIT_SUCCESS;
}
