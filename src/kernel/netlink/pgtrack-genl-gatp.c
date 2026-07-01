#include <netlink/pgtrack/cmds.h>
#include <user/scid-netlink-defs.h>
#include <pgtrack.h>
#include <netlink.h>

struct foreach_pfn_args {
	struct sk_buff *skb;
	struct netlink_callback *nlcb;
	unsigned long next_starting_pfn;
	u8 cmd;
};

typedef void (*foreach_pgtrack_fn)(
		unsigned long start, 
		foreach_pfn_cb cb, 
		void *args);

static bool __pfn_iter(
		unsigned long pfn, 
		__always_unused struct page_status *pgs, 
		void *__args)
{
	struct foreach_pfn_args *args = __args;
	void *hdr;
	struct sk_buff *skb = args->skb;
	struct netlink_callback *nlcb = args->nlcb;
	u8 cmd = args->cmd;

	hdr = genlmsg_put(
			skb, NETLINK_CB(skb).portid, nlcb->nlh->nlmsg_seq, 
			&genl_fam, NLM_F_MULTI, cmd);
	if(!hdr)
		return false;

	if(unlikely(nla_put_u64_64bit(
					skb, SCID_GENL_ATTR_PFN, pfn, SCID_GENL_ATTR_PAD))) {
		genlmsg_cancel(skb, hdr);
		return false;
	}

	genlmsg_end(skb, hdr);

	args->next_starting_pfn = pfn + 1;
	return true;
}

static int __get_all_tracked_pages_dump(
		foreach_pgtrack_fn foreach_pgtrack, 
		struct sk_buff *skb, 
		struct netlink_callback *nlcb,
		u8 cmd)
{
	struct foreach_pfn_args args = {
		.next_starting_pfn = nlcb->args[0],
		.skb = skb,
		.nlcb = nlcb,
		.cmd = cmd,
	};

	foreach_pgtrack(nlcb->args[0], __pfn_iter, &args);

	nlcb->args[0] = args.next_starting_pfn;
	return skb->len;
}

int pgtrack_genl_get_all_tracked_pages_dumpit(
		struct sk_buff *skb, struct netlink_callback *nlcb)
{
	return __get_all_tracked_pages_dump(
			foreach_pfn_pgtrack, skb, nlcb, 
			SCID_GENL_CMD_GET_ALL_TRACKED_PAGES);
}

int pgtrack_genl_get_all_tracked_wx_pages_dumpit(
		struct sk_buff *skb, struct netlink_callback *nlcb)
{
	return __get_all_tracked_pages_dump(
			foreach_bad_pfn_pgtrack, skb, nlcb, 
			SCID_GENL_CMD_GET_ALL_TRACKED_WX_PAGES);
}
