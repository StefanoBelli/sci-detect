#ifndef EXAMPLE_UTILS_H
#define EXAMPLE_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

#ifdef EXAMPLE_CHECK_WITH_LIBSCID
#include <scid.h>
#endif

#define __unused __attribute__((__unused__))

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
		exit(EXIT_FAILURE); \
	}

#define __scid_die_if_ce(msg, cond, err) \
	if((cond)) { \
		fprintf(stderr, #msg " failed: %s\n", str_sciderr((err))); \
		exit(EXIT_FAILURE); \
	} 

#define __scid_die_if(msg, cond) \
	__scid_die_if_ce(msg, cond, cond)

struct __recvd_event {
	enum last_event_type type;
	union {
		struct wxwarning_event wxw;
	} event;
};

static void __event_wxwarning_cmdh(const void *in, void *out)
{
	const struct wxwarning_event *event = in;
	struct __recvd_event *uevent = out;
	
	uevent->type = WXWARNING;

	uevent->event.wxw.pfn = event->pfn;
	uevent->event.wxw.pid = event->pid;
	uevent->event.wxw.va = event->va;
}

static void *__scid_setup()
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

	err = scid_broadcast_subscribe(desc);
	__scid_die_if("scid_broadcast_subscribe", err);

	return desc;
}

static inline void __scid_terminate(void *desc)
{
	scid_del_socket(desc);
}

#define __wxwarning_test_block(_evt, _va) \
	if((_evt).type == WXWARNING) { \
	 	struct wxwarning_event *wxw = &(_evt).event.wxw; \
	 	if(wxw->va != ((unsigned long) (_va)) || wxw->pid != getpid()) { \
	 		if(!--nr_retry) { \
	 			example_failed(); \
	 			exit(EXIT_FAILURE); \
	 		} \
	 	} else { \
	 		printf("YES! we got the wxwarning!\n" \
	 				"\t--> pfn: %ld\n" \
	 				"\t--> pid: %d\n" \
	 				"\t--> va: 0x%lx\n", \
	 				wxw->pfn, wxw->pid, wxw->va); \
	 		break; \
	 	} \
	} \

#define __check_scid_bcast_base(__op, __ret, __test_block__, ...) \
	({ \
	 	void *desc = __scid_setup(); \
	 	__op \
	 	__VA_ARGS__ \
	 	int nr_retry = 3; \
	 	while(1) { \
	 		struct __recvd_event bcasted_event; \
	 		scid_poll_one_message(desc, &bcasted_event); \
	 		__test_block__ \
	 	} \
	 	__scid_terminate(desc); \
	 	__ret \
	})


#define check_scid_bcast_wxwarning(_va, op, ret, ...) \
	__check_scid_bcast_base( \
			op, \
			ret, \
			__wxwarning_test_block(bcasted_event, _va), \
			__VA_ARGS__)

#define example_passed() \
	puts("OK! Example passed!")

#define example_failed() \
	fputs("FAIL! Example failed!", stderr)

#else /* !EXAMPLE_CHECK_WITH_LIBSCID */

#define __check_scid_noop(__op, __ret, ...) \
({ \
 	__op \
 	__VA_ARGS__ \
 	__ret \
})

#define check_scid_bcast_wxwarning(arg1, __op, __ret, ...) \
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
			fputs("child did not end well... bye bye :(", stderr); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

#endif 
