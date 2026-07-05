#ifndef SCID_CLI_H
#define SCID_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <dis-asm.h>

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

static int __fprintf_styled(
		void *stream, 
		__unused enum disassembler_style st, 
		const char *fmt, ...) 
{
    va_list args;
    int r;
    va_start(args, fmt);
    r = vfprintf(stream, fmt, args);
    va_end(args);

    return r;
}

static void print_disasm(
		const char* buf, 
		unsigned long base_va, 
		size_t buffer_len)
{
	struct disassemble_info info;
	disassembler_ftype disasm;
	size_t pc;

	init_disassemble_info(
			&info, 
			stdout, 
			(fprintf_ftype) fprintf,
			__fprintf_styled);

	info.arch = bfd_arch_i386;
	info.mach = bfd_mach_x86_64;
	info.endian = BFD_ENDIAN_LITTLE;
	info.read_memory_func = buffer_read_memory;
	info.buffer = (unsigned char*) buf;
	info.buffer_vma = base_va;
	info.buffer_length = buffer_len;

	disassemble_init_for_target(&info);
	disasm = disassembler(
			info.arch, 
			info.endian == BFD_ENDIAN_BIG, 
			info.mach, 
			NULL);

	if(!disasm) {
		fputs("unable to obtain disasm fn", stderr);
		return;
	}

	printf("---[ disasm begins below: len=%ldB, assuming base va=0x%08lx ]---\n", 
			info.buffer_length, info.buffer_vma);

	pc = info.buffer_vma;
	while(pc < info.buffer_vma + info.buffer_length) {
		printf("%08lx: ", pc);

		int count = disasm(pc, &info);
		if(count <= 0) {
			fprintf(stderr, "ILLEGAL INSTRUCTION\n");
			return;
		}

		puts("");
		pc += count;
	}

	printf("---[ disasm above: len=%ldB, assumed base va=0x%08lx ] ---\n", 
			info.buffer_length, info.buffer_vma);
}

#endif
