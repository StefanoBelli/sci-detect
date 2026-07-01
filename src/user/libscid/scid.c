#include <netlink/genl/ctrl.h>

#include "scid-internals.h"

/* tunables */
#define NL_SOCKET_TXBUF_SIZE 0
#define NL_SOCKET_RXBUF_SIZE 65535

/* handle, descriptor */
struct scid_nl_sk {
	struct nl_sock *socket;
	int family_id;
	int group_id;
	struct cmd_reg_info *regi;
	uint8_t regi_size;
};

struct __main_nlmsg_cb_args {
	void *uargs;
	struct scid_nl_sk *desc;
};

static int __main_nlmsg_cb(struct nl_msg *msg, void *arg) 
{
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct nlattr *attrs[SCID_GENL_MAX_NR_ATTRS + 1];

	if (genlmsg_parse(hdr, 0, attrs, SCID_GENL_MAX_NR_ATTRS, global_policy) < 0)
		return NL_SKIP;

	struct genlmsghdr *genl_hdr = genlmsg_hdr(hdr);

	uint8_t cmd = genl_hdr->cmd;

	struct __main_nlmsg_cb_args *args = arg;

	for(uint8_t i = 0; i < args->desc->regi_size; i++) {
		struct cmd_reg_info *cur = &args->desc->regi[i];

		if(cmd == cur->cmd) {
			if(cur->user_handler)
				return cur->wrapper_handler(
						cur->user_handler, 
						attrs, 
						args->uargs);

			return NL_SKIP;
		}
	}

	return NL_SKIP;
}

const char *str_sciderr(long err)
{
	if(!err)
		return "success";
	else if(err == SCID_NL_SKALLOC_FAILURE)
		return "socket allocation failure";
	else if(err == SCID_NL_SKCONN_FAILURE)
		return "connection failure";
	else if(err == SCID_NL_SKRESOLVE_NAME_FAILURE)
		return "family resolution failure";
	else if(err == SCID_NL_SKRESOLVE_GROUP_NAME_FAILURE)
		return "group resolution failure";
	else if(err == SCID_NL_DESC_ALLOC)
		return "descriptor allocation failure";
	else if(err == SCID_NL_SKADDMEMB_FAILURE)
		return "unable to add to multicast group";
	else if(err == SCID_NL_SKDROPMEMB_FAILURE)
		return "unable to drop membership to multicast group";
	else if(err == SCID_NL_MSG_ALLOC_FAILURE)
		return "unable to alloc new message";
	else if(err == SCID_NL_MSG_HDRPUT_FAILURE)
		return "unable to put genl header";
	else if(err == SCID_NL_SKSEND_FAILURE)
		return "unable to send";
	else if(err == SCID_NL_SKRECV_FAILURE)
		return "unable to recv";
	else if(err == SCID_INVALID_ARGS)
		return "invalid arguments";
	else if(err == SCID_REGIS_ZERO)
		return "no compile-time registration";
	else if(err == SCID_REGI_ALLOC)
		return "registration allocation failure";
	else if(err == SCID_NL_SKSETBUFSIZE_FAILURE)
		return "unable to set socket bufsize";

	return "unknown failure";
}

long scid_regi_cmd(void* desc, uint8_t cmd, cmd_handler_fpt cmdh)
{
	struct scid_nl_sk *_desc = desc;

	if(!cmdh)
		return SCID_INVALID_ARGS;

	for(uint8_t i = 0; i < _desc->regi_size; i++) {
		if(_desc->regi[i].cmd == cmd) {
			_desc->regi[i].user_handler = cmdh;
			return 0;
		}
	}

	return SCID_INVALID_ARGS;
}

void *scid_new_socket(int *errored)
{
	struct nl_sock *socket;
	int family_id;
	int group_id;
	uint8_t nr_regis;

	if(!errored)
		return (void*) SCID_INVALID_ARGS;

	nr_regis = NR_STATIC_REGI_DEFS;
	if(!nr_regis)
		return (void*) SCID_REGIS_ZERO;

	*errored = 1;

	socket = nl_socket_alloc();
	if (!socket) 
		return (void*) SCID_NL_SKALLOC_FAILURE;

	if (genl_connect(socket) < 0) {
		nl_socket_free(socket);
		return (void*) SCID_NL_SKCONN_FAILURE;
	}

	/* ensure enough space for receiving some data */
	if(nl_socket_set_buffer_size(
				socket, NL_SOCKET_RXBUF_SIZE, NL_SOCKET_TXBUF_SIZE) < 0) {
		nl_socket_free(socket);
		return (void*) SCID_NL_SKSETBUFSIZE_FAILURE;
	}

	family_id = genl_ctrl_resolve(socket, SCID_GENL_NAME);
	if (family_id < 0) {
		nl_socket_free(socket);
		return (void*) SCID_NL_SKRESOLVE_NAME_FAILURE;
	}

	group_id = genl_ctrl_resolve_grp(socket, SCID_GENL_NAME, SCID_GENL_MCGRP_NAME);
	if (group_id < 0) {
		nl_socket_free(socket);
		return (void*) SCID_NL_SKRESOLVE_GROUP_NAME_FAILURE;
	}

	struct scid_nl_sk *desc = malloc(sizeof(struct scid_nl_sk));
	if(!desc) {
		nl_socket_free(socket);
		return (void*) SCID_NL_DESC_ALLOC;
	}

	desc->regi = malloc(sizeof(struct cmd_reg_info) * nr_regis);
	if(!desc->regi) {
		nl_socket_free(socket);
		free(desc);
		return (void*) SCID_REGI_ALLOC;
	}

	*errored = 0;

	desc->socket = socket;
	desc->family_id = family_id;
	desc->group_id = group_id;
	desc->regi_size = nr_regis;

	/* don't memcpy, explicit user_handler = NULL for each i */
	for(uint8_t i = 0; i < desc->regi_size; i++) {
		desc->regi[i].cmd = __static_regi_defs[i].cmd;
		desc->regi[i].wrapper_handler = __static_regi_defs[i].wrapper_handler;
		desc->regi[i].user_handler = NULL;
	}

	return desc;
}

long scid_broadcast_subscribe(void *desc)
{
	struct scid_nl_sk *_desc = desc;

	nl_socket_disable_seq_check(_desc->socket);

	if (nl_socket_add_memberships(
				_desc->socket, _desc->group_id, 0) < 0)

		return SCID_NL_SKADDMEMB_FAILURE;

	return 0;
}

long scid_broadcast_unsubscribe(void *desc)
{
	struct scid_nl_sk *_desc = desc;

	if (nl_socket_drop_memberships(
				_desc->socket, _desc->group_id, 0) < 0)

		return SCID_NL_SKDROPMEMB_FAILURE;

	return 0;
}

void scid_del_socket(void* desc)
{
	struct scid_nl_sk *_desc = desc;

	if(_desc) {
		if(_desc->socket) {
			nl_socket_free(_desc->socket);
			_desc->socket = NULL;
		}

		if(_desc->regi) {
			free(_desc->regi);
			_desc->regi = NULL;
		}

		free(_desc);
	}
}

int scid_poll_one_message(void* desc, void *args)
{
	struct scid_nl_sk *_desc = desc;
	int rv;

	/* this is ok, recvmsgs is sync and blocks thread */
	struct __main_nlmsg_cb_args nlargs = {
		.uargs = args,
		.desc = _desc,
	};

	nl_socket_modify_cb(
			_desc->socket, 
			NL_CB_VALID, NL_CB_CUSTOM, 
			__main_nlmsg_cb, &nlargs);

	rv = nl_recvmsgs_default(_desc->socket);
	if (rv < 0)
		return rv;

	return 0;
}

long scid_poll_forever(void *desc, void *args, int *loop)
{
	int rv;

	if(!loop)
		return SCID_INVALID_ARGS;

	while(*loop) {
		rv = scid_poll_one_message(desc, args);
		if(rv < 0)
			return rv;
	}

	return 0;
}

static long __scid_send_cmd_with_hdrflgs(
		void *desc, uint8_t cmd, void *args, 
		cmd_attrs_add_cb cb_put_attrs, 
		const void* cb_attrs_args, int hdrflgs)
{
	long rv;
	struct scid_nl_sk *_desc = desc;
	struct nl_msg *msg;
	void *hdr;

	msg = nlmsg_alloc();
	if(!msg)
		return SCID_NL_MSG_ALLOC_FAILURE;

	hdr = genlmsg_put(
			msg, NL_AUTO_PID, NL_AUTO_SEQ, _desc->family_id, 
			0, NLM_F_REQUEST | hdrflgs, cmd, SCID_GENL_VERSION);
	if(!hdr) {
		nlmsg_free(msg);
		return SCID_NL_MSG_HDRPUT_FAILURE;
	}

	if(cb_put_attrs) {
		rv = cb_put_attrs(msg, cb_attrs_args);
		if(rv) {
			nlmsg_free(msg);
			return rv;
		}
	}

	if(nl_send_auto(_desc->socket, msg) < 0) {
		nlmsg_free(msg);
		return SCID_NL_SKSEND_FAILURE;
	}

	nlmsg_free(msg);

	if(scid_poll_one_message(_desc, args) < 0)
		return SCID_NL_SKRECV_FAILURE;

	return 0;
}

long __scid_send_cmd(
		void *desc, uint8_t cmd, void *args, 
		cmd_attrs_add_cb cb_put_attrs, const void* cb_attrs_args)
{
	return __scid_send_cmd_with_hdrflgs(desc, cmd, args, cb_put_attrs, cb_attrs_args, 0);
}

long __scid_send_dump_cmd(
		void *desc, uint8_t cmd, void *args, 
		cmd_attrs_add_cb cb_put_attrs, const void* cb_attrs_args)
{
	return __scid_send_cmd_with_hdrflgs(desc, cmd, args, cb_put_attrs, cb_attrs_args, NLM_F_DUMP);
}
