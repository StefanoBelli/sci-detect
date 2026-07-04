#define _GNU_SOURCE

#include <getopt.h>
#include <scid.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "scid-cli.h"

/* args for the cli */

static const char *short_opts = "bupfgtaosh";

static const struct option opts[] = {
	{ "sub-bcast", no_argument, NULL, 'b' },
	{ "unsub-bcast", no_argument, NULL, 'u' },
	{ "poll-one", no_argument, NULL, 'p' },
	{ "poll-forever", no_argument, NULL, 'f' },
	{ "get-last-events", no_argument, NULL, 'g' },
	{ "is-tracked-page", required_argument, NULL, 't' },
	{ "get-all-tracked-pages", no_argument, NULL, 'a' },
	{ "get-one-last-event", required_argument, NULL, 'o' },
	{ "get-cur-page-snapshot", required_argument, NULL, 's' },
	{ "help", no_argument, NULL, 'h' },
};

/* impls */

static const char* requires_arg_str(int val)
{
	switch(val) {
		case required_argument:
			return "requires argument";

		case no_argument:
			return "no argument";

		default:
			return "idk about arg";
	}
}

static void print_help(const char* filename)
{
	printf("usage: %s [cmd0] [cmd1] [cmd2] <arg> ...\n", filename);
	for(size_t i = 0; i < __static_array_size(opts); i++)
		printf(" -%c, --%s: %s\n", 
				opts[i].val, opts[i].name, 
				requires_arg_str(opts[i].has_arg));
}

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

static void is_tracked_page(void *desc, unsigned long pfn)
{
	die_if_sciderr(scid_cmd_is_tracked_page, desc, NULL, pfn);
}

static void get_all_tracked_pages(void *desc)
{
	die_if_sciderr(scid_cmd_get_all_tracked_pages, desc, NULL);
}

static void get_one_last_event(void *desc, uint32_t idx)
{
	die_if_nlerr(scid_cmd_get_one_last_event, desc, NULL, idx);
}

static void get_cur_page_snapshot(void *desc, unsigned long pfn)
{
	die_if_nlerr(scid_cmd_get_cur_page_snapshot, desc, NULL, pfn);
}

static void wxwarning_pretty_print(const struct wxwarning_event *wxw)
{
	printf("wx page: pfn=%ld, va=0x%lx, pid=%d\n",
			wxw->pfn, wxw->va, wxw->pid);
}

static const char* snapshot_fault_str(enum snapshot_fault fault)
{
	switch(fault) {
		case SNAPSHOT_NO_FAULT:
			return "no";
		case SNAPSHOT_WRITE_FAULT:
			return "write";
		case SNAPSHOT_IFETCH_FAULT:
	 		return "instr-fetch";
	 	default:
	 		return "unknown???";
	}
}

static void datetime_str(time_t time, char *buf, size_t max_size)
{
    struct tm tinfo;

    if (localtime_r(&time, &tinfo) == NULL)
        return;

    strftime(buf, max_size, "%Y-%m-%d %H:%M:%S", &tinfo);
}

#define mod_none(x) (x)
#define mod_isprint(x) isprint(x) ? (x) : '.'

#define GEN_BUFS_ACCESSES_16(__buf, __i, __mod) \
	__mod((unsigned char) (__buf)[(__i) + 0]), __mod((unsigned char) (__buf)[(__i) + 1]), \
	__mod((unsigned char) (__buf)[(__i) + 2]), __mod((unsigned char) (__buf)[(__i) + 3]), \
	__mod((unsigned char) (__buf)[(__i) + 4]), __mod((unsigned char) (__buf)[(__i) + 5]), \
	__mod((unsigned char) (__buf)[(__i) + 6]), __mod((unsigned char) (__buf)[(__i) + 7]), \
	__mod((unsigned char) (__buf)[(__i) + 8]), __mod((unsigned char) (__buf)[(__i) + 9]), \
	__mod((unsigned char) (__buf)[(__i) + 10]), __mod((unsigned char) (__buf)[(__i) + 11]), \
	__mod((unsigned char) (__buf)[(__i) + 12]), __mod((unsigned char) (__buf)[(__i) + 13]), \
	__mod((unsigned char) (__buf)[(__i) + 14]), __mod((unsigned char) (__buf)[(__i) + 15]) \

static void print_hexdump(const char* buf)
{
	puts("---[ hexdump below ]---");
	for(size_t i = 0; i < SCID_PAGE_SIZE; i += 16)
		printf(
				"%ld:\t"
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\t"
				"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
					i, 
					GEN_BUFS_ACCESSES_16(buf, i, mod_none), 
					GEN_BUFS_ACCESSES_16(buf, i, mod_isprint));
	puts("---[ hexdump above ]---");
}

#undef GEN_BUFS_ACCESSES_16
#undef mod_none
#undef mod_isprint

static void snapshot_pretty_print(const struct snapshot_event *snap, int has_buffer)
{
	char datetime_buf[100];
	memset(datetime_buf, 0, 100);
	datetime_str(snap->datetime, datetime_buf, 100);

	printf("snapshot: pfn=%ld, va=0x%lx, pid=%d, "
			"seq=%ld, fault=%s, datetime=%s\n",
			snap->pfn, snap->va, snap->pid, snap->seq, 
			snapshot_fault_str(snap->fault), datetime_buf);

	if(has_buffer)
		print_hexdump(snap->buffer);
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
		} else if(le->type == SNAPSHOT) {
			printf(" snapshot\n\t");
			snapshot_pretty_print(le->data, 0);
		} else
			puts(" unknown event type");
	}
}

static void is_tracked_page_handler(const void *args, __unused void *uargs)
{
	const struct is_tracked_page *itp = args;

	printf("page: pfn=%ld, found=%s", 
			itp->pfn, 
			bool_to_str(itp->pfn_found));

	if(!itp->pfn_found) {
		puts("");
		return;
	}

	printf(", writable=%s, executable=%s\n", 
			bool_to_str(itp->page_writable), 
			bool_to_str(itp->page_executable));

	printf("\tpids: ");

	for(uint32_t i = 0; i < itp->nr_pids; i++)
		printf("%d, ",itp->pids[i]);

	puts("");
}

static void snapshot_event_handler(const void *args, __unused void *uargs)
{
	snapshot_pretty_print(args, 1);
}

static void get_all_tracked_pages_handler(const void *args, __unused void *uargs)
{
	printf("%ld\n", (unsigned long) args);
}

static void get_one_last_event_handler(const void *args, __unused void *uargs)
{
	const struct last_event *le = args;

	if(le->type == WXWARNING) {
		printf("wx page detection\n\t");
		wxwarning_pretty_print(le->data);
	} else if(le->type == SNAPSHOT) {
		printf("snapshot\n\t");
		snapshot_pretty_print(le->data, 1);
	}
}

static void get_cur_page_snapshot_handler(const void *args, __unused void *uargs)
{
	const struct cur_page_snapshot *cps = args;
	printf("pfn=%ld, found=%s\n", cps->pfn, bool_to_str(cps->pfn_found));

	if(cps->pfn_found) {
		if(cps->snap) {
			printf(" snapshot\n\t");
			snapshot_pretty_print(cps->snap, 1);
		} else
			puts(" no snapshot available");
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

	die_if_sciderr(
			scid_regi_cmd, 
				desc, SCID_GENL_CMD_EVENT_SNAPSHOT, snapshot_event_handler);

	die_if_sciderr(
			scid_regi_cmd, 
				desc, SCID_GENL_CMD_IS_TRACKED_PAGE, is_tracked_page_handler);

	die_if_sciderr(
			scid_regi_cmd, 
				desc, SCID_GENL_CMD_GET_ALL_TRACKED_PAGES, get_all_tracked_pages_handler);

	die_if_sciderr(
			scid_regi_cmd, 
				desc, SCID_GENL_CMD_GET_ONE_LAST_EVENT, get_one_last_event_handler);

	die_if_sciderr(
			scid_regi_cmd, 
				desc, SCID_GENL_CMD_GET_CUR_PAGE_SNAPSHOT, get_cur_page_snapshot_handler);
}

static void dispatch_cmd(const char* filename, void *desc, char c)
{
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
		case 't':
			is_tracked_page(desc, to_ul(optarg));
			break;
		case 'a':
			get_all_tracked_pages(desc);
			break;
		case 'o':
			get_one_last_event(desc, to_ul(optarg));
			break;
		case 's':
			get_cur_page_snapshot(desc, to_ul(optarg));
			break;
		case 'h':
			print_help(filename);
			break;
	}
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

	while ((c = getopt_long_only(argc, argv, short_opts, opts, NULL)) != -1)
		dispatch_cmd(argv[0], desc, c);

	scid_del_socket(desc);
	return EXIT_SUCCESS;
}
