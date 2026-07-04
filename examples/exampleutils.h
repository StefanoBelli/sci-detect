#ifndef EXAMPLE_UTILS_H
#define EXAMPLE_UTILS_H

/* don't put feature test macros here,
 * put them in each translation unit to avoid
 * confusion
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "examplemremap.h"

/* sysv shm common defs */

#define SYSV_SHM_KEY 0xdeadbeef
#define SYSV_SHM_SIZE PAGE_SIZE
#define SYSV_SHM_FLG (IPC_CREAT | IPC_EXCL)

#define SYSV_NO_EXCL_CREAT ~(IPC_CREAT | IPC_EXCL)

/* posix shm common defs */

#define POSIX_SHM_NAME "example-shm"
#define POSIX_SHM_OFLAGS (O_RDWR | O_CREAT | O_EXCL)
#define POSIX_SHM_MODE 0700

#define POSIX_NO_EXCL_CREAT ~(O_CREAT | O_EXCL)

#ifdef EXAMPLE_CHECK_WITH_LIBSCID
#include <scid.h>
#endif

#define __unused __attribute__((__unused__))

/*
 * this may be required when MAP_SHARED is used
 * in conjunction with file-backed memory to do
 * the examples correctly (expected behaviour,
 * documented in docs/)
 */

/* this may be a regular subroutine, macro avoid the need to
 * specify ftm for each TU */
#define __DROPC_STR "1\n"

#define flush_page_cache() \
	do { \
		sync(); \
		\
		int _____fd____ = open("/proc/sys/vm/drop_caches", O_WRONLY, 0); \
		if(_____fd____ < 0) { \
			perror("flush_page_cache's open"); \
			exit(EXIT_FAILURE); \
		} \
		\
		ssize_t dropc_len = strlen(__DROPC_STR); \
		\
		if(write(_____fd____, __DROPC_STR, dropc_len) != dropc_len) { \
			perror("flush_page_cache's write"); \
			close(_____fd____); \
			exit(EXIT_FAILURE); \
		} \
		\
		close(_____fd____); \
		\
		puts("NOTE: page cache flushed correctly"); \
		\
	} while(0)

#define full_membar() \
	__asm__ __volatile__("mfence;" ::: "memory")

#define spurious_byte_memwrite(ptr, value) \
	*((volatile char*)ptr) = value; \
	full_membar()

#define spurious_byte_memread(varname, ptr) \
	__unused volatile char varname = *(ptr); \
	full_membar()

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

#define x86_opcode_ret (0xc3)

#ifdef EXAMPLE_CHECK_WITH_LIBSCID
#define die_if(x, msg) \
	if((x)) { \
		perror(msg); \
		_exit(EXIT_FAILURE); \
	}

#define __scid_die_if_ce(msg, cond, err) \
	if((cond)) { \
		fprintf(stderr, #msg " failed: %s\n", str_sciderr((err))); \
		_exit(EXIT_FAILURE); \
	} 

#define __scid_die_if(msg, cond) \
	__scid_die_if_ce(msg, cond, cond)

struct __recvd_event {
	enum last_event_type type;
	union {
		struct wxwarning_event wxw;
		struct snapshot_event snap;
	} event;
};

__unused static void __event_wxwarning_cmdh(const void *in, void *out)
{
	const struct wxwarning_event *event = in;
	struct __recvd_event *uevent = out;
	
	uevent->type = WXWARNING;

	uevent->event.wxw.pfn = event->pfn;
	uevent->event.wxw.pid = event->pid;
	uevent->event.wxw.va = event->va;
}

__unused static void __event_snapshot_cmdh(const void *in, void *out)
{
	const struct snapshot_event *event = in;
	struct __recvd_event *uevent = out;
	
	uevent->type = SNAPSHOT;

	uevent->event.snap.pfn = event->pfn;
	uevent->event.snap.pid = event->pid;
	uevent->event.snap.va = event->va;
	uevent->event.snap.seq = event->seq;
	uevent->event.snap.datetime = event->datetime;
	uevent->event.snap.fault = event->fault;
	memcpy(uevent->event.snap.buffer, event->buffer, SCID_PAGE_SIZE);
}

__unused static void *__scid_setup()
{
	void *desc;
	int new_err;
	long err;

	desc = scid_new_socket(&new_err);
	__scid_die_if_ce("scid_new_socket", new_err, (long) desc);

	err = scid_regi_cmd(
			desc, 
			SCID_GENL_CMD_EVENT_WXWARNING, 
			__event_wxwarning_cmdh);
	__scid_die_if("scid_regi_cmd", err);

	err = scid_regi_cmd(
			desc, 
			SCID_GENL_CMD_EVENT_SNAPSHOT, 
			__event_snapshot_cmdh);
	__scid_die_if("scid_regi_cmd", err);

	err = scid_broadcast_subscribe(desc);
	__scid_die_if("scid_broadcast_subscribe", err);

	return desc;
}

__unused static inline void __scid_terminate(void *desc)
{
	scid_del_socket(desc);
}

#define __wxwarning_test_block(_evt, _va) \
	if((_evt).type == WXWARNING) { \
	 	struct wxwarning_event *wxw = &(_evt).event.wxw; \
	 	if(wxw->va != ((unsigned long) (_va)) || wxw->pid != getpid()) { \
	 		fprintf(stderr, "FAILED attempt (remaining retries: %d)\n", --nr_retry); \
	 		if(!nr_retry) { \
	 			example_failed(); \
	 			_exit(EXIT_FAILURE); \
	 		} \
	 	} else { \
	 		wxw_count++; \
	 		printf("YES! we got the wxwarning!\n" \
	 				"\t--> pfn: %ld\n" \
	 				"\t--> pid: %d\n" \
	 				"\t--> va: 0x%lx\n", \
	 				wxw->pfn, wxw->pid, wxw->va); \
	 		if(wxw_count == 2) \
	 			break; \
	 	} \
	} else if((_evt).type == SNAPSHOT) { \
		struct snapshot_event *snap = &(_evt).event.snap; \
		if( \
				snap->va != ((unsigned long) (_va)) || \
				snap->pid != getpid() || \
				snap->seq != 1){ \
			\
			char *oldbuf = (_va); \
			if(snap->fault == SNAPSHOT_WRITE_FAULT) \
				oldbuf = pre_or_post_op_buffer; \
			\
			if(memcmp(snap->buffer, oldbuf, SCID_PAGE_SIZE)) { \
	 			fprintf(stderr, "FAILED attempt (remaining retries: %d)\n", --nr_retry); \
	 			if(!nr_retry) { \
	 				example_failed(); \
	 				_exit(EXIT_FAILURE); \
	 			} \
	 		} \
	 	} else { \
	 		wxw_count++; \
	 		printf("YES! we got the snapshot!\n" \
	 				"\t--> fault: %d\n" \
	 				"\t--> seq: %ld\n" \
	 				"\t--> datetime: %ld\n", \
	 				snap->fault, snap->seq, snap->datetime); \
	 		if(wxw_count == 2) \
	 			break; \
	 	} \
	}

#define __check_scid_bcast_base(__pre_op, __op, __post_op, __ret, __cond_var__, __test_block__) \
	({ \
	 	void *desc = __scid_setup(); \
	 	__pre_op \
	 	__op \
	 	__post_op \
	 	int nr_retry = 3; \
	 	__cond_var__ \
	 	while(1) { \
	 		struct __recvd_event bcasted_event; \
	 		scid_poll_one_message(desc, &bcasted_event); \
	 		__test_block__ \
	 	} \
	 	__scid_terminate(desc); \
	 	__ret \
	})

#define check_scid_bcast_wxwarning_pre(_va, op, ret) \
	__check_scid_bcast_base( \
			char pre_or_post_op_buffer[SCID_PAGE_SIZE]; \
			memcpy(pre_or_post_op_buffer, (char*) (_va), SCID_PAGE_SIZE); \
			, \
			op \
			, \
			, \
			ret \
			, \
			int wxw_count = 0; \
			, \
			__wxwarning_test_block(bcasted_event, _va))

#define check_scid_bcast_wxwarning_post(_va, op, ret) \
	__check_scid_bcast_base( \
			, \
			op \
			, \
			char pre_or_post_op_buffer[SCID_PAGE_SIZE]; \
			memcpy(pre_or_post_op_buffer, (char*) (_va), SCID_PAGE_SIZE); \
			, \
			ret \
			, \
			int wxw_count = 0; \
			, \
			__wxwarning_test_block(bcasted_event, _va))

#define check_scid_bcast_wxwarning(_va, op, ret) \
	check_scid_bcast_wxwarning_pre(_va, op, ret)

#define example_passed() \
	puts("OK! Example passed!")

#define example_failed() \
	fputs("FAIL! Example failed!\n", stderr)

#else /* !EXAMPLE_CHECK_WITH_LIBSCID */

#define __check_scid_noop(__op, __ret) \
({ \
 	__op \
 	__ret \
})

#define check_scid_bcast_wxwarning(arg1, __op, __ret) \
	__check_scid_noop(__op, __ret)

#define example_passed()

#define example_failed()

#endif /* EXAMPLE_CHECK_WITH_LIBSCID */

#define wait_for_child(pid) \
	do { \
		int status; \
		if(waitpid(child_pid, &status, 0) < 0) { \
			perror("waitpid"); \
			exit(EXIT_FAILURE); \
		} \
		\
		if(!WIFEXITED(status) || WEXITSTATUS(status)) { \
			fputs("child did not end well... bye bye :(\n", stderr); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#ifdef EXAMPLE_MLOCK_ALL
__unused static void __maybe_mlock_all_addr_space(void)
{
	if(mlockall(MCL_CURRENT | MCL_FUTURE)) {
		perror("mlockall");
		exit(EXIT_FAILURE);
	}

	puts("MLOCK_ALL: successfully locked the address space");
}
#else /* !EXAMPLE_MLOCK_ALL */
__unused static void __maybe_mlock_all_addr_space(void)
{
}
#endif /* EXAMPLE_MLOCK_ALL */

#endif 
