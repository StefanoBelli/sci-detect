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

static void enable_testing_for_me(const char *subsys)
{
	enable_testing_for(subsys, gettid());
}

static void disable_testing_for_me(const char* subsys)
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

static void start_value_testing_for_me(const char* subsys, const char *key)
{
	start_value_testing_for(subsys, key, gettid());
}

static void stop_value_testing_for_me(const char* subsys, const char *key)
{
	stop_value_testing_for(subsys, key, gettid());
}

static void reset_value_testing_for_me(const char* subsys, const char *key)
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

static void query_value_testing_for_me(const char* subsys, const char* key, char* out, size_t outsize)
{
	query_value_testing_for(subsys, key, gettid(), out, outsize);
}
	
#undef BASEDIR

#endif
