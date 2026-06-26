/* ftm for madvise */
#define _DEFAULT_SOURCE

/* ftm for sync */
#define _XOPEN_SOURCE 500

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

	/* example 5 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL,
				PAGE_SIZE,
				PROT_EXEC,
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		if(mprotect(mem, PAGE_SIZE, PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		/* page was not mapped prior this write and mprotect
		 * changed vma flags */
		*mem = x86_opcode_ret;

		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))mem)();

		close(fd);
		munmap(mem, PAGE_SIZE);
	}

	/* example 6 */
	{
		flush_page_cache();

		int fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL,
				PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		check_scid_bcast_wxwarning(
				mem
				,
				*mem = x86_opcode_ret;
				,
		);

		((void(*)(void))mem)();

		close(fd);
		munmap(mem, PAGE_SIZE);
	}

	/* example 7 */
	{
		int fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL,
				PAGE_SIZE,
				PROT_EXEC,
				MAP_PRIVATE,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		if(mprotect(mem, PAGE_SIZE, PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		/* page was not mapped prior this write and mprotect
		 * changed vma flags */
		*mem = x86_opcode_ret;

		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))mem)();

		close(fd);
		munmap(mem, PAGE_SIZE);
	}

	/* example 8 */
	{
		int fd = open("res/file", O_RDWR, S_IRUSR | S_IWUSR);
		if(fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL,
				PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_PRIVATE,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		check_scid_bcast_wxwarning(
				mem
				,
				*mem = x86_opcode_ret;
				,
		);

		((void(*)(void))mem)();

		close(fd);
		munmap(mem, PAGE_SIZE);
	}
	
	/* example 9 */
	{
		int fd = shm_open(POSIX_SHM_NAME, POSIX_SHM_OFLAGS, POSIX_SHM_MODE);
		if(fd < 0) {
			perror("shm_open");
			exit(EXIT_FAILURE);
		}

		if(ftruncate(fd, PAGE_SIZE)) {
			perror("ftruncate");
			shm_unlink(POSIX_SHM_NAME);
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL,
				PAGE_SIZE,
				PROT_EXEC,
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			shm_unlink(POSIX_SHM_NAME);
			exit(EXIT_FAILURE);
		}

		if(mprotect(mem, PAGE_SIZE, PROT_WRITE)) {
			perror("mprotect");
			shm_unlink(POSIX_SHM_NAME);
			exit(EXIT_FAILURE);
		}

		/* page was not mapped prior this write and mprotect
		 * changed vma flags */
		*mem = x86_opcode_ret;

		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					shm_unlink(POSIX_SHM_NAME);
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))mem)();

		close(fd);
		shm_unlink(POSIX_SHM_NAME);
		munmap(mem, PAGE_SIZE);
	}

	/* example 10 */
	{
		int fd = shm_open(POSIX_SHM_NAME, POSIX_SHM_OFLAGS, POSIX_SHM_MODE);
		if(fd < 0) {
			perror("shm_open");
			exit(EXIT_FAILURE);
		}

		if(ftruncate(fd, PAGE_SIZE)) {
			perror("ftruncate");
			shm_unlink(POSIX_SHM_NAME);
			exit(EXIT_FAILURE);
		}

		char *mem = mmap(
				NULL,
				PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_SHARED,
				fd, 0);
		if(mem == MAP_FAILED) {
			perror("mmap");
			shm_unlink(POSIX_SHM_NAME);
			exit(EXIT_FAILURE);
		}

		check_scid_bcast_wxwarning(
				mem
				,
				*mem = x86_opcode_ret;
				,
		);

		((void(*)(void))mem)();

		close(fd);
		shm_unlink(POSIX_SHM_NAME);
		munmap(mem, PAGE_SIZE);
	}

	/* example 11 */
	{
		int shmid = shmget(SYSV_SHM_KEY, SYSV_SHM_SIZE, SYSV_SHM_FLG);
		if(shmid < 0) {
			perror("shmget");
			exit(EXIT_FAILURE);
		}

		/* if SHM_RDONLY enabled, the following mprotect(s) will fail with errno=EPERM */
		char *mem = shmat(shmid, NULL, /*SHM_RDONLY | */ SHM_EXEC);
		if(mem == (void*) -1) {
			perror("shmat");
			shmdt(mem);
			shmctl(shmid, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}

		/* PTE not populated yet... */
		if(mprotect(mem, PAGE_SIZE, PROT_WRITE)) {
			perror("mprotect");
			shmdt(mem);
			shmctl(shmid, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}

		/* page was not mapped prior this write and mprotect
		 * changed vma flags to wronly (x86 doesn't even care...) */
		*mem = x86_opcode_ret;

		/* pte is populated and mprotect MUST change not only the VMA but 
		 * the hw pte also */
		check_scid_bcast_wxwarning(
				mem
				,
				if(mprotect(mem, PAGE_SIZE, PROT_EXEC)) {
					perror("mprotect");
					shmdt(mem);
					shmctl(shmid, IPC_RMID, NULL);
					exit(EXIT_FAILURE);
				}
				,
		);

		((void(*)(void))mem)();

		shmdt(mem);
		shmctl(shmid, IPC_RMID, NULL);

		munmap(mem, PAGE_SIZE);
	}

	/* example 12 */
	{
		int shmid = shmget(SYSV_SHM_KEY, SYSV_SHM_SIZE, SYSV_SHM_FLG);
		if(shmid < 0) {
			perror("shmget");
			exit(EXIT_FAILURE);
		}

		char *mem = shmat(shmid, NULL, SHM_EXEC);
		if(mem == (void*) -1) {
			perror("shmat");
			shmdt(mem);
			shmctl(shmid, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}

		check_scid_bcast_wxwarning(
				mem
				,
				*mem = x86_opcode_ret;
				,
		);

		((void(*)(void))mem)();

		shmdt(mem);
		shmctl(shmid, IPC_RMID, NULL);

		munmap(mem, PAGE_SIZE);
	}

	/* example 13 */
	{
		char *mem = mmap(
				NULL, 
				3 * PAGE_SIZE, 
				PROT_EXEC, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				-1, 0);

		if(mprotect(page_nr(1), PAGE_SIZE, PROT_READ | PROT_WRITE)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		if(madvise(page_nr(1), PAGE_SIZE, MADV_POPULATE_READ | MADV_POPULATE_WRITE)) {
			perror("madvise");
			exit(EXIT_FAILURE);
		}

		spurious_byte_memread(ch, page_nr(1));
		
		check_scid_bcast_wxwarning(
				page_nr(1)
				,
				if(mprotect(page_nr(1), PAGE_SIZE, PROT_WRITE | PROT_EXEC)) {
					perror("mprotect");
					exit(EXIT_FAILURE);
				}
				,
		);

		spurious_byte_memwrite(page_nr(1), x86_opcode_ret);
		((void(*)(void)) mem)();

		if(mprotect(page_nr(2), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		check_scid_bcast_wxwarning(
				page_nr(2)
				,
				spurious_byte_memwrite(page_nr(2), x86_opcode_ret);
				,
		);

		if(mprotect(page_nr(3), PAGE_SIZE, PROT_WRITE | PROT_EXEC | PROT_READ)) {
			perror("mprotect");
			exit(EXIT_FAILURE);
		}

		check_scid_bcast_wxwarning(
				page_nr(3)
				,
				if(madvise(page_nr(3), PAGE_SIZE, MADV_POPULATE_READ | MADV_POPULATE_WRITE)) {
					perror("madvise");
					exit(EXIT_FAILURE);
				}
				,
				);
		munmap(mem, 3 * PAGE_SIZE);
	}

	example_passed();
	return EXIT_SUCCESS;
}
