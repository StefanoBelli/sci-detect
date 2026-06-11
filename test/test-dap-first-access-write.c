#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "pte-page-track-dap-hook"
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
			NULL, 4 * PAGE_SIZE, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	die_if(mem == MAP_FAILED);

	/* 
	 * TEST initial WRITE access on the first page 
	 */
	{
		spurious_byte_memwrite(page_nr(1), 'a');

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

	/* TEST another WRITE access on the first page */
	{
		spurious_byte_memwrite(page_nr(1), 'a');

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

	/* TEST another READ access on the first page */
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

	/* TEST another READ access via syscall on the first page */
	{
		die_if(trigger_syscall_pageread(page_nr(1), 10));

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

	/* TEST another WRITE access via syscall */
	{
		die_if(trigger_syscall_pagewrite(page_nr(1), 10));

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
	 * TEST initial WRITE access via syscall on a second page
	 * THIS materializes page -- confusion with CoW, everything should be ok 
	 */
	{
		die_if(trigger_syscall_pagewrite(page_nr(2), 10));

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

	/* TEST another WRITE access via syscall on the second page */
	{
		die_if(trigger_syscall_pagewrite(page_nr(2), 10));

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

	/* TEST another WRITE access on the second page */
	{
		spurious_byte_memwrite(page_nr(2), 'a');

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

	/* TEST another READ access on the second page */
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

	/* TEST another READ access via syscall on the second page */
	{
		die_if(trigger_syscall_pageread(page_nr(2), 10));

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

