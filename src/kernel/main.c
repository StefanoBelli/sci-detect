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

	setup_vmfs_pcp_lists();

	if ((rv = setup_hooks())) {
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
