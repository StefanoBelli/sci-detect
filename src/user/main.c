#include <stdio.h>
#include <stdlib.h>
#include <scid.h>

int main()
{
	int err;
	void *desc;

	desc = scid_new_socket(&err);
	if(err) {
		fprintf(stderr, "scid_new_socket failed: %s\n", str_sciderr(desc));
		return EXIT_FAILURE;
	}

	scid_del_socket(desc);
	return EXIT_SUCCESS;
}

