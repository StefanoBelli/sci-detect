#ifndef SCID_NETLINK_PGTRACK_CMDS_H
#define SCID_NETLINK_PGTRACK_CMDS_H

/*
 * this header file shall be included by the
 * netlink-setup translation unit (see netlink.c)
 * provides prototypes for cmds implemented in
 * pgtrack-genl.c
 */

#include <linux/netlink.h>
#include <net/genetlink.h>

int pgtrack_genl_get_last_events_doit(struct sk_buff *, struct genl_info *);
int pgtrack_genl_is_tracked_page_doit(struct sk_buff *, struct genl_info *);
int pgtrack_genl_get_all_tracked_pages_dumpit(struct sk_buff *, struct netlink_callback *);
int pgtrack_genl_get_all_tracked_wx_pages_dumpit(struct sk_buff *, struct netlink_callback *);
int pgtrack_genl_get_one_last_event_doit(struct sk_buff *, struct genl_info *);
int pgtrack_genl_get_cur_page_snapshot_doit(struct sk_buff *, struct genl_info *);

#endif
