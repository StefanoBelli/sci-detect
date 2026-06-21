#include <hooks.h>
#include <vmfs.h>
#include <logging.h>
#include <resolve_syms.h>
#include <pgtrack.h>
#include <netlink.h>
#include <netlink/pgtrack/setup.h>
#include <testing/testing.h>

MODULE_AUTHOR("Stefano Belli");
MODULE_DESCRIPTION("Stealth code injection detector");
MODULE_LICENSE("GPL");

int setup_module(void);
void teardown_module(void);

int setup_module(void) 
{
	int rv = 0;

	rv = setup_resolve_all_syms();
	if(rv) {
		scid_errf("setup_resolve_all_syms failed with rv=%d", rv);
		return rv;
	}

	rv = setup_testing();
	if(rv) {
		scid_errf("setup_testing failed with rv=%d", rv);
		return rv;
	}

	rv = setup_pgtrack_netlink();
	if(rv) {
		scid_errf("setup_pgtrack_netlink failed with rv=%d", rv);
		goto __teardown_from_testing;
	}

	rv = setup_netlink();
	if(rv) {
		scid_errf("setup_netlink failed with rv=%d", rv);
		goto __teardown_from_pgtrack_netlink;
	}

	rv = setup_pgtrack();
	if(rv) {
		scid_errf("setup_pgtrack failed with rv=%d", rv);
		goto __teardown_from_netlink;
	}

	rv = setup_vmfs_pcp_lists();
	if(rv) {
		scid_errf("setup_vmfs_pcp_lists failed with rv=%d", rv);
		goto __teardown_from_pgtrack;
	}

	rv = setup_hooks();
	if (rv) {
		scid_errf("setup_hooks failed with rv=%d", rv);
		goto __teardown_from_vmfs_pcp_lists;
	}

	return rv;

__teardown_from_vmfs_pcp_lists:
	teardown_vmfs_pcp_lists();
__teardown_from_pgtrack:
	teardown_pgtrack();
__teardown_from_netlink:
	teardown_netlink();
__teardown_from_pgtrack_netlink:
	teardown_pgtrack_netlink();
__teardown_from_testing:
	teardown_testing();

	return rv;
}

void teardown_module(void) 
{
	teardown_hooks();
	teardown_vmfs_pcp_lists();
	teardown_pgtrack();
	teardown_netlink();
	teardown_pgtrack_netlink();
	teardown_testing();
}

module_init(setup_module);
module_exit(teardown_module);
