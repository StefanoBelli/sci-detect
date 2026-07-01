#ifndef SCID_INTERNALS_H
#define SCID_INTERNALS_H

#include <scid.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>

typedef int (*internal_cmd_handler_fpt)(
		cmd_handler_fpt user_handler, 
		struct nlattr **attrs,
		void * uargs);

struct cmd_reg_info {
	uint8_t cmd;
	internal_cmd_handler_fpt wrapper_handler;
	cmd_handler_fpt user_handler;
};

extern const struct nla_policy global_policy[];
extern const uint8_t NR_STATIC_REGI_DEFS;
extern const struct cmd_reg_info __static_regi_defs[];

typedef long (*cmd_attrs_add_cb)(struct nl_msg *, const void*);

extern long __scid_send_cmd(
		void*, uint8_t, void*, 
		cmd_attrs_add_cb, const void*);

extern long __scid_send_dump_cmd(
		void *, uint8_t, void *, 
		cmd_attrs_add_cb, const void*);

#endif
