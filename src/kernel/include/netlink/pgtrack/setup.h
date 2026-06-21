#ifndef SCID_NETLINK_PGTRACK_SETUP_H
#define SCID_NETLINK_PGTRACK_SETUP_H

/*
 * this header file shall be included only by
 * the translation unit that performs setup/teardown
 * of this kernel module
 */

/**
 * setup_pgtrack_netlink - setup pgtrack netlink support
 *
 * Returns 0 if everything ok, < 0 otherwise
 */
int setup_pgtrack_netlink(void);

/**
 * teardown_pgtrack_netlink - teardown pgtrack netlink support
 */
void teardown_pgtrack_netlink(void);

#endif
