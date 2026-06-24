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
};

/*
 * response wrappers
 */

/* wxwarning event (broadcasted) */

int __scid_wrapper_event_wxwarning(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	struct wxwarning_event evt;

	memset(&evt, 0, sizeof(evt));

	if(attrs[SCID_GENL_ATTR_PID])
		evt.pid = nla_get_s32(attrs[SCID_GENL_ATTR_PID]);

	if(attrs[SCID_GENL_ATTR_PFN])
		evt.pfn = nla_get_u64(attrs[SCID_GENL_ATTR_PFN]);

	if(attrs[SCID_GENL_ATTR_VA])
		evt.va = nla_get_u64(attrs[SCID_GENL_ATTR_VA]);

	user_handler(&evt, uargs);

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
		wxw->va = nla_get_uint(attr);
	else if(nla_type(attr) == SCID_GENL_ATTR_PID)
		wxw->pid = nla_get_s32(attr);
	else if(nla_type(attr) == SCID_GENL_ATTR_PFN)
		wxw->pfn = nla_get_uint(attr);

	return NL_OK;
}

int __scid_wrapper_get_last_events(cmd_handler_fpt user_handler,
		struct nlattr **attrs, void *uargs)
{
	int rv = NL_OK;
	int rem;
	struct nlattr *pos;
	struct all_last_events all_evts;

	memset(&all_evts, 0, sizeof(all_evts));

	if(!attrs[SCID_GENL_ATTR_ARRAY_NR_ELEMS])
		return NL_SKIP;

	all_evts.nr = nla_get_u32(attrs[SCID_GENL_ATTR_ARRAY_NR_ELEMS]);

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
		.cmd = SCID_GENL_CMD_GET_LAST_EVENTS,
		.wrapper_handler = __scid_wrapper_get_last_events,
	}
};

const uint8_t NR_STATIC_REGI_DEFS = __static_array_size(__static_regi_defs); 	
