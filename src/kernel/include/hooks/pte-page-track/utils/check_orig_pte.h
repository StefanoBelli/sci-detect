#ifndef SCID_CHECK_ORIG_PTE_H
#define SCID_CHECK_ORIG_PTE_H

#include <linux/mm.h>

/**
 * Checks validity of orig_pte from the vmf.
 *
 * @vmf: the vmf
 *
 * Returns: true if orig_pte is valid, false otherwise
 */
bool check_orig_pte(struct vm_fault *vmf);

#endif
