#ifndef SCID_NETLINK_PGTRACK_EVENTS_H
#define SCID_NETLINK_PGTRACK_EVENTS_H

/*
 * this header file shall be included by the
 * translation units that need to broadcast event
 * to multicast group subscribers
 */

#include <linux/types.h>

/**
 * bcast_pgtrack_event_wxwarning - broadcast WX page frame detection warning
 *
 * @pfn: pfn of the WX-detected page
 * @pid: the pid of the task that triggered the WX-detection code
 * @va: the associated virtual address
 *
 * Returns: true if everything ok, false otherwise
 */
bool bcast_pgtrack_event_wxwarning(unsigned long pfn, pid_t pid, unsigned long va);

//bool bcast_pgtrack_event_pagecontent();

#endif
