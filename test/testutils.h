#ifndef TESTUTILS_H
#define TESTUTILS_H

#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define __unused __attribute__((__unused__))

#define BASEDIR "/sys/module/sci_detect/testing"

static void __enabledisable_testing_for(const char* enabledisable, const char* subsys, pid_t pid)
{
	char pathbuf[PATH_MAX];
	memset(pathbuf, 0, sizeof(pathbuf));

	snprintf(pathbuf, sizeof(pathbuf), BASEDIR "/%s/%s", subsys, enabledisable);

	int fd = open(pathbuf, O_WRONLY);
	if(fd < 0) {
		fprintf(stderr, "open(%s): %s\n", pathbuf, strerror(errno));
		exit(EXIT_FAILURE);
	}

	char pidbuf[100];
	memset(pidbuf, 0, sizeof(pidbuf));

	snprintf(pidbuf, sizeof(pidbuf), "%d", pid); 

	if(write(fd, pidbuf, strlen(pidbuf)) < 0) {
		close(fd);
		fprintf(stderr, "write(%s): %s\n", pathbuf, strerror(errno));
		exit(EXIT_FAILURE);
	}

	close(fd);
}

static void enable_testing_for(const char *subsys, pid_t pid)
{
	__enabledisable_testing_for("enable", subsys, pid);
}

static void disable_testing_for(const char* subsys, pid_t pid)
{
	__enabledisable_testing_for("disable", subsys, pid);
}

__unused static void enable_testing_for_me(const char *subsys)
{
	enable_testing_for(subsys, gettid());
}

__unused static void disable_testing_for_me(const char* subsys)
{
	disable_testing_for(subsys, gettid());
}

static void __writemethod_testing_key_for(const char *method, const char* subsys, const char* key, pid_t pid)
{
	char pathbuf[PATH_MAX];
	memset(pathbuf, 0, sizeof(pathbuf));

	snprintf(pathbuf, sizeof(pathbuf), BASEDIR "/%s/%d/%s/%s", subsys, pid, key, method); 

	int fd = open(pathbuf, O_WRONLY);
	if(fd < 0) {
		fprintf(stderr, "open(%s): %s\n", pathbuf, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* doesn't really matter */
	char pidbuf[100];
	memset(pidbuf, 0, sizeof(pidbuf));

	snprintf(pidbuf, sizeof(pidbuf), "%d", pid); 

	if(write(fd, pidbuf, strlen(pidbuf)) < 0) {
		close(fd);
		fprintf(stderr, "write(%s): %s\n", pathbuf, strerror(errno));
		exit(EXIT_FAILURE);
	}

	close(fd);
}

static void start_value_testing_for(const char* subsys, const char* key, pid_t pid)
{
	__writemethod_testing_key_for("start", subsys, key, pid);
}

static void stop_value_testing_for(const char* subsys, const char* key, pid_t pid)
{
	__writemethod_testing_key_for("stop", subsys, key, pid);
}

static void reset_value_testing_for(const char* subsys, const char* key, pid_t pid)
{
	__writemethod_testing_key_for("reset", subsys, key, pid);
}

__unused static void start_value_testing_for_me(const char* subsys, const char *key)
{
	start_value_testing_for(subsys, key, gettid());
}

__unused static void stop_value_testing_for_me(const char* subsys, const char *key)
{
	stop_value_testing_for(subsys, key, gettid());
}

__unused static void reset_value_testing_for_me(const char* subsys, const char *key)
{
	reset_value_testing_for(subsys, key, gettid());
}

static void __readmethod_testing_key_for(
		const char *method, const char* subsys, const char* key, pid_t pid, 
		char *outbuf, size_t outbuf_size)
{
	char pathbuf[PATH_MAX];
	memset(pathbuf, 0, sizeof(pathbuf));

	snprintf(pathbuf, sizeof(pathbuf), BASEDIR "/%s/%d/%s/%s", subsys, pid, key, method); 

	int fd = open(pathbuf, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "open(%s): %s\n", pathbuf, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(read(fd, outbuf, outbuf_size) < 0) {
		close(fd);
		fprintf(stderr, "read(%s): %s\n", pathbuf, strerror(errno));
		exit(EXIT_FAILURE);
	}

	close(fd);
}

static void query_value_testing_for(const char *subsys, const char *key, pid_t pid, char* out, size_t outsize)
{
	__readmethod_testing_key_for("query", subsys, key, pid, out, outsize);
}

__unused static void query_value_testing_for_me(const char* subsys, const char* key, char* out, size_t outsize)
{
	query_value_testing_for(subsys, key, gettid(), out, outsize);
}

static int query_int_value_testing_for(const char *subsys, const char* key, pid_t pid)
{
	char querybuf[100];
	memset(querybuf, 0, 100);

	query_value_testing_for(subsys, key, pid, querybuf, 100);

	return strtol(querybuf, NULL, 10);
}

__unused static int query_int_value_testing_for_me(const char* subsys, const char* key)
{
	return query_int_value_testing_for(subsys, key, gettid());
}

#undef BASEDIR

#define test_int_eq_hard(x, y) \
	if((x) != (y)) { \
		fprintf(stderr, #x " == " #y " FAILED (see " __FILE__ ":%d)\n", __LINE__); \
		fputs("\ttheir actual values are:\n", stderr); \
		fprintf(stderr ,"\t\t" #x " = %d\n", (x)); \
		fprintf(stderr, "\t\t" #y " = %d\n", (y)); \
		rv = EXIT_FAILURE; \
		goto __finish; \
	}

#define test_int_ge_hard(x, y) \
	if((x) < (y)) { \
		fprintf(stderr, #x " >= " #y " FAILED (see " __FILE__ ":%d)\n", __LINE__); \
		fputs("\ttheir actual values are:\n", stderr); \
		fprintf(stderr ,"\t\t" #x " = %d\n", (x)); \
		fprintf(stderr, "\t\t" #y " = %d\n", (y)); \
		rv = EXIT_FAILURE; \
		goto __finish; \
	} else if((x) > (y)) { \
		fprintf(stderr, "see " __FILE__ ":%d\n", __LINE__); \
		fprintf(stderr, "\t\t" #x " = %d\n", (x)); \
		fprintf(stderr, "\t\t" #y " = %d\n", (y)); \
	}

#ifndef SOFT_FAIL_TOLERANCE
#define SOFT_FAIL_TOLERANCE 1
#endif

#define test_int_eq(x, y) \
	if((x) != (y)) { \
		fprintf(stderr, #x " == " #y " %s FAILED (see " __FILE__ ":%d)\n", \
				(x) > (y) && (((x) - (y)) <= SOFT_FAIL_TOLERANCE) ? "SOFT" : "HARD", __LINE__); \
		fputs("\ttheir actual values are:\n", stderr); \
		fprintf(stderr ,"\t\t" #x " = %d\n", (x)); \
		fprintf(stderr, "\t\t" #y " = %d\n", (y)); \
		if(!((x) > (y) && (((x) - (y)) <= SOFT_FAIL_TOLERANCE))) { \
			rv = EXIT_FAILURE; \
			goto __finish; \
		} \
	}

#define die_if(expr) \
	if(expr) { \
		fprintf(stderr, "TEST FAILED because " #expr " is false, errno = %s " \
				"(see " __FILE__ ":%d)\n", strerror(errno), __LINE__); \
		rv = EXIT_FAILURE; \
		goto __finish; \
	}

#define test_passed() \
	puts("OK! All tests passed!"); \
	rv = EXIT_SUCCESS

#define full_membar() \
	__asm__ __volatile__("mfence;" ::: "memory")

#define spurious_byte_memwrite(ptr, value) \
	*((volatile char*)ptr) = value; \
	full_membar()

__unused static int trigger_syscall_pagewrite(void* addr, size_t len)
{
	int fd = open("/dev/random", O_RDONLY);
	if(fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	if(read(fd, addr, len) < 0) {
		close(fd);
		perror("read");
		return EXIT_FAILURE;
	}

	close(fd);
	return EXIT_SUCCESS;
}

#define spurious_byte_memread(varname, ptr) \
	__unused volatile char varname = *(ptr); \
	full_membar()

__unused static int trigger_syscall_pageread(void* addr, size_t len)
{
	int fd = open("/dev/null", O_WRONLY);
	if(fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	if(write(fd, addr, len) < 0) {
		close(fd);
		perror("write");
		return EXIT_FAILURE;
	}

	close(fd);
	return EXIT_SUCCESS;
}

#define __test_fork_and_wait(fncall) \
	do { \
		pid_t child_pid = fork(); \
		if(!child_pid) \
			exit(fncall); \
		int status; \
		die_if(child_pid < 0); \
		die_if(waitpid(child_pid, &status, 0) < 0); \
		die_if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS); \
	} while(0)

#ifndef __starting_mem_varname
#	define __starting_mem_varname mem
#endif

#if !defined(PAGE_SIZE)
#	if defined(__x86_64__) || defined(__i386__)
#		define PAGE_SIZE 4096
#	endif
#endif

#define page_nr(nr) \
	({ \
	 	_Static_assert((nr) > 0, "page index base is 1"); \
	 	(__starting_mem_varname + (((nr) - 1) * PAGE_SIZE)); \
	})

#endif
