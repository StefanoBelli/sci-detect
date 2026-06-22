#include <stdlib.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

/* this is for the libscid shared library */
#include <scid.h>

#define NL_SOCKET_TXBUF_SIZE 0
#define NL_SOCKET_RXBUF_SIZE 65535

struct cmd_reg_info;

typedef int (*internal_cmd_handler_fpt)(
		const struct cmd_reg_info* regi, 
		struct nlattr **attrs,
		void * uargs);

struct cmd_reg_info {
	uint8_t cmd;
	internal_cmd_handler_fpt wrapper_handler;
	cmd_handler_fpt user_handler;
};

/* prototypes for cmd/event response wrappers */
int __scid_wrapper_event_wxwarning(
		const struct cmd_reg_info *regi,
		struct nlattr **attrs, 
		void *uargs);

int __scid_wrapper_get_last_events(
		const struct cmd_reg_info *regi,
		struct nlattr **attrs, 
		void *uargs);

#define __static_array_size(x) (sizeof(x) / sizeof(typeof(x[0])))

/* TODO keep this in-sync */
static const struct cmd_reg_info __static_regi_defs[] = {
	{
		.cmd = SCID_GENL_CMD_EVENT_WXWARNING,
		.wrapper_handler = __scid_wrapper_event_wxwarning,
	},
	{
		.cmd = SCID_GENL_CMD_GET_LAST_EVENTS,
		.wrapper_handler = __scid_wrapper_get_last_events,
	}
};

struct scid_nl_sk {
	struct nl_sock *socket;
	int family_id;
	int group_id;
	struct cmd_reg_info *regi;
	uint8_t regi_size;
};

/* TODO keep this in-sync */
static const struct nla_policy global_policy[SCID_GENL_MAX_NR_ATTRS + 1] = {
	[SCID_GENL_ATTR_ARRAY_NR_ELEMS] = { .type = NLA_U32 },
	[SCID_GENL_ATTR_ARRAY] = { .type = NLA_NESTED },
	[SCID_GENL_ATTR_VA] = { .type = NLA_UINT },
	[SCID_GENL_ATTR_PFN] = { .type = NLA_UINT },
	[SCID_GENL_ATTR_PID] = { .type = NLA_S32 },
	[SCID_GENL_ATTR_EVT_TYPE] = { .type = NLA_U32 },
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
				return cur->wrapper_handler(cur, attrs, args->uargs);

			return NL_SKIP;
		}
	}

	return NL_SKIP;
}

const char *str_sciderr(void *err)
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

	return "unknown failure";
}

long scid_regi_cmd(void* desc, uint8_t cmd, cmd_handler_fpt cmdh)
{
	struct scid_nl_sk *_desc = desc;

	if(!cmdh)
		return (long) SCID_INVALID_ARGS;

	for(uint8_t i = 0; i < _desc->regi_size; i++) {
		if(_desc->regi[i].cmd == cmd) {
			_desc->regi[i].user_handler = cmdh;
			return 0;
		}
	}

	return (long) SCID_INVALID_ARGS;
}

void *scid_new_socket(int *errored)
{
	struct nl_sock *socket;
	int family_id;
	int group_id;
	uint8_t nr_regis;

	if(!errored)
		return SCID_INVALID_ARGS;

	nr_regis = __static_array_size(__static_regi_defs);
	if(!nr_regis)
		return SCID_REGIS_ZERO;

	*errored = 1;

	socket = nl_socket_alloc();
	if (!socket) 
		return SCID_NL_SKALLOC_FAILURE;

	/* ensure enough space for receiving some data */
	nl_socket_set_buffer_size(socket, NL_SOCKET_RXBUF_SIZE, NL_SOCKET_TXBUF_SIZE);

	if (genl_connect(socket) < 0) {
		nl_socket_free(socket);
		return SCID_NL_SKCONN_FAILURE;
	}

	family_id = genl_ctrl_resolve(socket, SCID_GENL_NAME);
	if (family_id < 0) {
		nl_socket_free(socket);
		return SCID_NL_SKRESOLVE_NAME_FAILURE;
	}

	group_id = genl_ctrl_resolve_grp(socket, SCID_GENL_NAME, SCID_GENL_MCGRP_NAME);
	if (group_id < 0) {
		nl_socket_free(socket);
		return SCID_NL_SKRESOLVE_GROUP_NAME_FAILURE;
	}

	struct scid_nl_sk *desc = malloc(sizeof(struct scid_nl_sk));
	if(!desc) {
		nl_socket_free(socket);
		return SCID_NL_DESC_ALLOC;
	}

	desc->regi = malloc(sizeof(struct cmd_reg_info) * nr_regis);
	if(!desc->regi) {
		nl_socket_free(socket);
		free(desc);
		return SCID_REGI_ALLOC;
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

		return (long) SCID_NL_SKADDMEMB_FAILURE;

	return 0;
}

long scid_broadcast_unsubscribe(void *desc)
{
	struct scid_nl_sk *_desc = desc;

	if (nl_socket_drop_memberships(
				_desc->socket, _desc->group_id, 0) < 0)

		return (long) SCID_NL_SKDROPMEMB_FAILURE;

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

int scid_poll_forever(void *desc, void *args, int *loop)
{
	int rv;

	if(!loop)
		return (long) SCID_INVALID_ARGS;

	while(*loop) {
		rv = scid_poll_one_message(desc, args);
		if(rv < 0)
			return rv;
	}

	return 0;
}

typedef long (*cmd_attrs_add_cb)(struct nl_msg *, const void*);

static long __scid_send_cmd(
		void *desc, uint8_t cmd, void *args, 
		cmd_attrs_add_cb cb_put_attrs, const void* cb_attrs_args)
{
	long rv;
	struct scid_nl_sk *_desc = desc;
	struct nl_msg *msg;
	void *hdr;

	msg = nlmsg_alloc();
	if(!msg)
		return (long) SCID_NL_MSG_ALLOC_FAILURE;

	hdr = genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, _desc->family_id, 0, 0, cmd, 0);
	if(!hdr) {
		nlmsg_free(msg);
		return (long) SCID_NL_MSG_HDRPUT_FAILURE;
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
		return (long) SCID_NL_SKSEND_FAILURE;
	}

	nlmsg_free(msg);

	if(scid_poll_one_message(_desc, args) < 0)
		return (long) SCID_NL_SKRECV_FAILURE;

	return 0;
}

/* 
 * command senders
 */

/* get_last_events cmd */

long scid_cmd_get_last_events(void *desc, void *args)
{
	return __scid_send_cmd(
			desc, SCID_GENL_CMD_GET_LAST_EVENTS, args, 
			NULL, NULL);
}

/*
 * response wrappers
 */

/* wxwarning event (broadcasted) */

int __scid_wrapper_event_wxwarning(const struct cmd_reg_info *regi,
		struct nlattr **attrs, void *uargs)
{
	struct wxwarning_event evt;

	memset(&evt, 0, sizeof(evt));

	if(attrs[SCID_GENL_ATTR_PID])
		evt.pid = nla_get_s32(attrs[SCID_GENL_ATTR_PID]);

	if(attrs[SCID_GENL_ATTR_PFN])
		evt.pfn = nla_get_uint(attrs[SCID_GENL_ATTR_PFN]);

	if(attrs[SCID_GENL_ATTR_VA])
		evt.va = nla_get_uint(attrs[SCID_GENL_ATTR_VA]);

	regi->user_handler(&evt, uargs);

	return NL_OK;
}

/* get_last_events cmd */

static void free_all_last_events(struct all_last_events *all)
{
	if(!all->nr || !all->evts)
		return;

	for(uint32_t i = 0; i < all->nr; i++) {
		void *data = all->evts[i].data;
		if(data)
			free(data);
	}

	free(all->evts);
}

static int populate_last_evt_wxwarning(struct last_event *evt, struct nlattr *attr)
{
	struct wxwarning_event *wxw;

	if(!evt->data) {
		evt->data = malloc(sizeof(struct wxwarning_event));
		if(!evt->data)
			return NL_STOP;
	}

	wxw = evt->data;

	if(nla_type(attr) == SCID_GENL_ATTR_VA)
		wxw->va = nla_get_uint(attr);
	else if(nla_type(attr) == SCID_GENL_ATTR_PID)
		wxw->pid = nla_get_s32(attr);
	else if(nla_type(attr) == SCID_GENL_ATTR_PFN)
		wxw->pfn = nla_get_uint(attr);

	return NL_OK;
}

int __scid_wrapper_get_last_events(const struct cmd_reg_info *regi,
		struct nlattr **attrs, void *uargs)
{
	int rv = NL_OK;
	int rem;
	struct nlattr *pos;
	struct all_last_events all_evts;

	memset(&all_evts, 0, sizeof(all_evts));

	if(!attrs[SCID_GENL_ATTR_ARRAY_NR_ELEMS])
		return NL_SKIP;

	all_evts.nr = nla_get_u32(attrs[SCID_GENL_ATTR_ARRAY_NR_ELEMS]);

	if(!attrs[SCID_GENL_ATTR_ARRAY])
		return NL_SKIP;

	if(!all_evts.nr)
		goto __finish;

	all_evts.evts = malloc(sizeof(struct last_event) * all_evts.nr);
	if(!all_evts.evts)
		return NL_STOP;

	memset(all_evts.evts, 0, all_evts.nr * sizeof(struct last_event));

	/* this may be a bit tricky... the evt type attr helps distinguish index */
	int i = -1;

	/* suppress compiler warning about non-initialized var usage */
	enum last_event_type i_evt_type = WXWARNING;

	nla_for_each_nested(pos, attrs[SCID_GENL_ATTR_ARRAY], rem) {
		int pos_is_evt_type = nla_type(pos) == SCID_GENL_ATTR_EVT_TYPE;

		/* consistency checks */
		if(i < 0 && !pos_is_evt_type)
			abort();

		if(pos_is_evt_type) {
			i_evt_type = nla_get_u32(pos);

			i++;

			/* consistency checks */
			if(i < 0 || i >= (int) all_evts.nr) 
				abort();

			all_evts.evts[i].type = i_evt_type;
			continue;
		}

		if(i_evt_type == WXWARNING)
			rv = populate_last_evt_wxwarning(&all_evts.evts[i], pos);

		if(rv != NL_OK)
			goto __finish_onlyfree;
	}

__finish:
	regi->user_handler(&all_evts, uargs);

__finish_onlyfree:
	free_all_last_events(&all_evts);

	return rv;
}
