#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "add-dap-hook"
#define ENTRY_KEY "entry"
#define ZEROPAGE_KEY "zero-page"
#define RETURNOK_KEY "return-ok"
#define MATERIALIZEPAGE_KEY "materialize-page"

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
			NULL, 4 * 4096, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	die_if(mem == MAP_FAILED);

	/* TEST initial WRITE access  */
	{
		spurious_byte_memwrite(mem, 'a');

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 1);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 1);
	}

	/* TEST another WRITE access  */
	{
		spurious_byte_memwrite(mem, 'a');

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 1);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 1);
	}

	/* TEST another READ access  */
	{
		spurious_byte_memread(ch, mem);

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 1);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 1);
	}

	/* TEST another READ access via syscall */
	{
		die_if(trigger_syscall_pageread(mem, 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 1);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 1);
	}

	/* TEST another WRITE access via syscall */
	{
		die_if(trigger_syscall_pagewrite(mem, 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 1);
		test_int_eq(returnok, 1);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 1);
	}

	/* TEST initial WRITE access via syscall */
	{
		die_if(trigger_syscall_pagewrite(mem + 4098, 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 2);
		test_int_eq(returnok, 2);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 2);
	}

	/* TEST another WRITE access via syscall */
	{
		die_if(trigger_syscall_pagewrite(mem + 4098, 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 2);
		test_int_eq(returnok, 2);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 2);
	}

	/* TEST another WRITE access  */
	{
		spurious_byte_memwrite(mem + 4125, 'a');

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 2);
		test_int_eq(returnok, 2);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 2);
	}

	/* TEST another READ access  */
	{
		spurious_byte_memread(ch, mem + 4200);

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 2);
		test_int_eq(returnok, 2);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 2);
	}

	/* TEST another READ access via syscall */
	{
		die_if(trigger_syscall_pageread(mem + 4200, 10));

		int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
		int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
		int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
		int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

		test_int_eq(entry, 2);
		test_int_eq(returnok, 2);
		test_int_eq(zeropage, 0);
		test_int_eq(materializepage, 2);
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

