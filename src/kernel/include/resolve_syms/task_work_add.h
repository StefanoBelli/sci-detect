#ifndef SCID_RESOLVE_SYMS_TASK_WORK_ADD_H
#define SCID_RESOLVE_SYMS_TASK_WORK_ADD_H

#include <linux/task_work.h>
#include <resolve_syms.h>

#define task_work_add_SYMPAIR_INDEX 6

DEFINE_RESOLVED_THUNK
(
 		/* index in sym table */
		sympair_nr(task_work_add),

		/* the fn return type */
		int
		, 

		/* symbol name to resolve */
		task_work_add
		,

		/* ... if symbol cannot be resolved */
		return 1; 
		, 

		/* ... if symbol is resolved */
		return symaddr(tsk, work, notify);
		,

		/* fn args */
		struct task_struct *tsk,
		struct callback_head *work,
		enum task_work_notify_mode notify
);

#endif
