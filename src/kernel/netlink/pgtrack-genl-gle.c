#include <linux/compiler.h>
#include <linux/slab.h>

#include <netlink.h>
#include <logging.h>
#include <netlink/pgtrack/cmds.h>
#include <user/scid-netlink-defs.h>

#include "pgtrack-genl-events.h"

int pgtrack_genl_get_last_events_doit(
		__always_unused struct sk_buff *in_skb, struct genl_info *info)
{
	struct sk_buff *skb;
	void *hdr;
	struct nlattr *nest;

	/* new genl skb */
	skb = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if(!skb) {
		scid_err("unable to get new skb");
		return -ENOMEM;
	}

	/* put header */
	hdr = genlmsg_put(skb, info->snd_portid, info->snd_seq, &genl_fam, 0, info->genlhdr->cmd);
    if (!hdr) {
        scid_err("unable to build hdr");
        goto __failure_nlfree;
    }

    /* take the fifo lock */
    down_read(&le_lock);

    /* always send out an array */
    unsigned int fifo_len = __fifo_len();

    /* put the nr elems of array prior */
    if(unlikely(nla_put_u32(skb, SCID_GENL_ATTR_ARRAY_NR_ELEMS, fifo_len))) {
    	scid_err("unable to put nr_elems");
    	goto __failure_unlock_cancel_nlfree;
    }

    /* put the nesting for the array, app will know how to 
     * rebuild everything */
    nest = nla_nest_start(skb, SCID_GENL_ATTR_ARRAY);
    if(!nest) {
    	scid_err("unable to create nesting");
    	goto __failure_unlock_cancel_nlfree;
    }

    /* if the fifo is empty... */
    if(!fifo_len)
    	goto __end_ok;

    struct event **evts = kmalloc(__fifo_bytes(fifo_len), GFP_KERNEL);
    if(!evts) {
    	scid_err("memory exhausted");
    	goto __failure_endnest_unlock_cancel_nlfree;
    }

    if(unlikely(!kfifo_out_peek(&le, evts, __fifo_bytes(fifo_len))))
    	scid_warn("why kfifo_out_peek is 0???");
    else {
    	for(unsigned int i = 0; i < fifo_len; i++) {
    		if(unlikely(nla_put_u32(skb, SCID_GENL_ATTR_EVT_TYPE, evts[i]->type))) {
    			scid_err("unable to put evt type in skb");
    			kfree(evts);
    			goto __failure_endnest_unlock_cancel_nlfree;
    		}

    		if(unlikely(!event_to_populate_skb_with(evts[i], skb, EVENT_TO_SKB_GET_EVTS))) {
    			scid_err("unable to populate skb");
    			kfree(evts);
    			goto __failure_endnest_unlock_cancel_nlfree;
    		}
    	}
    }

    kfree(evts);

    /* path taken when either the array is empty or not, in any case
     * everything ok! */
__end_ok:
	up_read(&le_lock);
    nla_nest_end(skb, nest);
    genlmsg_end(skb, hdr);
    return genlmsg_reply(skb, info);

    /* path taken when something goes wrong... (e.g. skb is full) */
__failure_endnest_unlock_cancel_nlfree:
	nla_nest_end(skb, nest);
__failure_unlock_cancel_nlfree:
	up_read(&le_lock);
	genlmsg_cancel(skb, hdr);
__failure_nlfree:
	nlmsg_free(skb);
	return -EMSGSIZE;
}
