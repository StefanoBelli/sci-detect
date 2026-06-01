#include "testutils.h"
#include <sys/mman.h>

#define SUBSYS_NAME "add-dap-hook"
#define ENTRY_KEY "entry"
#define ZEROPAGE_KEY "zero-page"
#define RETURNOK_KEY "return-ok"
#define MATERIALIZEPAGE_KEY "materialize-page"

int main()
{
	const int NUM_PAGES = 22;

	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	start_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	start_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	start_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

	{
		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, NUM_PAGES * 4096, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, 
				-1, 0);
		die_if(mem == MAP_FAILED);

		/* TEST the prefaulting: PTEs must be populated when mmap() is returning... */
		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, NUM_PAGES);
			test_int_eq(returnok, NUM_PAGES);
			test_int_eq(zeropage, 0);
			test_int_eq(materializepage, NUM_PAGES);
		}
	}

	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

	{
		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, NUM_PAGES * 4096, 
				PROT_READ, 
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, 
				-1, 0);
		die_if(mem == MAP_FAILED);

		/* TEST the prefaulting: PTEs must be populated when mmap() 
		 * is returning (with zeropages due to readonly access)... */
		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, NUM_PAGES);
			test_int_eq(returnok, 0);
			test_int_eq(zeropage, NUM_PAGES);
			test_int_eq(materializepage, 0);
		}
	}

	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

	{
		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, NUM_PAGES * 4096, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE | MAP_ANONYMOUS, 
				-1, 0);

		die_if(mem == MAP_FAILED);

		die_if(madvise(mem, NUM_PAGES * 4096, MADV_POPULATE_READ));

		/* TEST the prefaulting: PTEs must be populated when madvise() is returning... */
		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, NUM_PAGES);
			test_int_eq(returnok, 0);
			test_int_eq(zeropage, NUM_PAGES);
			test_int_eq(materializepage, 0);
		}
	}

	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

	{
		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, NUM_PAGES * 4096, 
				PROT_READ, 
				MAP_PRIVATE | MAP_ANONYMOUS, 
				-1, 0);

		die_if(mem == MAP_FAILED);

		die_if(madvise(mem, NUM_PAGES * 4096, MADV_POPULATE_READ));

		/* TEST the prefaulting: PTEs must be populated when mmap() 
		 * is returning (with zeropages due to readonly access)... */
		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, NUM_PAGES);
			test_int_eq(returnok, 0);
			test_int_eq(zeropage, NUM_PAGES);
			test_int_eq(materializepage, 0);
		}
	}

	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

	{
		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, NUM_PAGES * 4096, 
				PROT_READ | PROT_WRITE, 
				MAP_PRIVATE | MAP_ANONYMOUS, 
				-1, 0);

		die_if(mem == MAP_FAILED);

		die_if(madvise(mem, NUM_PAGES * 4096, MADV_POPULATE_READ | MADV_POPULATE_WRITE));

		/* TEST the prefaulting: PTEs must be populated when madvise() is returning... */
		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, NUM_PAGES);
			test_int_eq(returnok, NUM_PAGES);
			test_int_eq(zeropage, 0);
			test_int_eq(materializepage, NUM_PAGES);
		}
	}

	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
	reset_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

	{
		/* PREPARING: do the mmap */
		char *mem = (char*) mmap(
				NULL, NUM_PAGES * 4096, 
				PROT_READ, 
				MAP_PRIVATE | MAP_ANONYMOUS, 
				-1, 0);

		die_if(mem == MAP_FAILED);

		die_if(madvise(mem, (NUM_PAGES >> 1) * 4096, MADV_POPULATE_READ));

		/* TEST the prefaulting: PTEs must be populated when mmap() 
		 * is returning (with zeropages due to readonly access)... */
		{
			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, (NUM_PAGES >> 1));
			test_int_eq(returnok, 0);
			test_int_eq(zeropage, (NUM_PAGES >> 1));
			test_int_eq(materializepage, 0);
		}

		{
			spurious_byte_memread(ch, mem);

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, (NUM_PAGES >> 1));
			test_int_eq(returnok, 0);
			test_int_eq(zeropage, (NUM_PAGES >> 1));
			test_int_eq(materializepage, 0);
		}

		{
			spurious_byte_memread(ch, mem + (4096 * ((NUM_PAGES >> 1) + 1)));

			int entry = query_int_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
			int zeropage = query_int_value_testing_for_me(SUBSYS_NAME, ZEROPAGE_KEY);
			int returnok = query_int_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY);
			int materializepage = query_int_value_testing_for_me(SUBSYS_NAME, MATERIALIZEPAGE_KEY);

			test_int_eq(entry, (NUM_PAGES >> 1) + 1);
			test_int_eq(returnok, 0);
			test_int_eq(zeropage, (NUM_PAGES >> 1) + 1);
			test_int_eq(materializepage, 0);
		}
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

