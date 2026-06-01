#include "testutils.h"
#include <sys/mman.h>

int main()
{
	enable_testing_for_me("add-dap-hook");
	start_value_testing_for_me("add-dap-hook", "success");

	char *mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(mem == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	*mem = 'a';

	char out[8192];
	memset(out, 0, 8192);
	query_value_testing_for_me("add-dap-hook", "success", out, 8192);

	printf("%s\n", out);

	stop_value_testing_for_me("add-dap-hook", "success");
	disable_testing_for_me("add-dap-hook");

	return 0;
}

