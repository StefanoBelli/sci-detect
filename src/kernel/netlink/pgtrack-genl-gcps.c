#include <linux/compiler.h>

#include <user/scid-netlink-defs.h>
#include <netlink/pgtrack/cmds.h>
#include <pgtrack.h>
#include <pgsnap.h>
#include <netlink.h>
#include <logging.h>

/* populate_skb_with_page_snap prototype */
bool populate_skb_with_page_snap(const struct page_snap *, struct sk_buff*);

int pgtrack_genl_get_cur_page_snapshot_doit(
		__always_unused struct sk_buff *in_skb, struct genl_info *info)
{
	struct sk_buff *skb;
	void *hdr;
	unsigned long pfn;
	struct page_status *pgs;

	/* obtain new skb */
	skb = genlmsg_new(GENLMSG_DEFAULT_SIZE + PAGE_SIZE, GFP_KERNEL);
	if(!skb) {
		scid_err("unable to alloc new skb");
		return -ENOMEM;
	}

	/* put genl header */
	hdr = genlmsg_put(
			skb, info->snd_portid, info->snd_seq, 
			&genl_fam, 0, info->genlhdr->cmd);
	if(unlikely(!hdr)) {
		scid_err("unable to build hdr");
		goto __failure_nlfree;
	}

	/* obtain user-supplied pfn */
	if(unlikely(!info->attrs[SCID_GENL_ATTR_PFN])) {
		scid_warn("user didn't send pfn...");
		goto __failure_cancel_nlfree;
	}

	pfn = nla_get_u64(info->attrs[SCID_GENL_ATTR_PFN]);

	if(unlikely(nla_put_u64_64bit(skb, SCID_GENL_ATTR_PFN, 
					pfn, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put pfn in skb");
		goto __failure_cancel_nlfree;
	}

	/* define the RCU critical section 
	 * (rcu only, no need to get kref)
	 */
	rcu_read_lock();
	pgs = lookup_pfn_pgtrack(pfn);
	if(!pgs)
		goto __end_ok_rcu_unlock;

	spin_lock(&pgs->snapshot_lock);
	if(!pgs->snapshot)
		goto __end_ok_unlock_both;

	if(!populate_skb_with_page_snap(pgs->snapshot, skb))
		goto __failure_unlock_both;

__end_ok_unlock_both:
	spin_unlock(&pgs->snapshot_lock);
__end_ok_rcu_unlock:
	rcu_read_unlock();

	if(unlikely(nla_put_u32(skb, SCID_GENL_ATTR_PFN_FOUND, !!pgs))) {
		scid_err("unable to put pfn found");
		goto __failure_cancel_nlfree;
	}

	genlmsg_end(skb, hdr);
	return genlmsg_reply(skb, info);

__failure_unlock_both:
	spin_unlock(&pgs->snapshot_lock);
	rcu_read_unlock();
__failure_cancel_nlfree:
	genlmsg_cancel(skb, hdr);
__failure_nlfree:
	nlmsg_free(skb);
	return -EMSGSIZE;
}
