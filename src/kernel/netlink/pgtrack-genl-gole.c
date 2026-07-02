#include <netlink/pgtrack/cmds.h>
#include <user/scid-netlink-defs.h>
#include <logging.h>
#include <netlink.h>

#include "pgtrack-genl-events.h"

static int __do_get_one_last_event(struct sk_buff *skb, u32 idx)
{
	int rv = 0;

	down_read(&le_lock);

	unsigned int fifo_len = __fifo_len();
	if(idx >= fifo_len)
		goto __unlock;

    struct event **evts = kmalloc(__fifo_bytes(fifo_len), GFP_KERNEL);
    if(!evts) {
    	scid_err("memory exhausted");
    	rv = -ENOMEM;
    	goto __unlock;
    }

	if(unlikely(!kfifo_out_peek(&le, evts, __fifo_bytes(fifo_len)))) {
    	scid_warn("why kfifo_out_peek is 0???");
    	rv = -EBUSY;
    	goto __free_unlock;
    }

    if(unlikely(nla_put_u32(skb, SCID_GENL_ATTR_EVT_TYPE, evts[idx]->type))) {
    	scid_err("unable to put evt type in skb");
    	rv = -EMSGSIZE;
    	goto __free_unlock;
    }

    if(unlikely(!event_to_populate_skb_with(evts[idx], skb, EVENT_TO_SKB_PEEKONE))) {
    	scid_err("unable to populate skb");
    	rv = -EMSGSIZE;
    }

__free_unlock:
	kfree(evts);
__unlock:
	up_read(&le_lock);
	return rv;
}

int pgtrack_genl_get_one_last_event_doit(
		__always_unused struct sk_buff *in_skb, struct genl_info *info)
{
	int err_rv = -EMSGSIZE;
	u32 in_idx;
	struct sk_buff *skb;
	void *hdr;

	skb = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if(!skb) {
		scid_err("unable to get new skb");
		return -ENOMEM;
	}

	hdr = genlmsg_put(skb, info->snd_portid, info->snd_seq, &genl_fam, 0, info->genlhdr->cmd);
	if(!hdr) {
		scid_err("unable to build hdr");
		goto __failure_nlfree;
	}

	if(!info->attrs[SCID_GENL_ATTR_GENIDX]) {
		scid_warn("get_one_last_event without idx");
		err_rv = -ENOKEY;
		goto __failure_cancel_nlfree;
	}

	in_idx = nla_get_u32(info->attrs[SCID_GENL_ATTR_GENIDX]);

	err_rv = __do_get_one_last_event(skb, in_idx);
	if(err_rv)
		goto __failure_cancel_nlfree;

	genlmsg_end(skb, hdr);
	return genlmsg_reply(skb, info);

__failure_cancel_nlfree:
	genlmsg_cancel(skb, hdr);
__failure_nlfree:
	nlmsg_free(skb);
	return err_rv;
}
