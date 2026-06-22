#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <scid.h>

static void event_wxwarning_handler(const void *data, void *args)
{
	const struct wxwarning_event *event = data;
	printf("pfn: %ld, va: %ld, pid: %d\n", event->pfn, event->va, event->pid);
}

static void cmd_get_last_events_handler(const void *data, void *args)
{
	const struct all_last_events *events = data;

	printf("there are a total of %d events\n", events->nr);
	for(uint32_t i = 0; i < events->nr; i++) {
		if(events->evts[i].type == WXWARNING) {
			struct wxwarning_event *myevt = events->evts[i].data;
			printf("this is a wxwarning event\n");
			printf(" --> pfn: %ld\n", myevt->pfn);
			printf(" --> va: %ld\n", myevt->va);
			printf(" --> pid: %d\n", myevt->pid);
		} else {
			printf("unknown event\n");
		}
	}
}

static volatile int loop_control = 1;

static void sigint(int sig)
{
	loop_control = 0;
}

int main()
{
	signal(SIGINT, sigint);

	long err;
	int ns_err;
	void *desc;

	desc = scid_new_socket(&ns_err);
	if(ns_err) {
		fprintf(stderr, "scid_new_socket failed: %s\n", str_sciderr(desc));
		return EXIT_FAILURE;
	}

	err = scid_regi_cmd(desc, SCID_GENL_CMD_EVENT_WXWARNING, event_wxwarning_handler);
	if(err) {
		fprintf(stderr, "scid_regi_cmd failed: %s\n", str_sciderr(desc));
		scid_del_socket(desc);
		return EXIT_FAILURE;
	}

	err = scid_regi_cmd(desc, SCID_GENL_CMD_GET_LAST_EVENTS, cmd_get_last_events_handler);
	if(err) {
		fprintf(stderr, "scid_regi_cmd failed: %s\n", str_sciderr(desc));
		scid_del_socket(desc);
		return EXIT_FAILURE;
	}
	/*
	err = scid_broadcast_subscribe(desc);
	if(err) {
		fprintf(stderr, "scid_broadcast_subcscribe failed: %s\n", str_sciderr(desc));
		scid_del_socket(desc);
		return EXIT_FAILURE;
	}

	err = scid_poll_one_message(desc, NULL);
	if(err) {
		fprintf(stderr, "scid_poll_one_message failed (netlink err): %ld\n", err);
		scid_del_socket(desc);
		return EXIT_FAILURE;
	}

	err = scid_broadcast_unsubscribe(desc);
	if(err) {
		fprintf(stderr, "scid_broadcast_subcscribe failed: %s\n", str_sciderr(desc));
		scid_del_socket(desc);
		return EXIT_FAILURE;
	}*/

	err = scid_cmd_get_last_events(desc, NULL);
	if(err) {
		fprintf(stderr, "scid_cmd_get_last_events failed: %s\n", str_sciderr(desc));
		scid_del_socket(desc);
		return EXIT_FAILURE;
	}

	//scid_poll_forever(desc, NULL, &loop_control);

	scid_del_socket(desc);
	return EXIT_SUCCESS;
}

