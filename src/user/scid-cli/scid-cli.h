#ifndef SCID_CLI_H
#define SCID_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#include <scid.h>

#define __unused __attribute__((__unused__))

#define __die_if(sym, errout, ...) \
	do { \
		long err = sym(__VA_ARGS__); \
		if (err) { \
			fprintf(stderr, #sym errout); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#define __expand(...) __VA_ARGS__

#define die_if_sciderr(sym, ...) \
	__die_if(sym, __expand(": %s\n", str_sciderr(err)), __VA_ARGS__)

#define die_if_nlerr(sym, ...) \
	__die_if(sym, __expand(" (nlerr) : %ld\n", err),  __VA_ARGS__)

#define __static_array_size(x) \
	(sizeof(x) / sizeof(x[0]))

#define mod_none(x) (x)
#define mod_isprint(x) isprint(x) ? (x) : '.'

#define GEN_BUFS_ACCESSES_16(__buf, __i, __mod) \
	__mod((unsigned char) (__buf)[(__i) + 0]), __mod((unsigned char) (__buf)[(__i) + 1]), \
	__mod((unsigned char) (__buf)[(__i) + 2]), __mod((unsigned char) (__buf)[(__i) + 3]), \
	__mod((unsigned char) (__buf)[(__i) + 4]), __mod((unsigned char) (__buf)[(__i) + 5]), \
	__mod((unsigned char) (__buf)[(__i) + 6]), __mod((unsigned char) (__buf)[(__i) + 7]), \
	__mod((unsigned char) (__buf)[(__i) + 8]), __mod((unsigned char) (__buf)[(__i) + 9]), \
	__mod((unsigned char) (__buf)[(__i) + 10]), __mod((unsigned char) (__buf)[(__i) + 11]), \
	__mod((unsigned char) (__buf)[(__i) + 12]), __mod((unsigned char) (__buf)[(__i) + 13]), \
	__mod((unsigned char) (__buf)[(__i) + 14]), __mod((unsigned char) (__buf)[(__i) + 15]) \

/* some utils */

static const char* bool_to_str(unsigned long t)
{
	return t ? "true" : "false";
}

static unsigned long to_ul(const char* s)
{
	errno = 0;
	char *endptr = NULL;

	unsigned long rv = strtoul(s, &endptr, 10);
	if(errno) {
		perror("strtoul");
		exit(EXIT_FAILURE);
	}

	return rv;
}

static void datetime_str(time_t time, char *buf, size_t max_size)
{
    struct tm tinfo;

    if (localtime_r(&time, &tinfo) == NULL)
        return;

    strftime(buf, max_size, "%Y-%m-%d %H:%M:%S", &tinfo);
}

static void print_hexdump(const char* buf)
{
	puts("---[ hexdump below ]---");
	for(size_t i = 0; i < SCID_PAGE_SIZE; i += 16)
		printf(
				"%ld:\t"
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\t"
				"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
					i, 
					GEN_BUFS_ACCESSES_16(buf, i, mod_none), 
					GEN_BUFS_ACCESSES_16(buf, i, mod_isprint));
	puts("---[ hexdump above ]---");
}

#undef GEN_BUFS_ACCESSES_16
#undef mod_none
#undef mod_isprint

#endif
