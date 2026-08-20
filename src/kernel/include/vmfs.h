#ifndef SCID_VMFS_H
#define SCID_VMFS_H

#include <linux/mm.h>
#include <kcps.h>
#include <logging.h>

/* exposed, not opaque, to avoid function call overhead */
struct vm_fault_entry {
	/* "key" of the main kernel control path */
	struct vm_fault *vmf;

	enum fault_flag orig_flags;

	/* private hooks data, depends on kernel control path */
	void *private;
};

/* you may access these fields */
#define vmf(entry) ((entry)->vmf)
#define private(entry) ((entry)->private)
#define orig_flags(entry) ((entry)->orig_flags)

static bool __vmf_kcp_comparator(struct kcp_entry *kcpe, u64 key)
{
	return key == (u64) vmf((struct vm_fault_entry*) kcpe->data);
}

/* 
 * got_this_vmf - look for vmf
 *
 * @vmf: the vmf to look for
 *
 * Returns: the vmfe if found, NULL otherwise
 */
static inline struct vm_fault_entry* got_this_vmf(struct vm_fault* vmf)
{
	struct kcp_entry *kcpe = got_this_kcp((u64) vmf, __vmf_kcp_comparator);
	if(unlikely(!kcpe))
		return NULL;

	return kcpe->data;
}

/**
 * add_vmf - add the vmf
 *
 * @vmf: the vmf to add
 * @ff: the ff
 *
 * Returns: the vmfe
 */
static inline struct vm_fault_entry* add_vmf(struct vm_fault* vmf, enum fault_flag ff)
{
	struct kcp_entry *kcpe;
	struct vm_fault_entry *vmfe;

	vmfe = kmalloc(sizeof(struct vm_fault_entry), GFP_ATOMIC);
	if(unlikely(!vmfe)) {
		scid_err("memory exhausted");
		return NULL;
	}

	vmf(vmfe) = vmf;
	private(vmfe) = NULL;
	orig_flags(vmfe) = ff;

	kcpe = add_kcp((u64) vmf, vmfe);
	if(unlikely(!kcpe)) {
		kfree(vmfe);
		return NULL;
	}

	return kcpe->data;
}

/**
 * del_vmf - del the vmfe
 *
 * @vmfe: the vmfe
 */
static inline void del_vmf(struct vm_fault_entry* vmfe)
{
	del_kcp_bykey((u64) vmfe->vmf, __vmf_kcp_comparator);
	kfree(vmfe);
}

#endif
