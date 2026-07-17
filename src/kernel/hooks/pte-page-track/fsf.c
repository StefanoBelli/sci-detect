#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/kprobes.h>
#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/rcupdate.h>
#include <asm/nospec-branch.h>
#include <asm/trap_pf.h>
#include <asm-generic/rwonce.h>

#include <kpsleepable.h>
#include <pgtrack.h>
#include <ptealtprot.h>
#include <logging.h>
#include <sfes.h>

#define WITH_ORIG_IP 0
#define WITH_NEW_IP 1

static struct page *obtain_page_from_addr(unsigned long addr)
{
	struct page *pages[1] = { NULL };
	int nr_pages;

	nr_pages = get_user_pages_fast(addr, 1, 0, pages);
	if(nr_pages == 1) {
		put_page(pages[0]);
		return pages[0];
	}

	scid_errf("unable to get user page, err = %d", nr_pages);
	return NULL;
}

struct kprobe force_sig_fault__kp;

static inline void __do_pte_alt(struct page_status *pgs)
{
	__enable_sleep(&force_sig_fault__kp);

	mutex_lock(&pgs->pap->lock);
	exonly_locked_ptealtprot(pgs, NULL);
	mutex_unlock(&pgs->pap->lock);

	__disable_sleep(&force_sig_fault__kp);
}

static inline struct page* __do_obtain_page_from_addr(void __user *addr)
{
	struct page *page;

	__enable_sleep(&force_sig_fault__kp);
	page = obtain_page_from_addr((unsigned long) addr);
	__disable_sleep(&force_sig_fault__kp);

	return page;
}

#define REQUIRED_CPU_ERROR_CODE ( \
		X86_PF_PROT | \
		X86_PF_USER | \
		X86_PF_INSTR)

static bool fsf_checks_ok(struct pt_regs *regs, void __user **_addr)
{
	int sig = regs->di;
	int code;
	void __user* addr;
	struct sig_fault_extras_entry *sfee;

	/* signal being delivered */
	if(unlikely(sig != SIGSEGV))
		return false;

	code = regs->si;

	/* type of segment violation */
	if(code != SEGV_ACCERR)
		return false;

	addr = (void __user*) regs->dx;

	/* is this the right kernel control path? */
	sfee = got_this_sfe(current);
	if(unlikely(!sfee))
		return false;

	/* this is a consistency check */
	if(sfee->address != (unsigned long) addr || sfee->si_code != code) {
		scid_warn("got different sig fault extras, from validated ones");
		return false;
	}

	/* now check the error code */
	if(sfee->error_code != REQUIRED_CPU_ERROR_CODE)
		return false;

	/* everything is ok */
	*_addr = addr;
	return true;
}

#define force_sig_fault__symbol "force_sig_fault"

static int force_sig_fault__phkphook(
		__always_unused struct kprobe *kp, struct pt_regs *regs)
{
	void __user* addr;
	struct page *page;
	struct page_status *pgs;
	bool pgs_success;
	int rv = WITH_ORIG_IP;

	if(!fsf_checks_ok(regs, &addr))
		return rv;

	page = __do_obtain_page_from_addr(addr);

	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(page_to_pfn(page));
	pgs_success = pgs && try_page_status_get(pgs);
	rcu_read_unlock();

	if(likely(pgs_success)) {
		if(likely(!READ_ONCE(pgs->pap)))
			goto __put_pgs;

		__do_pte_alt(pgs);

		rv = WITH_NEW_IP;
		regs->ip = (unsigned long) __x86_return_thunk;
		goto __put_pgs;
	}
	
	return rv;

__put_pgs:
	page_status_put(pgs);
	return rv;
}

struct kprobe force_sig_fault__kp = {
	.symbol_name = force_sig_fault__symbol,
	.pre_handler = force_sig_fault__phkphook,
};

/* 
 * we need to hook into __bad_area_nosemaphore 
 * to be able to properly distinguish the situation.
 * No other way since the signal's code is either
 * SEGV_MAPERR or SEGV_ACCERR (write or exec, who knows?)
 *
 * But we need more infos (that is, the x86 arch-specific
 * error code)
 */

#define __bad_area_nosemaphore__symbol "__bad_area_nosemaphore"

static int __bad_area_nosemaphore__ehkrphook(
		struct kretprobe_instance *krpi, struct pt_regs *regs)
{
	unsigned long error_code = regs->si;
	unsigned long address = regs->dx;
	int si_code = regs->r8;
	struct task_struct *tsk = current;
	struct sig_fault_extras_entry *sfee;

	sfee = add_sfe(tsk, error_code, address, si_code);
	if(unlikely(!sfee)) {
		scid_err("add_sfe failed");
		return 1;
	}

	*((struct sig_fault_extras_entry**) krpi->data) = sfee;
	return 0;
}

static int __bad_area_nosemaphore__hkrphook(
		struct kretprobe_instance *krpi, __always_unused struct pt_regs *regs)
{
	del_sfe(*((struct sig_fault_extras_entry**) krpi->data));
	return 0;
}

struct kretprobe __bad_area_nosemaphore__krp = {
	.kp.symbol_name = __bad_area_nosemaphore__symbol,
	.entry_handler = __bad_area_nosemaphore__ehkrphook,
	.handler = __bad_area_nosemaphore__hkrphook,
	.data_size = sizeof(struct sig_fault_extras_entry*),
};

#endif /* DO_PTE_ALT_PROT */
