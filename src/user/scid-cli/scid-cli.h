#ifndef SCID_CLI_H
#define SCID_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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

#endif
