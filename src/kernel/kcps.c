#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/kprobes.h>
#include <linux/compiler.h>
#include <linux/errno.h>

#include <logging.h>
#include <kcps.h>

/*
 * NOTE: that if kretprobe's entry_handler is called, then
 * the handler of that kretprobe is ALWAYS called (unless
 * kernel oops/bugs).
 *
 * That is, if handle_pte_fault's entry_handler is called and
 * add_vmf (see below) is called, then the matching handler
 * that calls del_vmf (see below) is also called, so all memory
 * gets freed.
 *
 * Signals delivered to a thread are processed on the return
 * path to userspace. In any case, in the midst of handle_pte_fault,
 * if a termination signal is sent to the user-created thread,
 * the function is able to reach the end, so to call its kretprobe's
 * handler.
 *
 * NOTE2: why the migration mechanism is needed?
 *  what if a thread gets suspended while in #PF handler
 *  and gets scheduled on another CPU? the process running
 *  in the new CPU doesn't find the corresponding vm_fault
 *  when proceeding with the code (and encountering the hooks)
 *
 *  If this happens, try to look in remote CPUs and, if vm_fault
 *  found, migrate it to your local list.
 *
 *  should happen rarely though...
 *
 *  Or, what happens if a CPU gets, in some way "shut down" (cpuhp)?
 *
 * NOTE3: the rwlock is needed to let SMP systems' CPUs to
 * access remote lists of each other, to do the migration
 *
 */

#define HT_BITS 5

struct kcps_list {
	DECLARE_HASHTABLE(ht, HT_BITS);

#ifdef CONFIG_SMP
	rwlock_t lock;
#endif

};

#ifdef CONFIG_SMP
static DEFINE_PER_CPU(struct kcps_list, kcps);
#else
static struct kcps_list kcps;
#endif 

static struct kmem_cache *kcp_entry_cachep;

int setup_kcps_pcp_lists(void) 
{
	kcp_entry_cachep = kmem_cache_create(
			"scid__kcp_entry_cache", 
			sizeof(struct kcp_entry), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);

	if(!kcp_entry_cachep) {
		scid_err("unable to create a new kmem_cache for kcp_entry");
		return -ENOMEM;
	}

#ifdef CONFIG_SMP
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct kcps_list *l = per_cpu_ptr(&kcps, cpu);

		hash_init(l->ht);
		rwlock_init(&l->lock);
	}
#else
	hash_init(kcps.ht);
#endif

	return 0;
}

static inline void __ht_destroy_all(struct kcps_list *l) 
{
	struct kcp_entry *entry;
	struct hlist_node *tmp;
	unsigned int bucket_idx;

	hash_for_each_safe(l->ht, bucket_idx, tmp, entry, node) {
		hlist_del(&entry->node);
		kmem_cache_free(kcp_entry_cachep, entry);
	}
}

/* this must be called *AFTER* unregister_k(ret)probes did its job */
void teardown_kcps_pcp_lists(void) 
{

#ifdef CONFIG_SMP
	unsigned int cpu;

	for_each_possible_cpu(cpu)
		__ht_destroy_all(per_cpu_ptr(&kcps, cpu));
#else
	__ht_destroy_all(&kcps);
#endif

	kmem_cache_destroy(kcp_entry_cachep);
}

#define __my_hash_for_each_possible(name, obj, member, key) \
	hlist_for_each_entry(obj, &name[hash_long(key, HASH_BITS(name))], member)

/* just tell me if vmf is in the per-CPU list */
static struct kcp_entry *__lookup_in_pcp_list(
		struct kcps_list *list, u64 key, kcp_compare_fpt compare) 
{
	struct kcp_entry *entry;

#ifdef CONFIG_SMP
	unsigned long cpu_flags;
	read_lock_irqsave(&list->lock, cpu_flags);
#endif

	__my_hash_for_each_possible(list->ht, entry, node, key) {
		if(compare(entry, key)) {

#ifdef CONFIG_SMP
			read_unlock_irqrestore(&list->lock, cpu_flags);
#endif

			return entry;
		}
	}

#ifdef CONFIG_SMP
	read_unlock_irqrestore(&list->lock, cpu_flags);
#endif

	return NULL;
}

#undef __my_hash_for_each_possible

#define __my_hash_add(hashtable, node, key) \
	hlist_add_head(node, &hashtable[hash_long(key, HASH_BITS(hashtable))])

static inline void __add_entry_to_pcp_list(
		struct kcps_list *list, struct kcp_entry *kcpe, u64 key)
{

#ifdef CONFIG_SMP
	kcpe->list_lock = &list->lock;

	write_lock(&list->lock);
#endif

	__my_hash_add(list->ht, &kcpe->node, key);

#ifdef CONFIG_SMP
	write_unlock(&list->lock);
#endif

}

#undef __my_hash_add

static inline void __del_entry_from_pcp_list(struct kcp_entry *kcpe)
{

#ifdef CONFIG_SMP
	write_lock(kcpe->list_lock);
#endif

	hlist_del(&kcpe->node);

#ifdef CONFIG_SMP
	write_unlock(kcpe->list_lock);
#endif

}

/* preemption disabled by caller */
struct kcp_entry* got_this_kcp(u64 key, kcp_compare_fpt compare) 
{
	struct kcp_entry *found_entry;
	struct kcps_list *my_list;

#ifdef CONFIG_SMP
	unsigned int cpu;
	unsigned int my_cpu;

	my_list = this_cpu_ptr(&kcps);
#else
	my_list = &kcps;
#endif

	/* we are ok acquiring percpu-HT locks this way,
	 * preemption being disabled is a very important assumption here:
	 *
	 * if kcpe is not found in this cpu's HT, the kcpe we are looking
	 * for must be in another cpu HT.
	 *
	 * Now, why we cannot find kcpe? As explained above, in the page
	 * fault handler, the process may need to go to sleep, what if
	 * it gets rescheduled on another CPU? The kcpe we will look for
	 * is on another CPU's pcp HT.
	 *
	 * Since we are running a thread in kernel mode and with preemption
	 * disabled, and we (that is, current) are the only owner of that 
	 * kcpe object, it will **never** be migrated to another pcp HT.
	 * 
	 * THAT IS, WE DON'T RISK TO NEVER FIND THE kcpe object, even
	 * when on some pcp HT.
	 *
	 *  - "fast path": acquire the read lock on pcp-HT, look for the
	 *  kcpe object, found it, release the read lock.
	 *
	 *  - "slow path": ignore my own (this) cpu and start to do the 
	 *  things mentioned in the fast path for each cpu (cpumasked)
	 *  when kcpe is found, migrate it on my own pcp HT. (2 write
	 *  locks will be acquired and released to do the migration).
	 *
	 * The migration in the "slow path" is only done by the current 
	 * thread who is LOOKING for that kcpe object, never done 
	 * "proactively".
	 */

	found_entry = __lookup_in_pcp_list(my_list, key, compare);
	if(found_entry)
		return found_entry;

#ifdef CONFIG_SMP
	my_cpu = smp_processor_id();

	for_each_possible_cpu(cpu) {
		struct kcps_list *pcp_list;

		if(cpu == my_cpu)
			continue;

		pcp_list = per_cpu_ptr(&kcps, cpu);
		found_entry = __lookup_in_pcp_list(pcp_list, key, compare);
		if(!found_entry)
			continue;

		/* we need to migrate... */
		__del_entry_from_pcp_list(found_entry);
		__add_entry_to_pcp_list(my_list, found_entry, key);

		return found_entry;
	}
#endif

	return NULL;
}

/* preemption is disabled by caller */
struct kcp_entry *add_kcp(u64 key, void* data) 
{
	struct kcp_entry *entry;
	struct kcps_list *my_list;

	if((entry = kmem_cache_alloc(kcp_entry_cachep, GFP_ATOMIC)) == NULL) {
		scid_err("memory exhausted");
		return NULL;
	}

	entry->data = data;

#ifdef CONFIG_SMP
	entry->list_lock = NULL;

	my_list = this_cpu_ptr(&kcps);
#else
	my_list = &kcps;
#endif

	__add_entry_to_pcp_list(my_list, entry, key);

	return entry;
}

/*
 * doesn't really matter if deleting from my
 * own local cpu list, or another remote cpu.
 *
 * Taking the write lock and deleting the entry
 * forever, won't be looked at any further,
 * from now on.
 */
void del_kcp_bykey(u64 key, kcp_compare_fpt compare) 
{
	struct kcp_entry *kcpe = got_this_kcp(key, compare);
	if(unlikely(!kcpe))
		return;

	__del_entry_from_pcp_list(kcpe);

	kmem_cache_free(kcp_entry_cachep, kcpe);
}
