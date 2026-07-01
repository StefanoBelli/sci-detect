#include <linux/rmap.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>

#include <resolve_syms/rmap_walk.h>
#include <netlink/pgtrack/cmds.h>
#include <user/scid-netlink-defs.h>
#include <pgtrack.h>
#include <netlink.h>
#include <logging.h>

static bool __do_is_tracked_page_perms(struct sk_buff *skb, struct page_status *pgs)
{
	perm_type perm = atomic64_read(&pgs->perms);

	s32 writable = perm & PERM_WRITE_BIT;
	s32 executable = perm & PERM_EXEC_BIT;

	if(unlikely(nla_put_s32(
					skb, SCID_GENL_ATTR_PAGE_WRITABLE, writable))) {
		scid_err("unable to put writable in skb");
		return false;
	}

	if(unlikely(nla_put_s32(
					skb, SCID_GENL_ATTR_PAGE_EXECUTABLE, executable))) {
		scid_err("unable to put writable in skb");
		return false;
	}

	return true;
}

static bool __do_is_tracked_page_found(struct sk_buff *skb, u32 pfn, u32 found)
{
	if(unlikely(nla_put_u64_64bit(
					skb, SCID_GENL_ATTR_PFN, pfn, SCID_GENL_ATTR_PAD))) {
		scid_err("unable to put pfn in skb");
		return false;
	}

	if(unlikely(nla_put_u32(
					skb, SCID_GENL_ATTR_PFN_FOUND, found))) {
		scid_err("unable to put pfn found in skb");
		return false;
	}

	return true;
}

struct __ditp_rmap_args {
	/* input */
	struct sk_buff *skb;

	/* output from cb */
	bool err;
	uint32_t nr_pids;
};

static bool __do_is_tracked_page_rmap_one(
		__always_unused struct folio *folio, struct vm_area_struct *vma, 
		__always_unused unsigned long addr, void *__arg)
{
	struct __ditp_rmap_args *args = __arg;
	struct task_struct *tsk;

	/* for each task in the system... */
	for_each_process(tsk) {

		/* ...match all whose mm is the same as this VMA */
		if(vma->vm_mm == tsk->mm) {
			if(unlikely(nla_put_s32(args->skb, SCID_GENL_ATTR_PID, task_pid_vnr(tsk)))) {
				scid_err("unable to put pid in skb");
				args->err = true;
				return false;
			}

			args->nr_pids++;

			/* don't break, other tasks may point to the same mm */
		}
	}

	return true;
}

static bool __do_is_tracked_page_pids(struct sk_buff *skb, struct page_status *pgs)
{
	struct rmap_walk_control rwc;
	struct __ditp_rmap_args args;
	struct nlattr *nest;

	memset(&args, 0, sizeof(args));
	args.skb = skb;

	memset(&rwc, 0, sizeof(rwc));
	rwc.rmap_one = __do_is_tracked_page_rmap_one;
	rwc.arg = &args;

	struct folio *folio = page_folio(pgs->page);

	folio_lock(folio);

	nest = nla_nest_start(skb, SCID_GENL_ATTR_ARRAY);

	if(!folio_mapped(folio))
		goto __endnest_unlock;

	THUNK(rmap_walk)(folio, &rwc);

__endnest_unlock:
	nla_nest_end(skb, nest);

	if(!args.err && unlikely(nla_put_u32(
					skb, SCID_GENL_ATTR_ARRAY_NR_ELEMS, args.nr_pids))) {
		scid_err("unable to put array's nr_elems");
		args.err = true;
	}

	folio_unlock(folio);
	return !args.err;
}

static int __do_is_tracked_page(
		struct genl_info *info, u64 pfn, u32 found, struct page_status *pgs)
{
	struct sk_buff *skb;
	void *hdr;

	skb = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if(unlikely(!skb)) {
		scid_err("unable to get new skb");
		return -ENOMEM;
	}

	hdr = genlmsg_put(skb, info->snd_portid, info->snd_seq, &genl_fam, 0, info->genlhdr->cmd);
	if(unlikely(!hdr)) {
		scid_err("unable to build hdr");
		goto __failure_nlfree;
	}

	if(unlikely(!__do_is_tracked_page_found(skb, pfn, found)))
		goto __failure_cancel_nlfree;

	if(!found)
		goto __end_ok;

	if(unlikely(!__do_is_tracked_page_perms(skb, pgs)))
		goto __failure_cancel_nlfree;

	if(unlikely(!__do_is_tracked_page_pids(skb, pgs)))
		goto __failure_cancel_nlfree;

__end_ok:
	genlmsg_end(skb, hdr);
	return genlmsg_reply(skb, info);

__failure_cancel_nlfree:
	genlmsg_cancel(skb, hdr);
__failure_nlfree:
	nlmsg_free(skb);
	return -EMSGSIZE;
}

int pgtrack_genl_is_tracked_page_doit(
		__always_unused struct sk_buff *in_skb, struct genl_info *info)
{
	struct nlattr *in_pfn_attr = info->attrs[SCID_GENL_ATTR_PFN];
	u64 in_pfn;
	int rv;

	if(in_pfn_attr)
		in_pfn = nla_get_u64(in_pfn_attr);
	else {
		scid_warn("is_tracked_page cmd without pfn as input :/");
		return 0;
	}

	rcu_read_lock();
	struct page_status *pgs = lookup_pfn_pgtrack(in_pfn);
	bool invalid = !pgs || !try_page_status_get(pgs);
	rcu_read_unlock();

	rv = __do_is_tracked_page(info, in_pfn, !invalid, pgs);

	if(likely(!invalid))
		page_status_put(pgs);

	return rv;
}
