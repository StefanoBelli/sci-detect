#include <hooks.h>
#include <hooks/setuputils.h>
#include <logging.h>

extern SETUP_HOOKSGROUP_SIGNATURE(pte_page_track);
extern TEARDOWN_HOOKSGROUP_SIGNATURE(pte_page_track);

struct hooks_group_desc {
	const char *name;
	const char *brief;
	int (*setup)(void);
	void (*teardown)(void);
};

static struct hooks_group_desc hooksgroups[] = {
	{
		.name = "pte-page-track",
		.brief = "hooks that track page frames by looking at PTE level",
		.setup = SETUP_SYM(pte_page_track),
		.teardown = TEARDOWN_SYM(pte_page_track),
	},
};

#define NR_HOOKSGROUPS \
	(sizeof(hooksgroups) / sizeof(struct hooks_group_desc))

/* @nr_hooks: 0-based indexing 
 *
 * do teardown of the first [0, nr_hooks) hookgroups.
 *
 * E.g. [0,0) => no hooksgroup gets teared down
 *      [0,1) => hooksgroup indexed 0 gets teared down
 *      [0,2) => hooksgroup indexed 0 and 1 gets teared down
 *      [0,3) => hooksgroup indexed 0,1,2 gets teared down
 *      and so on...
 */
static void __teardown_first(unsigned long nr_hooks)
{
	for(unsigned long i = 0; i < nr_hooks; i++) {
		scid_infof("----- [teardown] hooks group: %s -----", hooksgroups[i].name);
		hooksgroups[i].teardown();
	}
}

/* Attempt to setup all hookgroups. 
 *
 * If one doesnt't go well, then teardown all prior hookgroups 
 * that were setup successfully. 
 *
 * E.g. if hookgroup indexed 1's setup
 * didn't go well, then teardown hookgroup indexed 0 which was setup
 * successfully before.
 *
 * Returns: 0 if all hookgroups were setup ok, not 0 otherwise
 */
int setup_hooks(void) 
{
	for(unsigned long i = 0; i < NR_HOOKSGROUPS; i++) {
		scid_infof("----- [setup] hooks group: %s -----", hooksgroups[i].name);

		int rv = hooksgroups[i].setup();
		if(rv) {
			scid_errf("----- unable to [setup] hooks group: %s -----", hooksgroups[i].name);
			__teardown_first(i);

			return rv;
		}
	}

	return 0;
}

/* teardown all hookgroups */
void teardown_hooks(void)
{
	__teardown_first(NR_HOOKSGROUPS);
}
