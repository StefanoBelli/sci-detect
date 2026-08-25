#define _GNU_SOURCE

#include "exampleutils.h"

int main()
{
	/* example nr 1 */
	{
		char *mem;

		check_scid_bcast_wxwarning_post(
				mem
				,
				mem = mmap(NULL, PAGE_SIZE,
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
					-1, 0);
				,
		);

		munmap(mem, PAGE_SIZE);
	}

	/* example nr 2 */
	{
		char *mem;

		check_scid_bcast_wxwarning_post(
				mem
				,
				mem = mmap(NULL, PAGE_SIZE,
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE,
					-1, 0);
				,
		);

		munmap(mem, PAGE_SIZE);
	}

	/* example nr 3 */
	{
		char *mem;
		int fd;
		
		flush_page_cache();

		fd = open("res/file", O_RDWR);

		mem = mmap(NULL, 3 * PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_SHARED | MAP_POPULATE,
				fd, 0);

		/* the vma driver wants writenotify, on prefault you
		 * are write-protected and if this is the first wx detection
		 * you won't get notified until you really do a store/ifetch operation
		 */
		check_scid_bcast_wxwarning(
				mem
				,
				*mem = x86_opcode_ret;
				,
		);

		check_scid_bcast_wxwarning(
				mem + PAGE_SIZE
				,
				*(mem + PAGE_SIZE) = x86_opcode_ret;
				,
		);

		((void(*)(void))(mem + PAGE_SIZE))();

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 3+4 */
	{
		char *mem;
		int fd;

		/* we don't do flush page cache */

		fd = open("res/file", O_RDWR);

		mem = mmap(NULL, 3 * PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_SHARED | MAP_POPULATE,
				fd, 0);

		*mem = x86_opcode_ret;

		check_scid_bcast_snapshot_post(
				mem
				,
				2
				,
				SNAPSHOT_IFETCH_FAULT
				,
				((void(*)(void))mem)();
				,
		);

		check_scid_bcast_snapshot_post(
				mem + PAGE_SIZE
				,
				3
				,
				SNAPSHOT_WRITE_FAULT
				,
				*(mem + PAGE_SIZE) = x86_opcode_ret;
				,
		);

		check_scid_bcast_wxwarning(
				mem + 2 * PAGE_SIZE
				,
				*(mem + 2 * PAGE_SIZE) = x86_opcode_ret;
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

#if 0
	/* example nr 3 */
	{
		char *mem;
		int fd;
		
		flush_page_cache();

		fd = open("res/file", O_RDWR);

		mem = mmap(NULL, 3 * PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_SHARED | MAP_POPULATE,
				fd, 0);

		/* in this case, we need better support. due to the writenotify 
		 * mechanism, the page is setup as RO+X. If we do the first access as
		 * write, we're lucky, otherwise X is already enabled and pass through
		 * without page faulting
		 */
		check_scid_bcast_wxwarning_post(
				mem
				,
				((void(*)(void))mem)();
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}
#endif

	/* example nr 4 */
	{
		char *mem;
		int fd;
		
		flush_page_cache();

		fd = open("res/file", O_RDWR);

		check_scid_bcast_wxwarning_post(
				mem
				,
				mem = mmap(NULL, 3 * PAGE_SIZE,
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_PRIVATE | MAP_POPULATE,
					fd, 0);
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	example_passed();
	return EXIT_SUCCESS;
}
