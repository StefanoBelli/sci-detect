#include <testing/testing.h>

#ifdef SCID_CONFIG_TESTING

#include <linux/list.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/preempt.h>

#ifdef CONFIG_SYSFS
#include <asm/barrier.h>
#include <asm-generic/rwonce.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/kstrtox.h>
#include <linux/compiler.h>
#endif

#include <logging.h>

struct subsys_kvpair {
	/* key */
	const char *key;

	/* value */
	void *value;
	unsigned long value_size;

	/* kvops */
	const struct kv_ops *kv_ops;

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

	/* this acts more as a refcount, protects both the 
	 * testing instance descriptor and the kvpairs */
	struct rcu_head rcu;
};

struct subsys_under_test {
	/* name of the subsys */
	const char* name;

	/* kvt array, "replicated" on each instance */
	const struct subsys_kv_template* kvt;

#ifdef CONFIG_SYSFS
	/* the kobject */
	struct kobject kobj;
#endif

	/* list for each registered subsys */
	struct list_head other_subsyses; 

	/* this lock protects the testing_inst list */
	spinlock_t tilock; 
	struct list_head testing_inst;
};

static struct list_head sut_head;

/* WARNING: this does not eliminate @sti from the containing list */
static void __subsys_testing_instance_free(struct rcu_head *rcuh)
{
	struct subsys_testing_instance *sti =
		container_of(rcuh, struct subsys_testing_instance, rcu);

	struct subsys_kvpair *kvp;
	struct subsys_kvpair *tmp;

	if(!sti)
		return;

	list_for_each_entry_safe(kvp, tmp, &sti->kv_pairs, kvp_head) {
		if(kvp->value)
			kfree(kvp->value);

		if(kvp)
			kfree(kvp);

		list_del(&kvp->kvp_head);
	}

	kfree(sti);
}

/* helpers to avoid deadlocks */

struct sti_eliminate_lock_control {
	bool lock_before;
	bool unlock_after;
};

#define __selc_init(lb, ua) \
	(struct sti_eliminate_lock_control) { \
		.lock_before = (lb), \
		.unlock_after = (ua) \
	}

#define LOCK_AND_UNLOCK __selc_init(true, true)
#define ONLY_UNLOCK __selc_init(false, true)
#define NO_LOCKING_AT_ALL __selc_init(false, false)

static void __subsys_testing_instance_eliminate(
		struct subsys_testing_instance *sti, 
		struct subsys_under_test *sut,
		struct sti_eliminate_lock_control lock_control)
{
	if(lock_control.lock_before)
		spin_lock(&sut->tilock);

	list_del_rcu(&sti->other_testing_insts);

	if(lock_control.unlock_after)
		spin_unlock(&sut->tilock);

	call_rcu(&sti->rcu, __subsys_testing_instance_free);
}

/* YOU either get the tilock or define a rcu critical section around this fn call  */
static struct subsys_testing_instance *__subsys_testing_instance_search(
		pid_t vpid, struct list_head *lh)
{
	struct subsys_testing_instance *iter_sti;

	/* rcu critical section not needed here */
	list_for_each_entry_rcu(iter_sti, lh, other_testing_insts) {
		if(iter_sti->vpid == vpid)
			return iter_sti;
	}

	return NULL;
}

static struct subsys_testing_instance* __do_enable_subsys(
		struct subsys_under_test *my_sut, pid_t vpid)
{
	spin_lock(&my_sut->tilock);

	if(__subsys_testing_instance_search(vpid, &my_sut->testing_inst)) {
		spin_unlock(&my_sut->tilock);
		return NULL;
	}

	struct subsys_testing_instance *new_sti =
		(struct subsys_testing_instance *) 
			kmalloc(sizeof(struct subsys_testing_instance), GFP_KERNEL);
	if(!new_sti)
		goto __unlock_cleanup0;

	new_sti->vpid = vpid;
	INIT_LIST_HEAD(&new_sti->kv_pairs);

	for(unsigned long i = 0; i < MAX_KVS; i++) {
		const struct subsys_kv_template *my_kvt = &my_sut->kvt[i];

		if(my_kvt->key == NULL)
			break;

		struct subsys_kvpair *kvp = (struct subsys_kvpair*)
			kmalloc(sizeof(struct subsys_kvpair), GFP_KERNEL);
		if(!kvp)
			goto __unlock_cleanup0;

		kvp->value = kzalloc(my_kvt->value_size, GFP_KERNEL);
		if(!kvp->value)
			goto __unlock_cleanup0;

		kvp->key = my_kvt->key;
		kvp->started = false;
		kvp->value_size = my_kvt->value_size;
		kvp->kv_ops = &my_kvt->kv_ops;

		if(kvp->kv_ops->init_value && 
				!kvp->kv_ops->init_value(kvp->value, kvp->value_size))
			goto __unlock_cleanup0;

		list_add(&kvp->kvp_head, &new_sti->kv_pairs);
	}

	/* now visible to everyone */
	list_add_rcu(&new_sti->other_testing_insts, &my_sut->testing_inst);

	spin_unlock(&my_sut->tilock);
	return new_sti;

__unlock_cleanup0:
	spin_unlock(&my_sut->tilock);

	/* we don't need to hold the lock here */
	call_rcu(&new_sti->rcu, __subsys_testing_instance_free);

	return NULL;
}

#ifdef CONFIG_SYSFS

/* shall be used by every other kobject */
static const struct kobj_type default_kot = {
	/* don't need the ->release */
	.sysfs_ops = &kobj_sysfs_ops,
};

static struct kobject testing_mod_kobj;

#define __scidtest_kobject_init_and_add_fmt(kobj_target, kobj_parent, fmt, ...) \
	kobject_init_and_add((kobj_target), &default_kot, (kobj_parent), fmt, __VA_ARGS__)

#define __scidtest_kobject_init_and_add(kobj_target, kobj_parent, name) \
	__scidtest_kobject_init_and_add_fmt(kobj_target, kobj_parent, "%s", name)

#define __scidtest_kobject_del_and_put(kobj) \
	kobject_del((kobj)); \
	kobject_put((kobj))

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/query pseudofile impl */
static ssize_t query_key_show(
		struct kobject *kobj, 
		__always_unused struct kobj_attribute *kobj_attr, 
		char* s)
{

#define MAX_TMPBUF_SIZE 256

	char *tmpbuf = (char*) kzalloc(MAX_TMPBUF_SIZE, GFP_KERNEL);
	if(!tmpbuf)
		return -ENOMEM;

	rcu_read_lock();
	
	struct subsys_kvpair *kvp =
		container_of(kobj, struct subsys_kvpair, kobj);

	if(kvp->kv_ops->uquery_value)
		kvp->kv_ops->uquery_value(tmpbuf, MAX_TMPBUF_SIZE, kvp->value, kvp->value_size);

#undef MAX_TMPBUF_SIZE

	rcu_read_unlock();

	int rv = sysfs_emit(s, "%s\n", tmpbuf);

	kfree(tmpbuf);
	
	return rv;
}

static const struct kobj_attribute query_key_kobj_attr = 
	__ATTR(query, 0400, query_key_show, NULL);

static void __kvp_from_kobj_startstop(struct kobject *kobj, bool has_to_start)
{
	rcu_read_lock();

	struct subsys_kvpair *kvp =
		container_of(kobj, struct subsys_kvpair, kobj);

	/* this should be enough to impede the optimizing compiler to do
	 * whatever it wants with mem accesses, right?! */
	WRITE_ONCE(kvp->started, has_to_start);
	smp_mb();

	rcu_read_unlock();
}

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/start pseudofile impl */
static ssize_t start_store(
		struct kobject *kobj, 
		__always_unused struct kobj_attribute *kobj_attr, 
		__always_unused const char* s, 
		size_t len)
{
	__kvp_from_kobj_startstop(kobj, true);
	return len;
}

static const struct kobj_attribute start_kobj_attr = 
	__ATTR(start, 0200, NULL, start_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/stop pseudofile impl */
static ssize_t stop_store(
		struct kobject *kobj, 
		__always_unused struct kobj_attribute *kobj_attr, 
		__always_unused const char* s, 
		size_t len)
{
	__kvp_from_kobj_startstop(kobj, false);
	return len;
}

static const struct kobj_attribute stop_kobj_attr = 
	__ATTR(stop, 0200, NULL, stop_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/<pid>/<key>/reset pseudofile impl */
static ssize_t reset_store(
		struct kobject *kobj, 
		__always_unused struct kobj_attribute *kobj_attr, 
		__always_unused const char* s, 
		size_t len)
{
	rcu_read_lock();
	
	struct subsys_kvpair *kvp =
		container_of(kobj, struct subsys_kvpair, kobj);

	if(kvp->kv_ops->reset_value)
		kvp->kv_ops->reset_value(kvp->value, kvp->value_size);

	rcu_read_unlock();

	return len;
}

static const struct kobj_attribute reset_kobj_attr = 
	__ATTR(reset, 0200, NULL, reset_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/disable pseudofile impl */
static ssize_t disable_store(
		struct kobject *kobj, 
		__always_unused struct kobj_attribute *kobj_attr, 
		__always_unused const char* s, 
		size_t len)
{
	int rv;
	struct subsys_under_test *sut = container_of(kobj, struct subsys_under_test, kobj);
	pid_t pid_res;

	rv = kstrtoint(s, 10, &pid_res);
	if(rv)
		return rv;

	spin_lock(&sut->tilock);
	struct subsys_testing_instance *sti = 
		__subsys_testing_instance_search(pid_res, &sut->testing_inst);
	if(!sti) {
		spin_unlock(&sut->tilock);
		return -ESRCH;
	}

	__scidtest_kobject_del_and_put(&sti->kobj);
	__subsys_testing_instance_eliminate(sti, sut, ONLY_UNLOCK);

	return len;
}

static const struct kobj_attribute disable_kobj_attr = 
	__ATTR(disable, 0200, NULL, disable_store);

/* provides the /sys/module/sci_detect/testing/<subsys>/enable pseudofile impl */
static ssize_t enable_store(
		struct kobject *kobj, 
		__maybe_unused struct kobj_attribute *kobj_attr, 
		const char* s, 
		size_t len)
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

	/* the /<pid>/ subdir */
	if(__scidtest_kobject_init_and_add_fmt(&sti->kobj, kobj, "%d", pid_res))
		goto __cleanup1;

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
	__scidtest_kobject_del_and_put(&sti->kobj);
__cleanup1:
	__subsys_testing_instance_eliminate(sti, sut, LOCK_AND_UNLOCK);
	return -ENOENT;
}

static const struct kobj_attribute enable_kobj_attr = 
	__ATTR(enable, 0200, NULL, enable_store);

#endif //CONFIG_SYSFS

static inline bool __testing_root_expose(void)
{

#ifdef CONFIG_SYSFS
	struct kobject *mod_kobj = &THIS_MODULE->mkobj.kobj;

	int rv = __scidtest_kobject_init_and_add(&testing_mod_kobj, mod_kobj, "testing");
	if(rv) {
		scid_errf("unable to create testing kobj :(, rv = %d", rv);
		return false;
	}

	return true;
#else
	return false;
#endif

}

static inline void __testing_root_hide(void)
{

#ifdef CONFIG_SYSFS
	__scidtest_kobject_del_and_put(&testing_mod_kobj);
#endif

}

static inline bool __registered_subsys_expose(struct subsys_under_test *sut)
{

#ifdef CONFIG_SYSFS
	if(__scidtest_kobject_init_and_add(
				&sut->kobj, &testing_mod_kobj, sut->name)) {

		scid_err("unable to create subsys kobj");
		return false;
	}

	if(sysfs_create_file(&sut->kobj, &enable_kobj_attr.attr) || 
			sysfs_create_file(&sut->kobj, &disable_kobj_attr.attr)) {

		__scidtest_kobject_del_and_put(&sut->kobj);
		scid_err("unable to create enable/disable file for kobj");
		return false;
	}

	return true;
#else
	return false;
#endif

}

static inline void __registered_subsys_hide(struct subsys_under_test *sut)
{

#ifdef CONFIG_SYSFS
	__scidtest_kobject_del_and_put(&sut->kobj);
#endif

}

static inline void __enabled_stinstance_hide(struct subsys_testing_instance *sti)
{

#ifdef CONFIG_SYSFS
	__scidtest_kobject_del_and_put(&sti->kobj);
#endif

}

#endif //SCID_CONFIG_TESTING

/* extern linkage,
 *
 * if SCID_CONFIG_TESTING is disabled, 
 * these should do nothing and eventually 
 * return positive results 
 */

bool testing_register_subsys(__maybe_unused const struct subsys_regi_args *args)
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
	sut->kvt = args->kvt;
	spin_lock_init(&sut->tilock);
	INIT_LIST_HEAD(&sut->testing_inst);

	if(!__registered_subsys_expose(sut)) {
		kfree(sut);
		return false;
	}

	/* warning: no lock needed for the main "subsys" list */
	list_add(&sut->other_subsyses, &sut_head);

	return true;
#else
	return true;
#endif

}

int setup_testing(void)
{

#ifdef SCID_CONFIG_TESTING
	INIT_LIST_HEAD(&sut_head);

	if(!__testing_root_expose())
		return -1;

	return 0;
#else
	return 0;
#endif

}

void teardown_testing(void)
{

#ifdef SCID_CONFIG_TESTING
	struct subsys_under_test *pos;
	struct subsys_under_test *tmp;

	list_for_each_entry_safe(pos, tmp, &sut_head, other_subsyses) {
		scid_infof("unregistering testing subsystem %s", pos->name);

		struct subsys_testing_instance *sti;
		struct subsys_testing_instance *sti_tmp;

		spin_lock(&pos->tilock);
		list_for_each_entry_safe(sti, sti_tmp, &pos->testing_inst, other_testing_insts) {
			scid_infof(" --> eliminating testing instance for pid=%d", sti->vpid);

			__enabled_stinstance_hide(sti);
			__subsys_testing_instance_eliminate(sti, pos, NO_LOCKING_AT_ALL);
		}
		spin_unlock(&pos->tilock);

		__registered_subsys_hide(pos);
		list_del(&pos->other_subsyses);
		kfree(pos);
	}

	__testing_root_hide();
#endif

}

#ifdef SCID_CONFIG_TESTING
int __do_testing_setval(
		const char *subsys_name, 
		const char *key, 
		void *args)
{
	if(!in_task())
		return TESTING_SETVAL_NOTASK;

	struct subsys_under_test *sut = NULL;

	list_for_each_entry(sut, &sut_head, other_subsyses) {
		if(!strcmp(sut->name, subsys_name))
			break;
	}

	if(!sut)
		return TESTING_SETVAL_NOSUT;

	pid_t vpid_thr = task_pid_vnr(current);

	rcu_read_lock();

	struct subsys_testing_instance *sti = 
		__subsys_testing_instance_search(vpid_thr, &sut->testing_inst);

	if(!sti) {
		rcu_read_unlock();
		return TESTING_SETVAL_NOINST;
	}

	struct subsys_kvpair *kvp = NULL;

	list_for_each_entry(kvp, &sti->kv_pairs, kvp_head) {
		if(!strcmp(key, kvp->key))
			break;
	}

	if(!kvp) {
		rcu_read_unlock();
		return TESTING_SETVAL_NOKEY;
	}

	bool has_started = READ_ONCE(kvp->started);
	smp_mb();

	if(!has_started) {
		rcu_read_unlock();
		return TESTING_SETVAL_NOTSTARTED;
	}

	int final_rv = 0;

	if(kvp->kv_ops->set_value)
		kvp->kv_ops->set_value(kvp->value, args, kvp->value_size);
	else
		final_rv = TESTING_SETVAL_NOKVOP;

	rcu_read_unlock();
	return final_rv;
}
#endif
