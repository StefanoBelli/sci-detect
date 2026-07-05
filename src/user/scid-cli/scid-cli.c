#define _GNU_SOURCE

#include <getopt.h>
#include <string.h>

#include "scid-cli.h"

/* args for the cli */

static const char *short_opts = "bupfgtaosedxkvlnh";

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
	{ "enable-hexdump", no_argument, NULL, 'e' },
	{ "disable-hexdump", no_argument, NULL, 'd' },
	{ "enable-disasm", no_argument, NULL, 'x' },
	{ "disable-disasm", no_argument, NULL, 'k' },
	{ "disasm-base-va", required_argument, NULL, 'v' },
	{ "disasm-length", required_argument, NULL, 'l' },
	{ "hexdump-length", required_argument, NULL, 'n' },
	{ "help", no_argument, NULL, 'h' },
};

/* program state variables */

static int do_hexdump = 1;
static int do_disasm = 1;
static unsigned long disasm_base_va = 0;
static size_t hexdump_buf_len = SCID_PAGE_SIZE;
static size_t disasm_buf_len = SCID_PAGE_SIZE;

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
	char datetime_buf[100];
	memset(datetime_buf, 0, 100);
	datetime_str(wxw->evt_datetime, datetime_buf, 100);

	printf("wx page: pfn=%ld, va=0x%lx, pid=%d, datetime=%s\n",
			wxw->pfn, wxw->va, wxw->pid, datetime_buf);
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

static void snapshot_pretty_print(
		const struct snapshot_event *snap, int has_buffer, const int64_t *__datetime_field)
{
	char datetime_buf[100];
	memset(datetime_buf, 0, 100);
	datetime_str(*__datetime_field, datetime_buf, 100);

	printf("snapshot: pfn=%ld, va=0x%lx, pid=%d, "
			"seq=%ld, fault=%s, datetime=%s\n",
			snap->pfn, snap->va, snap->pid, snap->seq, 
			snapshot_fault_str(snap->fault), datetime_buf);

	if(has_buffer) {
		if(do_hexdump) 
			print_hexdump(snap->buffer, hexdump_buf_len);

		if(do_disasm)
			print_disasm(
					snap->buffer, disasm_base_va, disasm_buf_len);
	}

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
			struct snapshot_event *snap = le->data;
			printf(" snapshot\n\t");
			snapshot_pretty_print(snap, 0, &snap->evt_datetime);
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
	const struct snapshot_event *snap = args;
	snapshot_pretty_print(snap, 1, &snap->evt_datetime);
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
		struct snapshot_event *snap = le->data;
		printf("snapshot\n\t");
		snapshot_pretty_print(snap, 1, &snap->evt_datetime);
	}
}

static void get_cur_page_snapshot_handler(const void *args, __unused void *uargs)
{
	const struct cur_page_snapshot *cps = args;
	printf("pfn=%ld, found=%s\n", cps->pfn, bool_to_str(cps->pfn_found));

	if(cps->pfn_found) {
		if(cps->snap) {
			struct snapshot_event *snap = cps->snap;
			printf(" snapshot\n\t");
			snapshot_pretty_print(snap, 1, &snap->datetime);
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
		case 'e':
			do_hexdump = 1;
			break;
		case 'd':
			do_hexdump = 0;
			break;
		case 'x':
			do_disasm = 1;
			break;
		case 'k':
			do_disasm = 0;
			break;
		case 'v':
			disasm_base_va = to_ul(optarg);
			break;
		case 'l':
			disasm_buf_len = to_ul(optarg);
			break;
		case 'n':
			hexdump_buf_len = to_ul(optarg);
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
