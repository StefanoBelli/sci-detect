#include <linux/percpu.h>
#include <linux/slab.h>
#include <linux/rwlock.h>

#include <vmfs.h>
#include <logging.h>

/*
 * why the migration mechanism is needed?
 *  what if a thread gets suspended while in #PF handler
 *  and gets scheduled on another CPU? the process running
 *  in the new CPU doesn't find the corresponding vm_fault
 *  when proceeding with the code (and encountering the hooks)
 *
 *  If this happens, try to look in remote CPUs and, if vm_fault
 *  found, migrate it to your local list.
 *
 * should happen rarely though...
 */
struct vm_fault_entry {
	struct vm_fault *vmf;
	rwlock_t *lock; /* needed to delete: points to pcp-list lock */
	struct list_head node;
};

struct vm_fault_list {
	struct list_head head;
	rwlock_t lock;
};

static DEFINE_PER_CPU(struct vm_fault_list, vmfs);

void setup_vmfs_pcp_lists(void) 
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct vm_fault_list *l = per_cpu_ptr(&vmfs, cpu);

		INIT_LIST_HEAD(&l->head);
		rwlock_init(&l->lock);
	}
}

/* this must be called *AFTER* unregister_k(ret)probes did its job */
void teardown_vmfs_pcp_lists(void) 
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct vm_fault_list *l = per_cpu_ptr(&vmfs, cpu);

		struct vm_fault_entry *entry;
		struct vm_fault_entry *tmp;

		list_for_each_entry_safe(entry, tmp, &l->head, node) {
			kfree(entry);
		}
	}
}

/* just tell me if vmf is in the per-CPU list */
static struct vm_fault_entry *__lookup_vmf_in_pcp_list(
		struct vm_fault_list *list, struct vm_fault *vmf) 
{
	unsigned long cpu_flags;
	struct vm_fault_entry *entry;

	read_lock_irqsave(&list->lock, cpu_flags);

	list_for_each_entry(entry, &list->head, node) {
		if(vmf == entry->vmf) {
			read_unlock_irqrestore(&list->lock, cpu_flags);
			return entry;
		}
	}

	read_unlock_irqrestore(&list->lock, cpu_flags);
	return NULL;
}

static inline void __del_entry_from_pcp_list(struct vm_fault_entry *vmfe)
{
	write_lock(vmfe->lock);
	list_del(&vmfe->node);
	write_unlock(vmfe->lock);
}

static inline void __add_entry_to_pcp_list(
		struct vm_fault_list *list, struct vm_fault_entry *vmfe)
{
	vmfe->lock = &list->lock;

	write_lock(&list->lock);
	list_add(&vmfe->node, &list->head);
	write_unlock(&list->lock);
}

/* preemption disabled by caller */
int got_this_vmf(struct vm_fault *vmf) 
{
	struct vm_fault_list *my_list;
	unsigned int cpu;

	my_list = this_cpu_ptr(&vmfs);
	if(__lookup_vmf_in_pcp_list(my_list, vmf))
		return 1;

	for_each_enabled_cpu(cpu) {
		struct vm_fault_entry *entry;
		struct vm_fault_list *pcp_list;

		pcp_list = per_cpu_ptr(&vmfs, cpu);
		entry = __lookup_vmf_in_pcp_list(pcp_list, vmf);
		if(!entry)
			continue;

		/* we need to migrate... */
		__del_entry_from_pcp_list(entry);
		__add_entry_to_pcp_list(my_list, entry);

		return 1;
	}

	return 0;
}

/* preemption is disabled by caller */
struct vm_fault_entry *add_vmf(struct vm_fault *vmf) 
{
	struct vm_fault_entry *entry;
	struct vm_fault_list *my_list;

	if((entry = kmalloc(sizeof(struct vm_fault_entry), GFP_ATOMIC)) == NULL) {
		scid_err("memory exhausted");
		return NULL;
	}

	entry->vmf = vmf;
	
	my_list = this_cpu_ptr(&vmfs);
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

	kfree(entry);
}

