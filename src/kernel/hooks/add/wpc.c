#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/compiler.h>

#include <vmfs.h>
#include <logging.h>
#include <hooks/add/utils/addpages.h>

/* since private may be changed frequently... */
static_assert(
		__builtin_types_compatible_p(
			typeof(private((struct vm_fault_entry*)0)), 
			typeof(void*)));

#define wp_page_copy__symbol "wp_page_copy"

static int wp_page_copy__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs) 
{
	struct vm_fault *vmf = (struct vm_fault*) regs->di;

	/* are we on the right kernel control path? */
	struct vm_fault_entry *entry = got_this_vmf(vmf);
	if(!entry)
		return 1;

	/* flag that wp_page_copy is being run... 
	 * used later by do_wp_page hook to avoid 
	 * doing more useless checks */
	private(entry) = (void*) 1;

	/* consistency checks */
	if(!(vmf->flags & FAULT_FLAG_WRITE)) {
		scid_err("not a write fault");
		return 1;
	}

	if(vmf->flags & FAULT_FLAG_MKWRITE) {
		scid_err("unexpected pte mkwrite fault");
		return 1;
	}

	if(vmf->flags & FAULT_FLAG_REMOTE) {
		scid_err("write fault for remote task");
		return 1;
	}

	if(!(vmf->flags & FAULT_FLAG_ORIG_PTE_VALID)) {
		scid_err("orig_pte is not valid");
		return 1;
	}

	if(pte_none(vmf->orig_pte)) {
		scid_err("orig_pte is none");
		return 1;
	}

	if(!pte_present(vmf->orig_pte)) {
		scid_err("orig_pte is not present");
		return 1;
	}

	if(pte_write(vmf->orig_pte)) {
		scid_err("orig_pte has write flag");
		return 1;
	}

	/* alright, see you on the return handler... */
	*((struct vm_fault**)krpi->data) = vmf;
	return 0;
}

static bool wpc_further_pte_checks(
		pte_t pte, int rw, __maybe_unused int exec, 
		void* args) 
{
	if(!(pte_flags(pte) & _PAGE_USER)) {
		scid_err("got a kernel mapping instead of a user one");
		return false;
	}

	if(!rw) {
		scid_warn("COW failed, kernel will retry");
		return false;
	}

	struct vm_fault *vmf = (struct vm_fault*) args;

	if(pte_same(pte, vmf->orig_pte)) {
		scid_warn("COW failed, kernel will retry");
		return false;
	}

	return true;
}

static int wp_page_copy__hkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	struct vm_fault *vmf;

	/* should be enough: in the end, wp_page_copy's job is to
	 * allocate new page, copy content and setup PTE and every
	 * failure condition is due to OOM or some issues with 
	 * vmf_anon_prepare (or, HWPOISON) */
	if(regs_return_value(regs) != 0)
		return 0;

	/* get vmf pointer from entry handler */
	vmf = *((struct vm_fault**)krpi->data);

	if(!vmf->pte) {
		scid_err("pte is NULL...");
		return 0;
	}

	if(!add_pages_byfolio(vmf->pte, wpc_further_pte_checks, vmf, true, NULL))
		scid_err("unable to add pages");

	return 0;
}

struct kretprobe wp_page_copy__krp = {
	.entry_handler = wp_page_copy__ehkrphook,
	.handler = wp_page_copy__hkrphook,
	.kp.symbol_name = wp_page_copy__symbol,
	.data_size = sizeof(struct vm_fault*),
};
