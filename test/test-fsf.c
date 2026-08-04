#include "testutils.h"

#define SUBSYS_NAME "pte-page-track-fsf-hook"
#define ENTRY_KEY "entry"
#define CHECKSOK_KEY "checks-ok"

#define RESET_ALL() \
	reset_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, RETURNOK_KEY); \
	reset_value_testing_for_me(SUBSYS_NAME, PAGESOK_KEY); \

int main()
{
	MLOCKALL_CURRENTONLY();

	int rv = EXIT_SUCCESS;

	enable_testing_for_me(SUBSYS_NAME);
	start_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	start_value_testing_for_me(SUBSYS_NAME, CHECKSOK_KEY);

	/* in the future we might enable testing for this */

	test_passed();

//__finish:
	stop_value_testing_for_me(SUBSYS_NAME, ENTRY_KEY);
	stop_value_testing_for_me(SUBSYS_NAME, CHECKSOK_KEY);
	disable_testing_for_me(SUBSYS_NAME);

	return rv;
}
