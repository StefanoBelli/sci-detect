#include <ptealtprot.h>

#ifdef DO_PTE_ALT_PROT

#include <linux/kprobes.h>
#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/rcupdate.h>
#include <asm/nospec-branch.h>
#include <asm-generic/rwonce.h>

#include <kpsleepable.h>
#include <pgtrack.h>
#include <ptealtprot.h>
#include <logging.h>

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


#define force_sig_fault__symbol "force_sig_fault"

static int force_sig_fault__phkphook(
		__always_unused struct kprobe *kp, struct pt_regs *regs)
{
	int sig = (int) regs->di;
	void __user* addr;
	struct page *page;
	struct page_status *pgs;
	bool pgs_success;
	int rv = WITH_ORIG_IP;

	if(unlikely(sig != SIGSEGV))
		return WITH_ORIG_IP;

	addr = (void __user*) regs->dx;
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

#endif /* DO_PTE_ALT_PROT */
