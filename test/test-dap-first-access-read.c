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
			NULL, 4 * 4096, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 
			-1, 0);
	die_if(mem == MAP_FAILED);

	/* TEST without doing the initial access */
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

	/* TEST after initial READ access has been done */
	{
		spurious_byte_memread(ch, mem);

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
		spurious_byte_memread(ch, mem);

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

	/* TEST initial READ access on the following page */
	{
		spurious_byte_memread(ch, mem + 4100);

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
		spurious_byte_memread(ch, mem + 4100);

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

	/* TEST initial READ access on the third page */
	{
		spurious_byte_memread(ch, mem + 8200);

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
		spurious_byte_memread(ch, mem + 8200);

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

	/* TEST another READ access on the same following page */
	{
		die_if(trigger_syscall_pageread(mem + 8200, 10));

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

	/* TEST initial READ access via system call */
	{
		die_if(trigger_syscall_pageread(mem + 12300, 10));

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

	/* TEST another READ access  */
	{
		spurious_byte_memread(ch, mem + 12300);

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

	/* TEST another READ access via system call */
	{
		die_if(trigger_syscall_pageread(mem + 12300, 10));

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

	/* TEST another READ access  */
	{
		spurious_byte_memread(ch, mem + 12300);

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

