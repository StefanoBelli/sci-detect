#define _GNU_SOURCE
#include <sys/uio.h>
#include "exampleutils.h"

int main()
{
	char* mem;
	pid_t child_pid;

	mem = mmap(
			NULL, 
			PAGE_SIZE, 
			PROT_READ | PROT_WRITE | PROT_EXEC, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	child_pid = fork();
	if(!child_pid) {
		char buf[PAGE_SIZE];
		memset(buf, x86_opcode_ret, PAGE_SIZE);
		struct iovec local_iov[1];
		struct iovec remote_iov[1];
		size_t ret;

		local_iov[0].iov_base = buf;
		local_iov[0].iov_len = PAGE_SIZE;

		remote_iov[0].iov_base = mem;
		remote_iov[0].iov_len = PAGE_SIZE;

		check_scid_bcast_wxwarning(
				mem
				,
				ret = process_vm_writev(getppid(), local_iov, 1, remote_iov, 1, 0);
				if(ret != PAGE_SIZE) {
					perror("process_vm_writevv");
				}
			,
		);

		exit(EXIT_SUCCESS);
	} else if(child_pid < 0) {
		perror("fork");
		return EXIT_FAILURE;
	} else
		wait_for_child(child_pid);

	if(munmap(mem, PAGE_SIZE))
		perror("munmap");

	example_passed();
	return EXIT_SUCCESS;
}
