#define _XOPEN_SOURCE 500

#include "exampleutils.h"

int main()
{
	/* example nr 1 */
	{
		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE | PROT_EXEC, 
				MAP_ANONYMOUS | MAP_PRIVATE,
				-1, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* no wx detection happens here, PTE -> zeropage in RO+X (zeropage ignored) */
		printf("%d\n", *mem);

		/* ensure that the earlier read didn't affect ptealtprot */
		check_scid_bcast_wxwarning_post(
				mem
				,
				*mem = x86_opcode_ret;
				,
		);

		/* exec won't work since it is the zeropage... */

		munmap(mem, 3 * PAGE_SIZE);
	}

	/* example nr 2 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* vma still RW */
		printf("%d\n", *mem);

		/* write code */
		*mem = x86_opcode_ret;

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 2+3 */
	{
		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_EXEC, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* noprot should be applied by first read (init) */
		check_scid_bcast_wxwarning_post(
				mem
				,
				printf("%d\n", *mem);
				,
		);

		/* run code */
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

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 4 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* vma still RW */
		printf("%d\n", *mem);

		/* write code */
		*mem = 0x90;

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 4+5 */
	{
		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE | PROT_EXEC, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* noprot should be applied by first read (init) */
		check_scid_bcast_wxwarning_post(
				mem
				,
				printf("%d\n", *mem);
				,
		);

		/* we check for write too (since inited with noprot) */
		check_scid_bcast_snapshot_post(
				mem
				,
				2
				,
				SNAPSHOT_WRITE_FAULT
				,
				*mem = x86_opcode_ret;
				,
		);

		/* run code */
		check_scid_bcast_snapshot_post(
				mem
				,
				3
				,
				SNAPSHOT_IFETCH_FAULT
				,
				((void(*)(void))mem)();
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 5 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* vma still RW */
		printf("%d\n", *mem);

		/* write code */
		*mem = 0x90;

		/* noneprot */
		check_scid_bcast_wxwarning_post(
				mem
				,
				mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC);
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 5+6 */
	{
		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* here we don't init but enforce the already existing noprot */
		printf("%d\n", *mem);

		/* we check for write */
		check_scid_bcast_snapshot_post(
				mem
				,
				2
				,
				SNAPSHOT_WRITE_FAULT
				,
				*mem = x86_opcode_ret;
				,
		);

		mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC);

		/* run code */
		check_scid_bcast_snapshot_post(
				mem
				,
				3
				,
				SNAPSHOT_IFETCH_FAULT
				,
				((void(*)(void))mem)();
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 6 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* vma still RW */
		printf("%d\n", *mem);

		/* write code */
		*mem = x86_opcode_ret;

		/* noneprot */
		check_scid_bcast_wxwarning_post(
				mem
				,
				mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC);
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 6+7 */
	{
		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_EXEC, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* here we don't init but enforce the already existing noprot */
		printf("%d\n", *mem);

		/* since it is noneprot, we also check for exec */
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

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 7 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* vma still RW */
		printf("%d\n", *mem);

		/* write code */
		*mem = x86_opcode_ret;

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 7+8 */
	{
		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		/* CoW file-backed memory */
		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_EXEC, 
				MAP_PRIVATE,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* noprot should be applied by first read (init) */
		check_scid_bcast_wxwarning_post(
				mem
				,
				printf("%d\n", *mem);
				,
		);

		/* run code, this is not the zeropage (pgcache file-backed memory) */
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

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

 	/* example nr 8 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* vma still RW */
		printf("%d\n", *mem);

		/* write code */
		*mem = x86_opcode_ret;

		/* noneprot */
		check_scid_bcast_wxwarning_post(
				mem
				,
				mprotect(mem, PAGE_SIZE, PROT_READ | PROT_EXEC);
				,
		);

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* example nr 8+9 */
	{
		int fd = open("res/file", O_RDWR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		/* CoW file-backed memory */
		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_READ | PROT_EXEC, 
				MAP_PRIVATE,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		/* here we don't init but enforce the already existing noprot */
		printf("%d\n", *mem);

		/* since it is noneprot, we also check for exec */
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

		munmap(mem, 3 * PAGE_SIZE);
		close(fd);
	}

	/* Further similar examples on MAP_PRIVATE + file backed memory would be useless since
	 * when a file-backed memory page is written, CoW happens and page becomes a private anon page
	 * for the current process
	 */

	example_passed();
	return EXIT_SUCCESS;
}

