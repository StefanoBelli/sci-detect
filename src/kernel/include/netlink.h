#ifndef SCID_NETLINK_H
#define SCID_NETLINK_H

extern struct genl_family genl_fam;

/**
 * setup_netlink - setup the netlink socket used to
 * communicate with userspace
 *
 * Returns: 0 if success, < 0 otherwise
 */
int setup_netlink(void);

/**
 * teardown_netlink - teardown the netlink socket
 */
void teardown_netlink(void);

#endif
