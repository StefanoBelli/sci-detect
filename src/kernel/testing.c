#include <testing.h>

#ifdef SCID_CONFIG_TESTING

#include <linux/list.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>

#include <logging.h>

struct subsys_kvpair {
	/* key */
	const char *key;

	/* value */
	void *value;
	unsigned long value_size;

	/* whether I should collect data or not? */
	bool started;

	/* the kobject */
	struct kobject kobj;

	/* other kv-pairs for subsys */
	struct list_head kvp_head;
};

struct subsys_testing_instance {
	/* pid-based instance, pid reuse warning! */
	pid_t vpid;

	/* the kobject */
	struct kobject kobj;

	/* list for each testing inst for this subsys */
	struct list_head other_testing_insts;

	/* keyvalue pairs for this subsys */
	struct list_head kv_pairs;

	/* rcu */
	struct rcu_head rcu;
};

struct subsys_under_test {
	/* name of the subsys */
	const char* name;

	/* kvt array, "replicated" on each instance */
	struct subsys_kv_template* kvt;
	unsigned long kvt_len;

	/* the kobject */
	struct kobject kobj;

	/* list for each registered subsys */
	struct list_head other_subsyses; 

	/* list for each enabled testing inst */
	spinlock_t tilock; 
	struct list_head testing_inst;
};

static struct list_head sut_head;

/* provides the /sys/module/sci_detect/testing/<subsys>/enable pseudofile impl */
static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *kobj_attr, const char* s, size_t len)
{
	return 0;
}

static const struct kobj_attribute enable_kobj_attr =
	__ATTR(enable, 0200, NULL, enable_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/disable pseudofile impl */
static ssize_t disable_store(struct kobject *kobj, struct kobj_attribute *kobj_attr, const char* s, size_t len)
{
	return 0;
}

static const struct kobj_attribute disable_kobj_attr =
	__ATTR(disable, 0200, NULL, disable_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/query pseudofile impl */
static ssize_t query_key_show(struct kobject *kobj, struct kobj_attribute *kobj_attr, char* s)
{
	return 0;
}

static const struct kobj_attribute query_key_kobj_attr =
	__ATTR(query, 0400, query_key_show, NULL);

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/start pseudofile impl */
static ssize_t start_store(struct kobject *kobj, struct kobj_attribute *kobj_attr, const char* s, size_t len)
{
	return 0;
}

static const struct kobj_attribute start_kobj_attr =
	__ATTR(start, 0200, NULL, start_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/stop pseudofile impl */
static ssize_t stop_store(struct kobject *kobj, struct kobj_attribute *kobj_attr, const char* s, size_t len)
{
	return 0;
}

static const struct kobj_attribute stop_kobj_attr =
	__ATTR(stop, 0200, NULL, stop_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/reset pseudofile impl */
static ssize_t reset_store(struct kobject *kobj, struct kobj_attribute *kobj_attr, const char* s, size_t len)
{
	return 0;
}

static const struct kobj_attribute reset_kobj_attr =
	__ATTR(reset, 0200, NULL, reset_store);

/* shall be used by every other kobject */
static const struct kobj_type default_kot = {
	/* don't need the ->release */
	.sysfs_ops = &kobj_sysfs_ops,
};

static struct kobject testing_mod_kobj;

#	define __scidtest_kobject_init_and_add_fmt(kobj_target, kobj_parent, fmt, ...) \
		({ \
			kobject_init((kobj_target), &default_kot); \
			kobject_add((kobj_target), (kobj_parent), fmt, __VA_ARGS__); \
		})

#	define __scidtest_kobject_init_and_add(kobj_target, kobj_parent, name) \
		__scidtest_kobject_init_and_add_fmt(kobj_target, kobj_parent, "%s", name)

#endif

int setup_testing(void)
{

#ifdef SCID_CONFIG_TESTING
	int rv;

	INIT_LIST_HEAD(&sut_head);
	struct kobject mod_kobj = THIS_MODULE->mkobj.kobj;

	rv = __scidtest_kobject_init_and_add(&testing_mod_kobj, &mod_kobj, "testing");
	if(rv) {
		scid_errf("unable to create testing kobj :( ;;; rv=%d", rv);
		return rv;
	}

	return 0;
#else
	return 0;
#endif

}

bool testing_register_subsys(struct subsys_regi_args *args)
{

#ifdef SCID_CONFIG_TESTING
	struct subsys_under_test *sut;

	sut = (struct subsys_under_test*) 
		kmalloc(sizeof(struct subsys_under_test), GFP_KERNEL);
	if(!sut) {
		scid_err("memory exhausted");
		return false;
	}

	sut->name = args->name;
	sut->kvt = &args->kvt;
	sut->kvt_len = args->kvt_len;
	spin_lock_init(&sut->tilock);
	INIT_LIST_HEAD(&sut->testing_inst);
	int rv = 
		__scidtest_kobject_init_and_add(
				&sut->kobj, &testing_mod_kobj, sut->name);
	if(rv) {
		kfree(sut);
		scid_err("unable to create subsys kobj");
		return false;
	}

	rv = sysfs_create_file(&sut->kobj, &enable_kobj_attr.attr);
	if(rv) {
		kobject_del(&sut->kobj);
		kfree(sut);
		scid_err("unable to create enable file for kobj");
		return false;
	}

	/* warning: no lock needed for the main "subsys" list */
	list_add(&sut->other_subsyses, &sut_head);

	return true;
#else
	return true;
#endif

}

void teardown_testing(void)
{

#ifdef SCID_CONFIG_TESTING
	struct subsys_under_test *pos;
	struct subsys_under_test *tmp;

	list_for_each_entry_safe(pos, tmp, &sut_head, other_subsyses) {
		list_del(&pos->other_subsyses);
		kfree(pos);
	}

#endif

}

bool testing_setval(const char *subsys_name, const char *key, void *args)
{

#ifdef SCID_CONFIG_TESTING
	return true;
#else
	return true;
#endif

}
