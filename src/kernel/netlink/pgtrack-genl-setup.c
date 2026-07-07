#include <netlink/pgtrack/setup.h>
#include <logging.h>

#include "pgtrack-genl-events.h"

int setup_pgtrack_netlink(void)
{
	int rv;

	event_cachep = kmem_cache_create(
			"scid__event_cache", 
			sizeof(struct event), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);
	if(!event_cachep) {
		scid_err("unable to create new kmem_cache");
		return -ENOMEM;
	}

	do_event_bcast_work_cachep = kmem_cache_create(
			"scid__do_event_bcast_work_cache", 
			sizeof(struct do_event_bcast_work), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);
	if(!do_event_bcast_work_cachep) {
		scid_err("unable to create new kmem_cache");
		goto __destroy_from_event_cachep;
	}

	/* first, setup the fifo, size will be ROUNDED UP TO the nearest 2^n !!! */
	rv = kfifo_alloc(&le, __fifo_bytes(NR_QUEUED_EVENTS), GFP_KERNEL);
	if(rv) {
		scid_errf("unable to setup kfifo (rv=%d)", rv);
		goto __destroy_from_bcast_cachep;
	}

	bcast_evt_wq = alloc_ordered_workqueue("scid-bcast-evt", 0);
	if(!bcast_evt_wq) {
		scid_err("unable to allocate wq");
		goto __destroy_from_kfifo;
	}

	return 0;

__destroy_from_kfifo:
	free_le_kfifo();
__destroy_from_bcast_cachep:
	kmem_cache_destroy(do_event_bcast_work_cachep);
__destroy_from_event_cachep:
	kmem_cache_destroy(event_cachep);
	return -1;
}

void teardown_pgtrack_netlink(void)
{
	drain_workqueue(bcast_evt_wq);
	destroy_workqueue(bcast_evt_wq);
	free_le_kfifo();
	kmem_cache_destroy(do_event_bcast_work_cachep);
	kmem_cache_destroy(event_cachep);
}
