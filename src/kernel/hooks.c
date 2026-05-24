#include <hooks.h>
#include <logging.h>

extern int __setup_add_hooks(void);
extern int __setup_chg_hooks(void);
extern int __setup_del_hooks(void);

extern void __teardown_add_hooks(void);
extern void __teardown_chg_hooks(void);
extern void __teardown_del_hooks(void);

struct hooks_group_desc {
	const char *name;
	const char *brief;
	int (*setup)(void);
	void (*teardown)(void);
};

static struct hooks_group_desc hooksgroups[] = {
	{
		.name = "add",
		.brief = "hooks that cause the start of page-state tracking",
		.setup = __setup_add_hooks,
		.teardown = __teardown_add_hooks,
	},
	
	{
		.name = "chg",
		.brief = "hooks that cause the change of state for the tracked page",
		.setup = __setup_chg_hooks,
		.teardown = __teardown_chg_hooks,
	},

	{
		.name = "del",
		.brief = "hooks that cause the untracking of page-state",
		.setup = __setup_del_hooks,
		.teardown = __teardown_del_hooks,
	},
};

#define NR_HOOKSGROUPS \
	(sizeof(hooksgroups) / sizeof(struct hooks_group_desc))

static void __teardown_first(unsigned long nr_hooks)
{
	for(unsigned long i = 0; i < nr_hooks; i++) {
		scid_infof("teardown hooks group: %s", hooksgroups[i].name);
		hooksgroups[i].teardown();
	}
}

int setup_hooks(void) 
{
	for(unsigned long i = 0; i < NR_HOOKSGROUPS; i++) {
		scid_infof("setting up hooks group: %s", hooksgroups[i].name);

		int setup_rv = hooksgroups[i].setup();
		if(setup_rv) {
			scid_errf("unable to setup hooks group %s", hooksgroups[i].name);
			__teardown_first(i);

			return setup_rv;
		}
	}

	return 0;
}

void teardown_hooks(void)
{
	__teardown_first(NR_HOOKSGROUPS);
}
