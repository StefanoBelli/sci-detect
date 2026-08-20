#define _GNU_SOURCE
#include <sys/uio.h>
#include "exampleutils.h"

int main()
{
	pid_t child;
	char *mem;

	mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_ANONYMOUS | MAP_PRIVATE, 
			-1, 0);

	*mem = x86_opcode_ret;

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

	child = fork();
	if(!child) {
		char buf[PAGE_SIZE];
		struct iovec local_iov[1];
		struct iovec remote_iov[1];
		size_t ret;

		local_iov[0].iov_base = buf;
		local_iov[0].iov_len = PAGE_SIZE;

		remote_iov[0].iov_base = mem;
		remote_iov[0].iov_len = PAGE_SIZE;

		ret = process_vm_readv(getppid(), local_iov, 1, remote_iov, 1, 0);
		if(ret != PAGE_SIZE) {
			perror("process_vm_readv");
			goto __failure;
		}

		/* here we get the FIRST one, why is that? 
		 *
		 * Well, by doing process_vm_writev(getppid()) we broke CoW 
		 * for the REMOTE (parent) PROCESS, NOT OURS! 
		 */
		check_scid_bcast_snapshot(
				/* the virtual address */
				mem
				,
				/* the expected seq num */
				1
				,
				/* the expected fault */
				SNAPSHOT_WRITE_FAULT
				,
				/* the snapshot-triggering operation */
				ret = process_vm_writev(getppid(), local_iov, 1, remote_iov, 1, 0);
				if(ret != PAGE_SIZE) {
					perror("process_vm_writevv");
					goto __failure;
				}
				,
		);

		exit(EXIT_SUCCESS);
	}

	wait_for_child(child);

	/* here we get the SECOND one, again, this is the parent, 
	 * child ran process_vm_writev and indirectly changed our PTEs */
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

	example_passed();

__failure:
	munmap(mem, PAGE_SIZE);
	return EXIT_SUCCESS;
}
