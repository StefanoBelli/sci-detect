#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/kprobes.h>
#include <linux/compiler.h>
#include <linux/errno.h>

#include <vmfs.h>
#include <logging.h>

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
 * NOTE3: the rwlock is needed to let SMP systems' CPUs to
 * access remote lists of each other, to do the migration
 *
 */

#define HT_BITS 5

struct vm_fault_list {
	DECLARE_HASHTABLE(ht, HT_BITS);

#ifdef CONFIG_SMP
	rwlock_t lock;
#endif

};

#ifdef CONFIG_SMP
static DEFINE_PER_CPU(struct vm_fault_list, vmfs);
#else
static struct vm_fault_list vmfs;
#endif 

static struct kmem_cache *vmfs_cachep;

int setup_vmfs_pcp_lists(void) 
{
	vmfs_cachep = kmem_cache_create(
			"vmfs_cache", 
			sizeof(struct vm_fault_entry), 
			0, 
			SLAB_HWCACHE_ALIGN | SLAB_ACCOUNT | SLAB_RECLAIM_ACCOUNT, 
			NULL);

	if(!vmfs_cachep) {
		scid_err("unable to create a new kmem_cache for vmfs");
		return -ENOMEM;
	}

#ifdef CONFIG_SMP
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct vm_fault_list *l = per_cpu_ptr(&vmfs, cpu);

		hash_init(l->ht);
		rwlock_init(&l->lock);
	}
#else
	hash_init(vmfs.ht);
#endif

	return 0;
}

static inline void __ht_destroy_all(struct vm_fault_list *l) 
{
	struct vm_fault_entry *entry;
	struct hlist_node *tmp;
	unsigned int bucket_idx;

	hash_for_each_safe(l->ht, bucket_idx, tmp, entry, node) {
		hlist_del(&entry->node);
		kfree(entry);
	}
}

/* this must be called *AFTER* unregister_k(ret)probes did its job */
void teardown_vmfs_pcp_lists(void) 
{
	kmem_cache_destroy(vmfs_cachep);

#ifdef CONFIG_SMP
	unsigned int cpu;

	for_each_possible_cpu(cpu)
		__ht_destroy_all(per_cpu_ptr(&vmfs, cpu));
#else
	__ht_destroy_all(&vmfs);
#endif

}

#define __my_hash_for_each_possible(name, obj, member, key) \
	hlist_for_each_entry(obj, &name[hash_long(key, HASH_BITS(name))], member)

/* just tell me if vmf is in the per-CPU list */
static struct vm_fault_entry *__lookup_in_pcp_list_by_vmf(
		struct vm_fault_list *list, struct vm_fault *vmf) 
{
	struct vm_fault_entry *entry;

#ifdef CONFIG_SMP
	unsigned long cpu_flags;
	read_lock_irqsave(&list->lock, cpu_flags);
#endif

	__my_hash_for_each_possible(list->ht, entry, node, (u64) vmf) {
		if(vmf == vmf(entry)) {

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

static inline void __del_entry_from_pcp_list(struct vm_fault_entry *vmfe)

{

#ifdef CONFIG_SMP
	write_lock(vmfe->value.list_lock);
#endif

	hlist_del(&vmfe->node);

#ifdef CONFIG_SMP
	write_unlock(vmfe->value.list_lock);
#endif

}

#define __my_hash_add(hashtable, node, key) \
	hlist_add_head(node, &hashtable[hash_long(key, HASH_BITS(hashtable))])

static inline void __add_entry_to_pcp_list(
		struct vm_fault_list *list, struct vm_fault_entry *vmfe)
{

#ifdef CONFIG_SMP
	vmfe->value.list_lock = &list->lock;

	write_lock(&list->lock);
#endif

	__my_hash_add(list->ht, &vmfe->node, (u64) vmf(vmfe));

#ifdef CONFIG_SMP
	write_unlock(&list->lock);
#endif

}

#undef __my_hash_add

/* preemption disabled by caller */
struct vm_fault_entry* got_this_vmf(struct vm_fault *vmf) 
{
	struct vm_fault_entry *found_entry;
	struct vm_fault_list *my_list;

#ifdef CONFIG_SMP
	unsigned int cpu;
	unsigned int my_cpu;

	my_list = this_cpu_ptr(&vmfs);
#else
	my_list = &vmfs;
#endif

	found_entry = __lookup_in_pcp_list_by_vmf(my_list, vmf);
	if(found_entry)
		return found_entry;

#ifdef CONFIG_SMP
	my_cpu = smp_processor_id();

	for_each_possible_cpu(cpu) {
		struct vm_fault_list *pcp_list;

		if(cpu == my_cpu)
			continue;

		pcp_list = per_cpu_ptr(&vmfs, cpu);
		found_entry = __lookup_in_pcp_list_by_vmf(pcp_list, vmf);
		if(!found_entry)
			continue;

		/* we need to migrate... */
		__del_entry_from_pcp_list(found_entry);
		__add_entry_to_pcp_list(my_list, found_entry);

		return found_entry;
	}
#endif

	return NULL;
}

/* preemption is disabled by caller */
struct vm_fault_entry *add_vmf(struct vm_fault *vmf) 
{
	struct vm_fault_entry *entry;
	struct vm_fault_list *my_list;

	if((entry = kmem_cache_alloc(vmfs_cachep, GFP_ATOMIC)) == NULL) {
		scid_err("memory exhausted");
		return NULL;
	}

	vmf(entry) = vmf;

	private(entry) = NULL;

#ifdef CONFIG_SMP
	entry->value.list_lock = NULL;

	my_list = this_cpu_ptr(&vmfs);
#else
	my_list = &vmfs;
#endif

	__add_entry_to_pcp_list(my_list, entry);

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
void del_vmf(struct vm_fault_entry *entry) 
{
	__del_entry_from_pcp_list(entry);

	kmem_cache_free(vmfs_cachep, entry);
}
