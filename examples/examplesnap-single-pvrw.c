#define _GNU_SOURCE
#include <sys/uio.h>
#include "exampleutils.h"

int main()
{
	char buf[PAGE_SIZE];
	struct iovec local_iov[1];
	struct iovec remote_iov[1];
	char *mem;
	size_t ret;

	mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_ANONYMOUS | MAP_PRIVATE, 
			-1, 0);

	/* here the first snapshot happens */
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

	local_iov[0].iov_base = buf;
	local_iov[0].iov_len = PAGE_SIZE;

	remote_iov[0].iov_base = mem;
	remote_iov[0].iov_len = PAGE_SIZE;

	ret = process_vm_readv(getpid(), local_iov, 1, remote_iov, 1, 0);
	if(ret != PAGE_SIZE) {
		perror("process_vm_readv");
		goto __failure;
	}

	/* here we get the third one */
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
			ret = process_vm_writev(getpid(), local_iov, 1, remote_iov, 1, 0);
			if(ret != PAGE_SIZE) {
				perror("process_vm_writevv");
				goto __failure;
			}
			,
	);

	/* no snapshot here */
	ret = process_vm_writev(getpid(), local_iov, 1, remote_iov, 1, 0);
	if(ret != PAGE_SIZE) {
		perror("process_vm_writevv");
		goto __failure;
	}

	/* here we get the fourth one */
	check_scid_bcast_snapshot(
			/* the virtual address */
			mem
			,
			/* the expected seq num */
			4
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
