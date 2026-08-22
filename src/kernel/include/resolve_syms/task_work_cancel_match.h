#ifndef SCID_RESOLVE_SYMS_TASK_WORK_CANCEL_MATCH_H
#define SCID_RESOLVE_SYMS_TASK_WORK_CANCEL_MATCH_H

#include <linux/sched.h>
#include <linux/types.h>

#include <resolve_syms.h>

#define task_work_cancel_match_SYMPAIR_INDEX 7

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(task_work_cancel_match),

		/* the fn return type */
		struct callback_head*
		, 

		/* symbol name to resolve */
		task_work_cancel_match
		,

		/* ... if symbol cannot be resolved */
		return NULL; 
		, 

		/* ... if symbol is resolved */
		return symaddr(tsk, match, data);
		,

		/* fn args */
		struct task_struct *tsk,
		bool (*match)(struct callback_head *, void *data),
		void *data
);

#endif
