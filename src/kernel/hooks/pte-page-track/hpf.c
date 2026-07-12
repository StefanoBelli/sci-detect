#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <ptealtprot.h>
#include <pgtrack.h>

#ifdef DO_PTE_ALT_PROT
#include <linux/rcupdate.h>
#include <resolve_syms/pte_offset_map_rw_nolock.h>
#endif /* DO_PTE_ALT_PROT */

#define handle_pte_fault__symbol "handle_pte_fault"

static int handle_pte_fault__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	WARN_ON(irqs_disabled());

	struct vm_fault *vmf;
	struct vm_fault_entry *entry;

	vmf = (struct vm_fault*) regs->di;
	if(!vmf) {
		scid_warn("vmf is NULL");
		return 1;
	}

#ifdef DO_PTE_ALT_PROT
	pmd_t dummy;
	spinlock_t *ptl;
	bool is_present_pte;

	if(!vmf->pmd)
		goto __TODO_remove_this;

	pte_t *ptep = THUNK(pte_offset_map_rw_nolock)(vmf->vma->vm_mm, vmf->pmd, vmf->address, &dummy, &ptl);

	if(ptep) {
		spin_lock(ptl);
		pte_t pte = ptep_get(ptep);
		is_present_pte = pte_present(pte);
		spin_unlock(ptl);
		pte_unmap(ptep);
	}
#endif

__TODO_remove_this:
	entry = add_vmf(vmf);
	if(!entry) {
		scid_err("add_vmf failed");
		return 1;
	}

	if(vmf->flags & FAULT_FLAG_INSTRUCTION) {
		scid_infof("instr fault (hpf), present pte = %d, pid = %d", is_present_pte, task_pid_vnr(current));
	}

	*((struct vm_fault_entry**)krpi->data) = entry;

	return 0;
}

static int handle_pte_fault__hkrphook(
		struct kretprobe_instance *krpi, __maybe_unused struct pt_regs *regs)
{
	struct vm_fault_entry *vmfe = *((struct vm_fault_entry **)krpi->data);
	__maybe_unused struct vm_fault *_vmf = vmf(vmfe);
	__maybe_unused struct page_status *pgs;
	struct page *page;

	del_vmf(vmfe);

#ifdef DO_PTE_ALT_PROT
	if(!_vmf->pte)
		return 0;

	/* TODO multiple pages affected */

	page = pte_page(ptep_get(_vmf->pte));

	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(page_to_pfn(page));

	if(pgs) {
		spin_lock(&pgs->pg_mms.lock);

		add_mm_to_pgs_locked(pgs, _vmf->vma->vm_mm, _vmf->address);

		if(atomic64_read(&pgs->perms) == PERM_BITS) {
			scid_info("got here");
			/* we don't care about this... */
			pgs->pg_mms.first_alt_handled = false;
			alternate_ptes_locked(pgs, _vmf);
		}

		spin_unlock(&pgs->pg_mms.lock);
	}

	rcu_read_unlock();

#endif /* DO_PTE_ALT_PROT */

	return 0;
}

struct kretprobe handle_pte_fault__krp = {
	.entry_handler = handle_pte_fault__ehkrphook,
	.handler = handle_pte_fault__hkrphook,
	.kp.symbol_name = handle_pte_fault__symbol,
	.data_size = sizeof(struct vm_fault_entry*),
};

static int handle_mm_fault__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	unsigned int flags = (unsigned int) regs->dx;

	if(flags & FAULT_FLAG_INSTRUCTION) {
		scid_infof("instr fault, pid = %d", task_pid_vnr(current));
	}

	return 0;
}

static int handle_mm_fault__hkrphook(
		struct kretprobe_instance *krpi, __maybe_unused struct pt_regs *regs)
{
	unsigned long ret = regs_return_value(regs);

	if(ret & VM_FAULT_ERROR) {
		scid_infof("VM_FAULT_ERROR, pid = %d", task_pid_vnr(current));
	}

	return 0;
}

struct kretprobe handle_mm_fault__krp = {
	.entry_handler = handle_mm_fault__ehkrphook,
	.handler = handle_mm_fault__hkrphook,
	.kp.symbol_name = "handle_mm_fault",
	.data_size = 0,
};
