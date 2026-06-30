#include <net/genetlink.h>

#include <netlink.h>
#include <logging.h>
#include <user/scid-netlink-defs.h>
#include <netlink/pgtrack/cmds.h>

#define __static_array_size(a) \
	(sizeof(a) / sizeof(typeof(a[0])))

#define __NLA_POLICY_SYM(name) genl_cmd_##name##_policy

#define __DECLARE_NLA_POLICY(name) \
	static const struct nla_policy 	\
	__NLA_POLICY_SYM(name)[SCID_GENL_MAX_NR_ATTRS + 1]

__DECLARE_NLA_POLICY(pfn_only_policy) = {
	[SCID_GENL_ATTR_PFN] = { .type = NLA_U64 },
};

static const struct genl_multicast_group genl_mcgrp[] = {
	{
		.name = SCID_GENL_MCGRP_NAME,
		.flags = GENL_MCAST_CAP_SYS_ADMIN,
	},
};

static const struct genl_ops genl_ops[] = {
	{
		.cmd = SCID_GENL_CMD_GET_LAST_EVENTS,
		.flags = GENL_ADMIN_PERM,
		.doit = pgtrack_genl_get_last_events_doit,
		.policy = NULL,
	},
	
	{
		.cmd = SCID_GENL_CMD_IS_TRACKED_PAGE,
		.flags = GENL_ADMIN_PERM,
		.doit = pgtrack_genl_is_tracked_page_doit,
		.policy = __NLA_POLICY_SYM(pfn_only_policy),
	}

};

struct genl_family genl_fam = {
	.name = SCID_GENL_NAME,
	.version = SCID_GENL_VERSION,
	.parallel_ops = true,
	.maxattr = SCID_GENL_MAX_NR_ATTRS,
	.ops = genl_ops,
	.n_ops = __static_array_size(genl_ops),
	.mcgrps = genl_mcgrp,
	.n_mcgrps = __static_array_size(genl_mcgrp)
};

int setup_netlink(void)
{
	int rv;

	rv = genl_register_family(&genl_fam);
	if(rv)
		scid_errf("unable to register netlink socket: %d", rv);

	return rv;
}

void teardown_netlink(void)
{
	int rv;

	rv = genl_unregister_family(&genl_fam);
	if(rv)
		scid_errf("unable to unregister netlink socket: %d", rv);
}

