#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <scid.h>

#include "scid-cli-macros.h"

static const char *short_opts = "bupfg";

static const struct option opts[] = {
	{ "sub-bcast", no_argument, NULL, 'b' },
	{ "unsub-bcast", no_argument, NULL, 'u' },
	{ "poll-one", no_argument, NULL, 'p' },
	{ "poll-forever", no_argument, NULL, 'f' },
	{ "get-last-events", no_argument, NULL, 'g' },
};

static void sub_bcast(void *desc)
{
	die_if_sciderr(scid_broadcast_subscribe, desc);
}

static void unsub_bcast(void *desc)
{
	die_if_sciderr(scid_broadcast_unsubscribe, desc);
}

static void poll_one(void *desc)
{
	die_if_nlerr(scid_poll_one_message, desc, NULL);
}

static void poll_forever(void *desc)
{
	int loop = 1;

	die_if_nlerr(scid_poll_forever, desc, NULL, &loop);
}

static void get_last_events(void *desc)
{
	die_if_sciderr(scid_cmd_get_last_events, desc, NULL);
}

static void wxwarning_pretty_print(const struct wxwarning_event *wxw)
{
	printf("wx page: pfn=%ld, va=0x%lx, pid=%d\n",
			wxw->pfn, wxw->va, wxw->pid);
}

static void wxwarning_event_handler(const void *args, __unused void *uargs)
{
	wxwarning_pretty_print(args);
}

static void get_last_events_handler(const void *args, __unused void *uargs)
{
	const struct all_last_events *ale = args;

	printf("number of events: %d\n", ale->nr);
	
	for(uint32_t i = 0; i < ale->nr; i++) {
		struct last_event *le = &ale->evts[i];
		printf("event %d of type", i + 1);
		if(le->type == WXWARNING) {
			printf(" wx page detection\n\t");
			wxwarning_pretty_print(le->data);
		}
	}
}

static void register_all_handlers(void *desc)
{
	die_if_sciderr(
			scid_regi_cmd, 
				desc, SCID_GENL_CMD_GET_LAST_EVENTS, get_last_events_handler);

	die_if_sciderr(
			scid_regi_cmd, 
				desc, SCID_GENL_CMD_EVENT_WXWARNING, wxwarning_event_handler);
}

int main(int argc, char **argv)
{
	int c;

	void *desc;
	int start_err;

	desc = scid_new_socket(&start_err);
	if(start_err) {
		fprintf(stderr, "scid_new_socket failed: %s\n", str_sciderr((long) desc));
		return EXIT_FAILURE;
	}

	register_all_handlers(desc);

	while ((c = getopt_long_only(argc, argv, short_opts, opts, NULL)) != -1) {
		switch(c) {
			case 'b':
				sub_bcast(desc);
				break;
			case 'u':
				unsub_bcast(desc);
				break;
			case 'p':
				poll_one(desc);
				break;
			case 'f':
				poll_forever(desc);
				break;
			case 'g':
				get_last_events(desc);
				break;
		}
	}

	scid_del_socket(desc);
	return 0;
}
