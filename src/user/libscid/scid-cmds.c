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

