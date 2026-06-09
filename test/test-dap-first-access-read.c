#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "add-dap-hook"
#define ENTRY_KEY "entry"
#define ZEROPAGE_KEY "zero-page"
#define RETURNOK_KEY "return-ok"
#define MATERIALIZEPAGE_KEY "materialize-page"

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY); \

int main()
{
	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	start_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	start_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	start_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

	/* PREPARING: do the mmap */
	char *mem = (char*) mmap(
			NULL, 10 * PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	die_if(mem == MAP_FAILED);

	/* 
	 * TEST without doing the initial access (any) 
	 */
	{
		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* 
	 * TEST after initial READ access has been done (first page)
	 */
	{
		spurious_byte_memread(ch, page_nr(1));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 1);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST another READ access on same page */
	{
		spurious_byte_memread(ch, page_nr(1));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* 
	 * TEST initial READ access on the following page 
	 */
	{
		spurious_byte_memread(ch, page_nr(2));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 1);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST another READ access on the same following page */
	{
		spurious_byte_memread(ch, page_nr(2));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* 
	 * TEST initial READ access on the third page 
	 */
	{
		spurious_byte_memread(ch, page_nr(3));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 1);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST another READ access on the same third page */
	{
		spurious_byte_memread(ch, page_nr(3));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST another READ access on the same third page */
	{
		die_if(trigger_syscall_pageread(page_nr(3), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* 
	 * TEST initial READ access via system call on a fourth page
	 */
	{
		die_if(trigger_syscall_pageread(page_nr(4), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST another READ access on the fourth page */
	{
		spurious_byte_memread(ch, page_nr(4));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 1);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST another READ access via system call on the fourth page */
	{
		die_if(trigger_syscall_pageread(page_nr(4), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST another READ access on the fourth page */
	{
		spurious_byte_memread(ch, page_nr(4));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* 
	 * TEST first READ access via system call on a fifth page 
	 */
	{
		die_if(trigger_syscall_pageread(page_nr(5), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST second WRITE access via system call on the fifth page */
	{
		die_if(trigger_syscall_pagewrite(page_nr(5), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 1);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 1);
	}

	RESET_ALL();

	/* 
	 * TEST first READ access via system call on a sixth page 
	 */
	{
		die_if(trigger_syscall_pageread(page_nr(6), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST second WRITE access via memory access on the sixth page, no CoW takes place */
	{
		spurious_byte_memwrite(page_nr(6), 'a');

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 1);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 1);
	}

	RESET_ALL();

	/* 
	 * TEST first READ access via system call on seventh page
	 */
	{
		die_if(trigger_syscall_pageread(page_nr(7), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST second READ access via memory access on the seventh page */
	{
		spurious_byte_memread(ch, page_nr(7));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 1);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST third WRITE access via memory access on the seventh page -- this must trigger CoW for zeropage */
	{
		spurious_byte_memwrite(page_nr(7), 'a');

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* 
	 * TEST first READ access via system call on the 8th page 
	 */
	{
		die_if(trigger_syscall_pageread(page_nr(8), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST second READ access via memory access on the 8th page */
	{
		spurious_byte_memread(ch, page_nr(8));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 1);
		test_int_eq(materializepage, 0);
	}

	RESET_ALL();

	/* TEST third WRITE access via syscall on the 8th page -- this must trigger CoW for zeropage */
	{
		die_if(trigger_syscall_pagewrite(page_nr(8), 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 0);
		test_int_eq(returnok, 0);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 0);
	}

	test_passed();

__finish:
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}

