#ifndef SCID_CLI_MACROS_H
#define SCID_CLI_MACROS_H

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

#endif
