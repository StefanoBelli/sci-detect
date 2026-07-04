#ifndef SCID_NETLINK_PGTRACK_EVENTS_H
#define SCID_NETLINK_PGTRACK_EVENTS_H

/*
 * this header file shall be included by the
 * translation units that need to broadcast event
 * to multicast group subscribers
 */

#include <linux/types.h>

/* fwd decl, see pgsnap.h */
struct page_snap;

/* fwd decl, see pgtrack.h */
struct page_wxwarn;

/**
 * bcast_pgtrack_event_wxwarning - broadcast WX page frame detection warning
 *
 * @wxwarn: ptr to wxwarn, transferring ownership to us
 *
 * Returns: true if everything ok, false otherwise
 */
bool bcast_pgtrack_event_wxwarning(const struct page_wxwarn *wxwarn);

/**
 * bcast_pgtrack_event_snapshot - broadcast a snapshot made on a WX page
 *
 * @snap: ptr to snap, transferring ownership to us
 *
 * Returns: true if everything ok, false otherwise
 */
bool bcast_pgtrack_event_snapshot(const struct page_snap *snap);

#endif
