#include <netlink/pgtrack/setup.h>
#include <logging.h>

#include "pgtrack-genl-events.h"

int setup_pgtrack_netlink(void)
{
	int rv;

	/* first, setup the fifo, size will be ROUNDED UP TO the nearest 2^n !!! */
	rv = kfifo_alloc(&le, __fifo_bytes(NR_QUEUED_EVENTS), GFP_KERNEL);
	if(rv) {
		scid_err("unable to setup kfifo");
		return rv;
	}

	/* then setup the ordered wq */
	bcast_evt_wq = alloc_ordered_workqueue("scid-bcast-evt", 0);
	if(!bcast_evt_wq) {
		scid_err("unable to allocate wq");
		free_le_kfifo();
		return -1;
	}

	return 0;
}

void teardown_pgtrack_netlink(void)
{
	/* destroy the wq */
	drain_workqueue(bcast_evt_wq);
	destroy_workqueue(bcast_evt_wq);

	/* ... then the fifo */
	free_le_kfifo();
}
