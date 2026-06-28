#include <linux/kprobes.h>
#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/pagevec.h>
#include <linux/page-flags.h>

#include <logging.h>
#include <pgtrack.h>
#include <testing/testing.h>

#define MY_TESTING_SUBSYS_NAME "pte-page-track-fuf-hook"

/* ---- testing-only code begins ---- */
#ifdef SCID_CONFIG_TESTING

#define __fuf_test_log(msg, ...) \
	scid_infof("TESTING/free_unref_folios: " msg, __VA_ARGS__)

#define __fuf_test_log_folio_add(tsk, folio) \
	__fuf_test_log("task(name=%s, pid=%d): runs fpasc, ADDED folio(desc=%px)", \
			(tsk)->comm, task_pid_vnr((tsk)), (folio))

#define __fuf_test_log_folio_freed(tsk1, tsk2, folio, elapsed) \
	__fuf_test_log("task(name=%s, pid=%d): runs fuf, FREED folio(desc=%px) " \
			"for task(name=%s, pid=%d), it took %lld secs", \
			(tsk1)->comm, task_pid_vnr((tsk1)), (folio), \
			(tsk2)->comm, task_pid_vnr((tsk2)), elapsed)

#define __fuf_test_log_task_resumes(tsk1, tsk2, elapsed) \
	__fuf_test_log("task(name=%s, pid=%d): allows task(name=%s, pid=%d) " \
			"to RESUME execution, it took %lld secs", \
			(tsk1)->comm, task_pid_vnr((tsk1)), \
			(tsk2)->comm, task_pid_vnr((tsk2)), elapsed)

#define __fuf_test_log_warn_stillthere(tsk1, tsk2, elapsed) \
	__fuf_test_log("task(name=%s, pid=%d): is doing cleanup work, but " \
			"task(name=%s, pid=%d) is STILL THERE waiting for SIGCONT " \
			"since %lld secs", \
			(tsk1)->comm, task_pid_vnr((tsk1)), \
			(tsk2)->comm, task_pid_vnr((tsk2)), elapsed)

#define calc_elapsed_secs_since(x) (ktime_get_seconds() - x)

#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/pid.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/time64.h>

struct fuf_folio {
	struct folio *folio;
	time64_t time_added_folio;
};

struct fuf_test_data {
	struct task_struct *tsk;
	time64_t time_sent_sigstop;
	struct fuf_folio *folios;
	unsigned int nr_folios;
	unsigned int nr_completed_folios;
	struct list_head node;
};

static void __free_and_del_one_fuf_test_data(struct fuf_test_data *entry)
{
	put_task_struct(entry->tsk);
	list_del(&entry->node);
	kfree(entry->folios);
	kfree(entry);
}

static LIST_HEAD(fuf_test_list);
static DEFINE_SPINLOCK(fuf_test_list_lock);

#define free_pages_and_swap_cache__symbol "free_pages_and_swap_cache"

static int free_pages_and_swap_cache__phkphook(
		__always_unused struct kprobe *kp, struct pt_regs *regs)
{
	struct fuf_test_data *data;
	struct encoded_page **pages = (struct encoded_page**) regs->di;
	unsigned int nr = regs->si;

	if(testing_is_sut_key(MY_TESTING_SUBSYS_NAME, "entry", NULL))
		return 0;

	data = kzalloc(sizeof(struct fuf_test_data), GFP_ATOMIC);
	if(!data) {
		scid_err("memory exhausted");
		return 0;
	}

	data->tsk = get_task_struct(current);
	data->nr_completed_folios = 0;

	/* this is "overdimensioned": nr is comprehensive of "NR_PAGES" and "pages" themselves, 
	 * of the encoded_page array */
	data->folios = kzalloc(sizeof(struct fuf_folio) * nr, GFP_ATOMIC);
	if(!data->folios) {
		put_task_struct(data->tsk);
		kfree(data);
		scid_err("memory exhausted");
		return 0;
	}

	/* doesn't count the NR_PAGES of the encoded_page array */
	unsigned int real_nr_folios = 0;

	for(unsigned int i = 0; i < nr; i++) {
		struct folio *folio = page_folio(encoded_page_ptr(pages[i]));

		if(unlikely(encoded_page_flags(pages[i]) & ENCODED_PAGE_BIT_NR_PAGES_NEXT))
			/* skip the following NR_PAGES */
			i++;

		/* collect the current folio */
		data->folios[real_nr_folios].folio = folio;
		data->folios[real_nr_folios].time_added_folio = ktime_get_seconds();
		real_nr_folios++;

		__fuf_test_log_folio_add(current, folio);
	}

	data->nr_folios = real_nr_folios;

	/* publish it */
	unsigned long flags;
	spin_lock_irqsave(&fuf_test_list_lock, flags);
	list_add(&data->node, &fuf_test_list);
	spin_unlock_irqrestore(&fuf_test_list_lock, flags);

	/* STOP current thread, waiting for folios to be 
	 * freed by another thread (or itself in the current 
	 * kernel control path, if refcount reaches 0) */
	send_sig(SIGSTOP, current, 1);

	data->time_sent_sigstop = ktime_get_seconds();

	return 0;
}

struct kprobe free_pages_and_swap_cache__kp = {
	.symbol_name = free_pages_and_swap_cache__symbol,
	.pre_handler = free_pages_and_swap_cache__phkphook,
};

static void fuf_check_folio(struct folio *folio)
{
	unsigned long flags;
	struct fuf_test_data *entry;
	struct fuf_test_data *tmp;

	/* take the lock, this is rather inefficient *BUT*... this is
	 * for testing only! */
	spin_lock_irqsave(&fuf_test_list_lock, flags);

	/* basically: "for each thread waiting to be SIGCONTd" */
	list_for_each_entry_safe(entry, tmp, &fuf_test_list, node) {

		/* "for each folio thread is waiting for to be freed" */
		for(unsigned int i = 0; i < entry->nr_folios; i++) {
			struct fuf_folio *entry_fuf_folio = &entry->folios[i];

			/* if testing folio equals the ""thread folio"" */
			if(folio == entry_fuf_folio->folio) {
				__fuf_test_log_folio_freed(
						current, entry->tsk, entry_fuf_folio->folio, 
						calc_elapsed_secs_since(entry_fuf_folio->time_added_folio));

				/* increment the number of completions for the thread */
				entry->nr_completed_folios++;

				/* same folio twice for the same thread?? */
				break;
			}
		}

		/* let's check if the thread's collected folios are freed */
		if(entry->nr_folios != entry->nr_completed_folios)
			continue;

		/* entry->tsk will (likely) not be current */
		send_sig(SIGCONT, entry->tsk, 1);

		/* calculate elapsed secs from SIGSTOP to SIGCONT (approx) */
		__fuf_test_log_task_resumes(current, entry->tsk, 
				calc_elapsed_secs_since(entry->time_sent_sigstop));

		/* free everything and del from list */
		__free_and_del_one_fuf_test_data(entry);

		/* check against the same folio, for each thread in the list */
	}

	/* release the lock */
	spin_unlock_irqrestore(&fuf_test_list_lock, flags);
}

#define FUF_TESTING_CALL(__kall) __kall

void free_all_fuf_test_list(void);

/* this will get called when module is unloaded */
void free_all_fuf_test_list(void)
{
	struct fuf_test_data *entry;
	struct fuf_test_data *tmp;

	/* no lock taken, when this gets called, point of no return... */
	list_for_each_entry_safe(entry, tmp, &fuf_test_list, node) {
		__fuf_test_log_warn_stillthere(current, entry->tsk, 
				calc_elapsed_secs_since(entry->time_sent_sigstop));

		__free_and_del_one_fuf_test_data(entry);
	}
}

/* ---- testing only code ends ---- */

#else /* !SCID_CONFIG_TESTING */
#define FUF_TESTING_CALL(__kall)
#endif /* SCID_CONFIG_TESTING */

#define free_unref_folios__symbol "free_unref_folios"

static int free_unref_folios__phkphook(
		__always_unused struct kprobe *kp, struct pt_regs *regs)
{
	/* you should not check this key in the user-testing code
	 * ... more like a placeholder */
	__testing("entry");

	struct folio_batch *folios = (struct folio_batch*) regs->di;

	for(unsigned char i = 0; i < folios->nr; i++) {
		struct folio *folio = folios->folios[i];

		FUF_TESTING_CALL(fuf_check_folio(folio));

		unsigned long nr_pages = folio_nr_pages(folio);

		for(unsigned long j = 0; j < nr_pages; j++) {
			struct page *page = folio_page(folio, j);
			pg_untrack(page);
		}
	}

	return 0;
}

struct kprobe free_unref_folios__kp = {
	.symbol_name = free_unref_folios__symbol,
	.pre_handler = free_unref_folios__phkphook,
};

