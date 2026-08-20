#include <hooks.h>
#include <kcps.h>
#include <logging.h>
#include <resolve_syms.h>
#include <pgtrack.h>
#include <netlink.h>
#include <pgsnap.h>
#include <ptealtprot.h>
#include <netlink/pgtrack/setup.h>
#include <testing/testing.h>
#include <hooks/activekps.h>

MODULE_AUTHOR("Stefano Belli");
MODULE_DESCRIPTION("Stealth code injection detector");
MODULE_LICENSE("GPL");

int setup_module(void);
void teardown_module(void);

#define BUILD "build: "

#define TYPE "type - "
#define STRICTNESS_ON "strictness[on] - "
#define STRICTNESS_OFF "strictness[off] - "
#define FEATURE_ON "feature[on] - "
#define FEATURE_OFF "feature[off] - "
#define WARNING_ON "warning[on] - "
#define WARNING_OFF "warning[off] - "
#define PARAM "parameter - "

static inline void print_build_specific_infos(void)
{

#ifdef SCID_CONFIG_TESTING
	scid_info(BUILD TYPE "testing");
#else
	scid_info(BUILD TYPE "release");
#endif

#ifdef PAP_INIT_LOCK_ORDER_STRICT
	scid_info(BUILD STRICTNESS_ON "init-lock-order");
#else
	scid_info(BUILD STRICTNESS_OFF "init-lock-order");
#endif

#ifdef PAP_IN_CPR_MMSLK_DONT_TRYLOCK
	scid_info(BUILD STRICTNESS_ON "no-cpr-trylock");
#else
	scid_info(BUILD STRICTNESS_OFF "no-cpr-trylock");
#endif

#ifdef DISABLE_PTE_ALT_PROT
	scid_info(BUILD FEATURE_OFF "ptealtprot");
#else
	scid_info(BUILD FEATURE_ON "ptealtprot");
#endif

#ifdef DISABLE_PAGE_SNAPSHOT
	scid_info(BUILD FEATURE_OFF "snapshot");
#else
	scid_info(BUILD FEATURE_ON "snapshot");
#endif

#ifdef KP_SHOW_NMISSED
	scid_info(BUILD WARNING_ON "show-kp-nmissed");
#else
	scid_info(BUILD WARNING_OFF "show-kp-nmissed");
#endif

	scid_infof(BUILD PARAM "KPS_MAXACTIVE = %d", KPS_MAXACTIVE);
}

int __init setup_module(void) 
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

	rv = setup_ptealtprot();
	if(rv) {
		scid_errf("setup_ptealtprot failed with rv=%d", rv);
		goto __teardown_from_testing;
	}

	/* circular dependency: 
	 *  - netlink subsystem depends on correct, setupped state, of 
	 * both page_snap and pgtrack to correctly allow kernel <-> user
	 * communication.
	 *
	 *  - page tracking subsystem depends on netlink to be able to
	 *  broadcast to user (same reason, basically)
	 *
	 * however, since hooks are setupped at the end, the only
	 * external input, up to that exact moment, that can possibly
	 * arrive, is from the userspace, so it is better to setup
	 * page tracking FIRST, then netlink.
	 */

	rv = setup_page_snap();
	if(rv) {
		scid_errf("setup_page_snap failed with rv=%d", rv);
		goto __teardown_from_ptealtprot;
	}

	rv = setup_pgtrack();
	if(rv) {
		scid_errf("setup_pgtrack failed with rv=%d", rv);
		goto __teardown_from_page_snap;
	}

	rv = setup_pgtrack_netlink();
	if(rv) {
		scid_errf("setup_pgtrack_netlink failed with rv=%d", rv);
		goto __teardown_from_pgtrack;
	}

	rv = setup_netlink();
	if(rv) {
		scid_errf("setup_netlink failed with rv=%d", rv);
		goto __teardown_from_pgtrack_netlink;
	}

	rv = setup_kcps_pcp_lists();
	if(rv) {
		scid_errf("setup_kcps_pcp_lists failed with rv=%d", rv);
		goto __teardown_from_netlink;
	}

	rv = setup_hooks();
	if (rv) {
		scid_errf("setup_hooks failed with rv=%d", rv);
		goto __teardown_from_kcps_pcp_lists;
	}

	print_build_specific_infos();

	return rv;

__teardown_from_kcps_pcp_lists:
	teardown_kcps_pcp_lists();
__teardown_from_netlink:
	teardown_netlink();
__teardown_from_pgtrack_netlink:
	teardown_pgtrack_netlink();
__teardown_from_pgtrack:
	teardown_pgtrack();
__teardown_from_page_snap:
	teardown_page_snap();
__teardown_from_ptealtprot:
	teardown_ptealtprot();
__teardown_from_testing:
	teardown_testing();

	return rv;
}

void __exit teardown_module(void) 
{
	teardown_hooks();
	teardown_kcps_pcp_lists();
	teardown_netlink();
	teardown_pgtrack_netlink();
	teardown_pgtrack();
	teardown_page_snap();
	teardown_ptealtprot();
	teardown_testing();
}

module_init(setup_module);
module_exit(teardown_module);
