#include <testing.h>

#ifdef SCID_CONFIG_TESTING

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>

#ifdef CONFIG_SYSFS
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/kstrtox.h>
#endif

#include <logging.h>

struct subsys_kvpair {
	/* key */
	const char *key;

	/* value */
	void *value;
	unsigned long value_size;

	/* kvops */
	struct kv_ops *kv_ops;

	/* whether I should collect data or not? */
	bool started;

#ifdef CONFIG_SYSFS
	/* the kobject */
	struct kobject kobj;
#endif

	/* other kv-pairs for subsys */
	struct list_head kvp_head;
};

struct subsys_testing_instance {
	/* pid-based instance, pid reuse warning! */
	pid_t vpid;

#ifdef CONFIG_SYSFS
	/* the kobject */
	struct kobject kobj;
#endif

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

#ifdef CONFIG_SYSFS
	/* the kobject */
	struct kobject kobj;
#endif

	/* list for each registered subsys */
	struct list_head other_subsyses; 

	/* list for each enabled testing inst */
	spinlock_t tilock; 
	struct list_head testing_inst;
};

static struct list_head sut_head;

/* TODO may be renamed/reused ??? */
static void __do_enable_failed_cleanup(struct subsys_testing_instance *sti)
{
	struct subsys_kvpair *kvp;
	struct subsys_kvpair *tmp;

	list_for_each_entry_safe(kvp, tmp, &sti->kv_pairs, kvp_head) {
		if(kvp->value)
			kfree(kvp->value);

		if(kvp)
			kfree(kvp);

		list_del(&kvp->kvp_head);
	}

	kfree(sti);
}

static struct subsys_testing_instance* __do_enable_subsys(struct subsys_under_test *my_sut, pid_t vpid)
{
	struct subsys_testing_instance *sti;

	spin_lock(&my_sut->tilock);
	list_for_each_entry(sti, &my_sut->testing_inst, other_testing_insts) {
		if(sti->vpid == vpid) {
			spin_unlock(&my_sut->tilock);
			return NULL;
		}
	}
	spin_unlock(&my_sut->tilock);

	struct subsys_testing_instance *new_sti =
		(struct subsys_testing_instance *)
			kmalloc(sizeof(struct subsys_testing_instance), GFP_KERNEL);

	if(!new_sti)
		return NULL;

	new_sti->vpid = vpid;
	INIT_LIST_HEAD(&new_sti->kv_pairs);
	for(unsigned long i = 0; i < my_sut->kvt_len; i++) {
		struct subsys_kv_template *my_kvt = &my_sut->kvt[i];

		struct subsys_kvpair *kvp = (struct subsys_kvpair*)
			kmalloc(sizeof(struct subsys_kvpair), GFP_KERNEL);
		if(!kvp)
			goto __cleanup0;

		kvp->value = kmalloc(my_kvt->value_size, GFP_KERNEL);
		if(!kvp->value)
			goto __cleanup0;

		kvp->key = my_kvt->key;
		kvp->started = false;
		kvp->value_size = my_kvt->value_size;
		kvp->kv_ops = &my_kvt->kv_ops;

		list_add(&kvp->kvp_head, &new_sti->kv_pairs);
	}

	spin_lock(&my_sut->tilock);
	list_add(&new_sti->other_testing_insts, &my_sut->testing_inst);
	spin_unlock(&my_sut->tilock);

	return new_sti;

__cleanup0:
	__do_enable_failed_cleanup(new_sti);
	return NULL;
}

#ifdef CONFIG_SYSFS

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

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/disable pseudofile impl */
static ssize_t disable_store(struct kobject *kobj, struct kobj_attribute *kobj_attr, const char* s, size_t len)
{
	return 0;
}

static const struct kobj_attribute disable_kobj_attr =
	__ATTR(disable, 0200, NULL, disable_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/enable pseudofile impl */
static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *kobj_attr, const char* s, size_t len)
{
	int rv;
	struct subsys_under_test *sut = container_of(kobj, struct subsys_under_test, kobj);
	pid_t pid_res;

	rv = kstrtoint(s, 10, &pid_res);
	if(rv)
		return rv;

	struct subsys_testing_instance *sti = __do_enable_subsys(sut, pid_res);
	if(!sti)
		return -EBUSY;

	if(__scidtest_kobject_init_and_add_fmt(&sti->kobj, kobj, "%d", pid_res))
		goto __cleanup1;

	/* the /<pid>/disable file */
	if(sysfs_create_file(&sti->kobj, &disable_kobj_attr.attr))
		goto __kobj_del_cleanup1;

	struct subsys_kvpair *pos;

	/* for each key */
	list_for_each_entry(pos, &sti->kv_pairs, kvp_head) {
		/* the /<pid>/<key>/ subdir */
		if(__scidtest_kobject_init_and_add(&pos->kobj, &sti->kobj, pos->key))
			goto __kobj_del_cleanup1;

		/* the /<pid>/<key>/query file */
		if(sysfs_create_file(&pos->kobj, &query_key_kobj_attr.attr))
			goto __kobj_del_cleanup1;

		/* the /<pid>/<key>/start file */
		if(sysfs_create_file(&pos->kobj, &start_kobj_attr.attr))
			goto __kobj_del_cleanup1;

		/* the /<pid>/<key>/stop file */
		if(sysfs_create_file(&pos->kobj, &stop_kobj_attr.attr))
			goto __kobj_del_cleanup1;

		/* the /<pid>/<key>/reset file */
		if(sysfs_create_file(&pos->kobj, &reset_kobj_attr.attr))
			goto __kobj_del_cleanup1;
	}

	return len;

__kobj_del_cleanup1:
	kobject_del(&sti->kobj);
__cleanup1:
	__do_enable_failed_cleanup(sti);
	return -ENOENT;
}

static const struct kobj_attribute enable_kobj_attr =
	__ATTR(enable, 0200, NULL, enable_store);

#endif //CONFIG_SYSFS
#endif //SCID_CONFIG_TESTING

int setup_testing(void)
{

#ifdef SCID_CONFIG_TESTING
	int rv;

	INIT_LIST_HEAD(&sut_head);

#ifdef CONFIG_SYSFS
	struct kobject mod_kobj = THIS_MODULE->mkobj.kobj;

	rv = __scidtest_kobject_init_and_add(&testing_mod_kobj, &mod_kobj, "testing");
	if(rv) {
		scid_errf("unable to create testing kobj :( ;;; rv=%d", rv);
		return rv;
	}
#endif 

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
	
	int rv;

	sut->name = args->name;
	sut->kvt = &args->kvt;
	sut->kvt_len = args->kvt_len;
	spin_lock_init(&sut->tilock);
	INIT_LIST_HEAD(&sut->testing_inst);

#ifdef CONFIG_SYSFS
	rv = __scidtest_kobject_init_and_add(
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
#endif

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
