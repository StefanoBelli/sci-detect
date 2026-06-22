#ifndef SCID_H
#define SCID_H

/* this is the user/kernel shared header */
#include <scid-netlink-defs.h>

#include <stdint.h>

#define SCID_NL_SKALLOC_FAILURE ((void*) -1)
#define SCID_NL_SKCONN_FAILURE ((void*) -2)
#define SCID_NL_SKRESOLVE_NAME_FAILURE ((void*) -3)
#define SCID_NL_SKRESOLVE_GROUP_NAME_FAILURE ((void*) -4)
#define SCID_NL_DESC_ALLOC ((void*) -5)
#define SCID_NL_SKADDMEMB_FAILURE ((void*) -6)
#define SCID_NL_SKDROPMEMB_FAILURE ((void*) -7)
#define SCID_NL_MSG_ALLOC_FAILURE ((void*) -8)
#define SCID_NL_MSG_HDRPUT_FAILURE ((void*) -9)
#define SCID_NL_SKSEND_FAILURE ((void*) -10)
#define SCID_NL_SKRECV_FAILURE ((void*) -11)
#define SCID_INVALID_ARGS ((void*) -12)
#define SCID_REGIS_ZERO ((void*) -13)
#define SCID_REGI_ALLOC ((void*) -14)

/**
 * str_sciderr - get error string from error code
 *
 * @err: the error
 *
 * Returns: a string about the error, never NULL
 */
const char* str_sciderr(void* err);

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
int scid_poll_forever(void* desc, void* args, int *loop);

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

#endif
