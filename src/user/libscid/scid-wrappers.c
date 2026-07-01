#include "scid-internals.h"

/*
 * global policy to enforce
 */
const struct nla_policy global_policy[SCID_GENL_MAX_NR_ATTRS + 1] = {
	[SCID_GENL_ATTR_ARRAY_NR_ELEMS] = { .type = NLA_U32 },
	[SCID_GENL_ATTR_ARRAY] = { .type = NLA_NESTED },
	[SCID_GENL_ATTR_VA] = { .type = NLA_U64 },
	[SCID_GENL_ATTR_PFN] = { .type = NLA_U64 },
	[SCID_GENL_ATTR_PID] = { .type = NLA_S32 },
	[SCID_GENL_ATTR_EVT_TYPE] = { .type = NLA_U32 },
	[SCID_GENL_ATTR_PFN_FOUND] = { .type = NLA_U32 },
	[SCID_GENL_ATTR_PAGE_WRITABLE] = { .type = NLA_S32 },
	[SCID_GENL_ATTR_PAGE_EXECUTABLE] = { .type = NLA_S32 },
	[SCID_GENL_ATTR_GENIDX] = { .type = NLA_U32 },
	[SCID_GENL_ATTR_PAGE_SNAPSHOT_SEQ] = { .type = NLA_U32 },
	[SCID_GENL_ATTR_PAGE_SNAPSHOT_DATETIME] = { .type = NLA_U64 },
	[SCID_GENL_ATTR_PAGE_SNAPSHOT] = { 
		.type = NLA_BINARY, 
		.minlen = SCID_PAGE_SIZE, 
		.maxlen = SCID_PAGE_SIZE,
	},
};

#define __nla_assign_or_skip(var, attr, type) \
	if((attr)) \
		(var) = nla_get_##type((attr)); \
	else \
		return NL_SKIP

/*
 * response wrappers
 */

/* wxwarning event (broadcasted) */

static int __scid_wrapper_event_wxwarning(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	struct wxwarning_event evt;
	memset(&evt, 0, sizeof(evt));

	__nla_assign_or_skip(evt.pid, attrs[SCID_GENL_ATTR_PID], s32);
	__nla_assign_or_skip(evt.pfn, attrs[SCID_GENL_ATTR_PFN], u64);
	__nla_assign_or_skip(evt.va, attrs[SCID_GENL_ATTR_VA], u64);

	user_handler(&evt, uargs);

	return NL_OK;
}

/* snapshot event (broadcasted) */

static int __scid_wrapper_event_snapshot(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	return NL_OK;
}

/* get_last_events cmd */

static void free_all_last_events(struct all_last_events *all)
{
	if(!all->nr || !all->evts)
		return;

	for(uint32_t i = 0; i < all->nr; i++) {
		void *data = all->evts[i].data;
		if(data)
			free(data);
	}

	free(all->evts);
}

static int populate_last_evt_wxwarning(struct last_event *evt, struct nlattr *attr)
{
	struct wxwarning_event *wxw;

	if(!evt->data) {
		evt->data = malloc(sizeof(struct wxwarning_event));
		if(!evt->data)
			return NL_STOP;
	}

	wxw = evt->data;

	if(nla_type(attr) == SCID_GENL_ATTR_VA)
		wxw->va = nla_get_u64(attr);
	else if(nla_type(attr) == SCID_GENL_ATTR_PID)
		wxw->pid = nla_get_s32(attr);
	else if(nla_type(attr) == SCID_GENL_ATTR_PFN)
		wxw->pfn = nla_get_u64(attr);

	return NL_OK;
}

static int __scid_wrapper_get_last_events(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	int rv = NL_OK;
	int rem;
	struct nlattr *pos;
	struct all_last_events all_evts;

	memset(&all_evts, 0, sizeof(all_evts));

	__nla_assign_or_skip(all_evts.nr, attrs[SCID_GENL_ATTR_ARRAY_NR_ELEMS], u32);

	if(!attrs[SCID_GENL_ATTR_ARRAY])
		return NL_SKIP;

	if(!all_evts.nr)
		goto __finish;

	all_evts.evts = malloc(sizeof(struct last_event) * all_evts.nr);
	if(!all_evts.evts)
		return NL_STOP;

	memset(all_evts.evts, 0, all_evts.nr * sizeof(struct last_event));

	/* this may be a bit tricky... the evt type attr helps distinguish index */
	int i = -1;

	/* suppress compiler warning about non-initialized var usage */
	enum last_event_type i_evt_type = WXWARNING;

	nla_for_each_nested(pos, attrs[SCID_GENL_ATTR_ARRAY], rem) {
		int pos_is_evt_type = nla_type(pos) == SCID_GENL_ATTR_EVT_TYPE;

		/* consistency checks */
		if(i < 0 && !pos_is_evt_type)
			abort();

		if(pos_is_evt_type) {
			i_evt_type = nla_get_u32(pos);

			i++;

			/* consistency checks */
			if(i < 0 || i >= (int) all_evts.nr) 
				abort();

			all_evts.evts[i].type = i_evt_type;
			continue;
		}

		if(i_evt_type == WXWARNING)
			rv = populate_last_evt_wxwarning(&all_evts.evts[i], pos);

		if(rv != NL_OK)
			goto __finish_onlyfree;
	}

__finish:
	user_handler(&all_evts, uargs);

__finish_onlyfree:
	free_all_last_events(&all_evts);

	return rv;
}

/* is_tracked_page */

static int __scid_wrapper_is_tracked_page(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	struct nlattr *pos;
	int rem;
	struct is_tracked_page itp;

	memset(&itp, 0, sizeof(itp));

	__nla_assign_or_skip(itp.pfn, attrs[SCID_GENL_ATTR_PFN], u64);
	__nla_assign_or_skip(itp.pfn_found, attrs[SCID_GENL_ATTR_PFN_FOUND], u32);

	if(!itp.pfn_found)
		goto __call_uhandler;

	__nla_assign_or_skip(itp.page_writable, attrs[SCID_GENL_ATTR_PAGE_WRITABLE], s32);
	__nla_assign_or_skip(itp.page_executable, attrs[SCID_GENL_ATTR_PAGE_EXECUTABLE], s32);

	__nla_assign_or_skip(itp.nr_pids, attrs[SCID_GENL_ATTR_ARRAY_NR_ELEMS], u32);

	if(!attrs[SCID_GENL_ATTR_ARRAY])
		return NL_SKIP;

	itp.pids = malloc(sizeof(pid_t) * itp.nr_pids);
	if(!itp.pids)
		return NL_STOP;

	uint32_t i = 0;
	nla_for_each_nested(pos, attrs[SCID_GENL_ATTR_ARRAY], rem) {
		if(nla_type(pos) != SCID_GENL_ATTR_PID || i == itp.nr_pids)
			abort();

		itp.pids[i++] = nla_get_s32(pos);
	}

__call_uhandler:
	user_handler(&itp, uargs);

	if(itp.pids)
		free(itp.pids);

	return NL_OK;
}

/* get_all_tracked_[wx_]pages */

static inline int __get_all_tracked_pages_base(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	unsigned long pfn;
	__nla_assign_or_skip(pfn, attrs[SCID_GENL_ATTR_PFN], u64);
	user_handler((const void*) pfn, uargs);
	return NL_OK;
}

static int __scid_wrapper_get_all_tracked_pages(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	return __get_all_tracked_pages_base(user_handler, attrs, uargs);
}

static int __scid_wrapper_get_all_tracked_wx_pages(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	return __get_all_tracked_pages_base(user_handler, attrs, uargs);
}

/* get_one_last_event */

static int __scid_wrapper_get_one_last_event(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	return NL_OK;
}

/* get_cur_page_snapshot */

static int __scid_wrapper_get_cur_page_snapshot(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	return NL_OK;
}

#define __static_array_size(x) (sizeof(x) / sizeof(typeof(x[0])))

/*
 * compile-time regi defs
 */
const struct cmd_reg_info __static_regi_defs[] = {
	{
		.cmd = SCID_GENL_CMD_EVENT_WXWARNING,
		.wrapper_handler = __scid_wrapper_event_wxwarning,
	},
	{
		.cmd = SCID_GENL_CMD_EVENT_SNAPSHOT,
		.wrapper_handler = __scid_wrapper_event_snapshot,
	},
	{
		.cmd = SCID_GENL_CMD_GET_LAST_EVENTS,
		.wrapper_handler = __scid_wrapper_get_last_events,
	},
	{
		.cmd = SCID_GENL_CMD_IS_TRACKED_PAGE,
		.wrapper_handler = __scid_wrapper_is_tracked_page,
	},
	{
		.cmd = SCID_GENL_CMD_GET_ALL_TRACKED_PAGES,
		.wrapper_handler = __scid_wrapper_get_all_tracked_pages,
	},
	{
		.cmd = SCID_GENL_CMD_GET_ALL_TRACKED_WX_PAGES,
		.wrapper_handler = __scid_wrapper_get_all_tracked_wx_pages,
	},
	{
		.cmd = SCID_GENL_CMD_GET_ONE_LAST_EVENT,
		.wrapper_handler = __scid_wrapper_get_one_last_event,
	},
	{
		.cmd = SCID_GENL_CMD_GET_CUR_PAGE_SNAPSHOT,
		.wrapper_handler = __scid_wrapper_get_cur_page_snapshot,
	},
};

const uint8_t NR_STATIC_REGI_DEFS = __static_array_size(__static_regi_defs); 	
