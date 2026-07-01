#ifndef SCID_H
#define SCID_H

/* this is the user/kernel shared header */
#include <scid-netlink-defs.h>

#include <stdint.h>
#include <sys/types.h>

#define SCID_NL_SKALLOC_FAILURE (-1)
#define SCID_NL_SKCONN_FAILURE (-2)
#define SCID_NL_SKRESOLVE_NAME_FAILURE (-3)
#define SCID_NL_SKRESOLVE_GROUP_NAME_FAILURE (-4)
#define SCID_NL_DESC_ALLOC (-5)
#define SCID_NL_SKADDMEMB_FAILURE (-6)
#define SCID_NL_SKDROPMEMB_FAILURE (-7)
#define SCID_NL_MSG_ALLOC_FAILURE (-8)
#define SCID_NL_MSG_HDRPUT_FAILURE (-9)
#define SCID_NL_SKSEND_FAILURE (-10)
#define SCID_NL_SKRECV_FAILURE (-11)
#define SCID_INVALID_ARGS (-12)
#define SCID_REGIS_ZERO (-13)
#define SCID_REGI_ALLOC (-14)
#define SCID_NL_SKSETBUFSIZE_FAILURE (-15)

/**
 * str_sciderr - get error string from error code
 *
 * @err: the error
 *
 * Returns: a string about the error, never NULL
 */
const char* str_sciderr(long err);

/*
 * @cmd_data: the command output, depends on the particular cmd
 * @uargs: the user-provided args passed to the callback
 */
typedef void (*cmd_handler_fpt)(const void *cmd_data, void *uargs);

/**
 * scid_regi_cmd - register command handler
 *
 * @desc: the descriptor
 * @cmd: the cmd
 * @cmdh: the command handler, can't be NULL
 *
 * Returns: 0 if ok, not 0 othw.
 */
long scid_regi_cmd(void* desc, uint8_t cmd, cmd_handler_fpt cmdh);

/**
 * scid_new_socket - prepare a new socket
 *
 * This needs to be done before each op
 *
 * @errored: store there whether call was success or not
 *
 * Returns: a new descriptor or error, check errored, then rv
 */
void *scid_new_socket(int *errored);

/**
 * scid_broadcast_subscribe - subscribe to the broadcast group
 *
 * @desc: the descriptor
 *
 * Returns: 0 if ok, not 0 otherwise
 */
long scid_broadcast_subscribe(void* desc);

/**
 * scid_broadcast_unsubscribe - unsubscribe to the broadcast group
 *
 * @desc: the descriptor
 *
 * Returns: 0 if ok, not 0 otherwise
 */
long scid_broadcast_unsubscribe(void* desc);

/**
 * scid_del_socket - delete the socket and free resources
 *
 * @desc: the descriptor
 *
 * Remember that you are responsible of the lifecycle for
 * struct cmd_regi array
 */
void scid_del_socket(void* desc);

/**
 * scid_poll_one_message - await for one message
 *
 * Thread may remain stuck if not broadcast-subscribed
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 *
 * Returns: NOT A SCID_* ERROR, but a libnl error, or success (0)
 */
int scid_poll_one_message(void* desc, void* args);

/**
 * scid_poll_forever - await for messages forever
 *
 * Ideal for broadcast messages
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 * @loop: ptr to control loop at "recvmsg granularity"
 *
 * Returns: 0 if ok, not 0 othw
 */
long scid_poll_forever(void* desc, void* args, int *loop);

/**
 * scid_cmd_get_last_events - do a get_last_events cmd and await for
 * the response
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 *
 * Returns: 0 if ok, not 0 othw
 */
long scid_cmd_get_last_events(void *desc, void *args);

/**
 * scid_cmd_is_tracked_page - do a is_tracked_page cmd and await for
 * the response
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 * @pfn: the pfn to look for
 *
 * Returns: 0 if ok, not 0 othw
 */
long scid_cmd_is_tracked_page(void *desc, void *args, unsigned long pfn);

/**
 * scid_cmd_get_all_tracked_pages - do a get_all_tracked_pages cmd, await for
 * the dump
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 *
 * Returns: 0 if ok, not 0 othw
 */
long scid_cmd_get_all_tracked_pages(void *desc, void *args);

/**
 * scid_cmd_get_all_tracked_wx_pages - do a get_all_tracked_wx_pages cmd, await
 * for the dump
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 *
 * Returns: 0 if ok, not 0 othw
 */
long scid_cmd_get_all_tracked_wx_pages(void *desc, void *args);

/**
 * scid_cmd_get_one_last_event - do a get_one_last_event cmd and await for
 * the response
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 * @idx: the event idx 
 *
 * Returns: 0 if ok, not 0 othw
 */
long scid_cmd_get_one_last_event(void *desc, void *args, uint32_t idx);

/**
 * scid_cmd_get_one_last_event - do a get_cur_page_snapshot cmd and await for
 * the response
 *
 * @desc: the descriptor
 * @args: args to be passed to the command response handler callback
 * @pfn: the page pfn to look for
 *
 * Returns: 0 if ok, not 0 othw
 */
long scid_cmd_get_cur_page_snapshot(void *desc, void *args, unsigned long pfn);

/* these are passed as input to command callback handlers */

struct wxwarning_event {
	int32_t pid;
	uint64_t pfn;
	uint64_t va;
};

enum last_event_type {
	WXWARNING,
};

struct last_event {
	enum last_event_type type;
	void *data;
};

struct all_last_events {
	struct last_event *evts;
	uint32_t nr;
};

struct is_tracked_page {
	uint64_t pfn;
	uint32_t pfn_found;
	int32_t page_writable;
	int32_t page_executable;
	pid_t *pids;
	uint32_t nr_pids;
};

#if defined(__x86_64__) || defined(__i386__)
#	define SCID_PAGE_SIZE 4096
#endif

#endif
