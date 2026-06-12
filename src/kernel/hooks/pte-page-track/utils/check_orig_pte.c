#include <hooks/pte-page-track/utils/check_orig_pte.h>

#include <logging.h>

bool check_orig_pte(struct vm_fault *vmf)
{
	if(!(vmf->flags & FAULT_FLAG_WRITE)) {
		scid_err("not a write fault");
		return false;
	}

	if(vmf->flags & FAULT_FLAG_MKWRITE) {
		scid_err("unexpected pte mkwrite fault");
		return false;
	}

	if(!(vmf->flags & FAULT_FLAG_ORIG_PTE_VALID)) {
		scid_err("orig_pte is not valid");
		return false;
	}

	if(pte_none(vmf->orig_pte)) {
		scid_err("orig_pte is none");
		return false;
	}

	if(!pte_present(vmf->orig_pte)) {
		scid_err("orig_pte is not present");
		return false;
	}

	if(pte_write(vmf->orig_pte)) {
		scid_err("orig_pte has write flag");
		return false;
	}

	return true;
}
