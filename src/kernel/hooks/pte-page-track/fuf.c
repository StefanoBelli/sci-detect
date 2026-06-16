#include <linux/kprobes.h>
#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/pagevec.h>
#include <linux/page-flags.h>

#include <logging.h>
#include <testing/testing.h>

#define MY_TESTING_SUBSYS_NAME "pte-page-track-fuf-hook"

#define free_unref_folios__symbol "free_unref_folios"

static int free_unref_folios__phkphook(
		__always_unused struct kprobe *kp, struct pt_regs *regs)
{
	__testing("called");

	struct folio_batch *folios = (struct folio_batch*) regs->di;

	for(unsigned char i = 0; i < folios->nr; i++) {
		struct folio *folio = folios->folios[i];
		unsigned long nr_pages = folio_nr_pages(folio);

		for(unsigned long j = 0; j < nr_pages; j++) {
			struct page *page = folio_page(folio, j);
			//stop tracking the page, as refcnt dropped to 0
		}
	}

	return 0;
}

struct kprobe free_unref_folios__kp = {
	.symbol_name = free_unref_folios__symbol,
	.pre_handler = free_unref_folios__phkphook,
};

