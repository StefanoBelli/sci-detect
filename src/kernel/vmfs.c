#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/slab.h>
#include <linux/rwlock.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/kprobes.h>
#include <linux/compiler.h>

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
	struct {
		struct vm_fault *vmf;

		/* needed to delete: points to pcp-list lock */
		rwlock_t *list_lock; 

		/* needed to avoid missed kfrees, see the do_exit kprobe */
		struct task_struct *tsk; 

		/* needed to determine specific caller */
		u64 caller; 
	} value;

	struct list_head node;
};

struct vm_fault_list {
	struct list_head head;
	rwlock_t lock;
};

static int __register_do_exit_kprobe(void);
static void __unregister_do_exit_kprobe(void);

static DEFINE_PER_CPU(struct vm_fault_list, vmfs);

int setup_vmfs_pcp_lists(void) 
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct vm_fault_list *l = per_cpu_ptr(&vmfs, cpu);

		INIT_LIST_HEAD(&l->head);
		rwlock_init(&l->lock);
	}

	return __register_do_exit_kprobe();
}

/* this must be called *AFTER* unregister_k(ret)probes did its job */
void teardown_vmfs_pcp_lists(void) 
{
	unsigned int cpu;

	__unregister_do_exit_kprobe();

	for_each_possible_cpu(cpu) {
		struct vm_fault_list *l = per_cpu_ptr(&vmfs, cpu);

		struct vm_fault_entry *entry;
		struct vm_fault_entry *tmp;

		list_for_each_entry_safe(entry, tmp, &l->head, node) {
			kfree(entry);
		}
	}
}

/* callback for __lookup_vmf_in_pcp_list */
typedef int (*__lookup_comparator)(void *arg, struct vm_fault_entry *entry);

/* just tell me if vmf is in the per-CPU list */
static struct vm_fault_entry *__lookup_in_pcp_list(
		struct vm_fault_list *list, void *arg, __lookup_comparator cmp) 
{
	unsigned long cpu_flags;
	struct vm_fault_entry *entry;

	read_lock_irqsave(&list->lock, cpu_flags);

	list_for_each_entry(entry, &list->head, node) {
		if(cmp(arg, entry)) {
			read_unlock_irqrestore(&list->lock, cpu_flags);
			return entry;
		}
	}

	read_unlock_irqrestore(&list->lock, cpu_flags);
	return NULL;
}

/* lookup by vmf */
static int __lookup_compare_by_vmfptr(void *vmf, struct vm_fault_entry *entry)
{
	return (struct vm_fault*) vmf == entry->value.vmf;
}

static inline struct vm_fault_entry *lookup_in_pcp_list_by_vmf(
		struct vm_fault_list *list, struct vm_fault *vmf) 
{
	return __lookup_in_pcp_list(list, vmf, __lookup_compare_by_vmfptr);
}

/* lookup by task */
static int __lookup_compare_by_tskptr(void *tsk, struct vm_fault_entry *entry)
{
	return (struct task_struct*) tsk == entry->value.tsk;
}

static inline struct vm_fault_entry *lookup_in_pcp_list_by_tsk(
		struct vm_fault_list *list, struct task_struct *tsk) 
{
	return __lookup_in_pcp_list(list, tsk, __lookup_compare_by_tskptr);
}

static inline void __del_entry_from_pcp_list(struct vm_fault_entry *vmfe)
{
	write_lock(vmfe->value.list_lock);
	list_del(&vmfe->node);
	write_unlock(vmfe->value.list_lock);
}

static inline void __add_entry_to_pcp_list(
		struct vm_fault_list *list, struct vm_fault_entry *vmfe)
{
	vmfe->value.list_lock = &list->lock;

	write_lock(&list->lock);
	list_add(&vmfe->node, &list->head);
	write_unlock(&list->lock);
}

/* preemption disabled by caller */
struct vm_fault_entry* got_this_vmf(struct vm_fault *vmf) 
{
	struct vm_fault_entry *found_entry;
	struct vm_fault_list *my_list;
	unsigned int cpu;
	unsigned int my_cpu;

	my_list = this_cpu_ptr(&vmfs);
	found_entry = lookup_in_pcp_list_by_vmf(my_list, vmf);
	if(found_entry)
		return found_entry;

	my_cpu = smp_processor_id();

	for_each_enabled_cpu(cpu) {
		struct vm_fault_list *pcp_list;

		if(cpu == my_cpu)
			continue;

		pcp_list = per_cpu_ptr(&vmfs, cpu);
		found_entry = lookup_in_pcp_list_by_vmf(pcp_list, vmf);
		if(!found_entry)
			continue;

		/* we need to migrate... */
		__del_entry_from_pcp_list(found_entry);
		__add_entry_to_pcp_list(my_list, found_entry);

		return found_entry;
	}

	return NULL;
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

	entry->value.vmf = vmf;
	entry->value.tsk = get_current();
	entry->value.caller = 0;
	
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

int is_caller_vmfe(
		struct vm_fault_entry* vmfe, enum caller_enum caller) 
{
	return vmfe->value.caller & caller;
}

void set_caller_vmfe(
		struct vm_fault_entry *vmfe, enum caller_enum caller)
{
	vmfe->value.caller |= caller;
}

void unset_caller_vmfe(
		struct vm_fault_entry *vmfe, enum caller_enum caller)
{
	vmfe->value.caller ^= caller;
}

#define do_exit__symbol "do_exit"

static int do_exit__pkphook(
		__maybe_unused struct kprobe *kp, 
		__maybe_unused struct pt_regs *regs)
{
	struct vm_fault_list *my_list;
	struct vm_fault_entry *found_entry;
	struct task_struct *tsk;
	unsigned int cpu;
	unsigned int my_cpu;

	my_list = this_cpu_ptr(&vmfs);

	/* no need to get refcnt for task_struct here */
	tsk = get_current();

	found_entry = lookup_in_pcp_list_by_tsk(my_list, tsk);
	if(found_entry)
		goto __finish_do_exit_delete;

	my_cpu = smp_processor_id();

	/* unfortunately, we have to do this... */
	for_each_enabled_cpu(cpu) {
		struct vm_fault_list *pcp_list;

		if(cpu == my_cpu)
			continue;

		pcp_list = per_cpu_ptr(&vmfs, cpu);
		found_entry = lookup_in_pcp_list_by_tsk(pcp_list, tsk);
		if(found_entry)
			goto __finish_do_exit_delete;
	}

	goto __finish_do_exit_return;

__finish_do_exit_delete:
	del_vmf(found_entry);
__finish_do_exit_return:
	return 0;
}

static struct kprobe do_exit__kp = {
	.symbol_name = do_exit__symbol,
	.pre_handler = do_exit__pkphook,
};

static int __register_do_exit_kprobe(void) 
{
	return register_kprobe(&do_exit__kp);
}

static void __unregister_do_exit_kprobe(void)
{
	unregister_kprobe(&do_exit__kp);
}
