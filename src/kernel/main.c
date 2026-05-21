#include <hooks/hooks.h>
#include <vmfs.h>
#include <logging.h>

MODULE_AUTHOR("Stefano Belli");
MODULE_DESCRIPTION("Stealth code injection detector");
MODULE_LICENSE("GPL");

int setup_module(void);
void teardown_module(void);

int setup_module(void) 
{
	int rv;

	rv = setup_vmfs_pcp_lists();
	if (rv) {
		scid_errf("setup_vmfs_pcp_lists failed with rv=%d", rv);
		return -ESRCH;
	}

	rv = setup_hooks();
	if (rv) {
		teardown_vmfs_pcp_lists();
		scid_errf("setup_hooks failed with rv=%d", rv);
		return -ESRCH;
	}

	return 0;
}

void teardown_module(void) 
{
	teardown_hooks();
	teardown_vmfs_pcp_lists();
}

module_init(setup_module);
module_exit(teardown_module);
