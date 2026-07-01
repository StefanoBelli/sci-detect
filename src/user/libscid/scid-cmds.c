#include "scid-internals.h"

/* 
 * command senders
 */

/* get_last_events cmd */
long scid_cmd_get_last_events(void *desc, void *args)
{
	return __scid_send_cmd(
			desc, SCID_GENL_CMD_GET_LAST_EVENTS, args, 
			NULL, NULL);
}

/* is_tracked_page cmd */
static long __itp_in_attrs_add_cb(struct nl_msg *msg, const void *args)
{
	return nla_put_u64(msg, SCID_GENL_ATTR_PFN, (unsigned long) args);
}

long scid_cmd_is_tracked_page(void *desc, void *args, unsigned long pfn)
{
	return __scid_send_cmd(
			desc, SCID_GENL_CMD_IS_TRACKED_PAGE, args,
			__itp_in_attrs_add_cb, (const void*) pfn);
}

/* get_all_tracked_pages cmd */
long scid_cmd_get_all_tracked_pages(void *desc, void *args)
{
	return __scid_send_dump_cmd(
			desc, SCID_GENL_CMD_GET_ALL_TRACKED_PAGES, args,
			NULL, NULL);
}

/* get_all_tracked_wx_pages cmd */
long scid_cmd_get_all_tracked_wx_pages(void *desc, void *args)
{
	return __scid_send_dump_cmd(
			desc, SCID_GENL_CMD_GET_ALL_TRACKED_WX_PAGES, args,
			NULL, NULL);
}

/* get_one_last_event cmd */
static long __gole_in_attrs_add_cb(struct nl_msg *msg, const void *args)
{
	return nla_put_u32(msg, SCID_GENL_ATTR_GENIDX, (uint32_t) args);
}

long scid_cmd_get_one_last_event(void *desc, void *args, uint32_t idx)
{
	return __scid_send_cmd(
			desc, SCID_GENL_CMD_GET_ONE_LAST_EVENT, args,
			__gole_in_attrs_add_cb, (const void*) idx);
}

/* get_cur_page_snapshot cmd */
static long __gcps_in_attrs_add_cb(struct nl_msg *msg, const void *args)
{
	return nla_put_u64(msg, SCID_GENL_ATTR_PFN, (unsigned long) args);
}

long scid_cmd_get_cur_page_snapshot(void *desc, void *args, unsigned long pfn)
{
	return __scid_send_cmd(
			desc, SCID_GENL_CMD_GET_CUR_PAGE_SNAPSHOT, args,
			__gcps_in_attrs_add_cb, (const void*) pfn);
}
