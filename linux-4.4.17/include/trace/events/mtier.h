#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtier

#if !defined(_TRACE_MTIER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MTIER_H

#include <linux/tracepoint.h>
#include <linux/binfmts.h>
#include <linux/timekeeping.h>
#include <linux/nodemask.h>

/*
 * Tracepoints for calling mod_do_mtier_management -- 
 */
TRACE_EVENT(entry_do_mtier_management,

    TP_PROTO(pid_t pid, unsigned long npages, nodemask_t *oldNode,
        nodemask_t *newNode, unsigned long maxNodes, ktime_t start),

    TP_ARGS(pid, npages, oldNode, newNode, maxNodes, start),

    TP_STRUCT__entry(
        __field(    pid_t,         pid         )
        __field(    unsigned long, npages      )
        __field(    unsigned long, oldNode     )
        __field(    unsigned long, newNode     )
        __field(    unsigned long, maxNodes    )
        __field(    unsigned long, time        )
    ),

    TP_fast_assign(
        __entry->pid      = pid;
        __entry->npages   = npages;
        __entry->oldNode  = *(unsigned long *)oldNode;
        __entry->newNode  = *(unsigned long *)newNode;
        __entry->maxNodes = maxNodes;
        __entry->time     = (unsigned long)start.tv64;
    ),

    TP_printk("pid=%lu pgtgt=%lu old=%lu new=%lu max=%lu time=%lu",
            (unsigned long)__entry->pid, __entry->npages, __entry->oldNode,
            __entry->newNode, __entry->maxNodes, __entry->time)

);

TRACE_EVENT(exit_do_mtier_management,

    TP_PROTO(pid_t pid, unsigned long pgact, ktime_t end),

    TP_ARGS(pid, pgact, end),

    TP_STRUCT__entry(
        __field(    pid_t,         pid      )
        __field(    unsigned long, pgact    )
        __field(    unsigned long, end      )
    ),

    TP_fast_assign(
        __entry->pid   = pid;
        __entry->pgact = pgact;
        __entry->end   = (unsigned long)end.tv64;
    ),

    TP_printk("pid=%lu pgact=%lu time=%lu", (unsigned long)__entry->pid,
        __entry->pgact, __entry->end)

);

TRACE_EVENT(enter_mtier_queue_pages_range,

    TP_PROTO(ktime_t start),

    TP_ARGS(start),

    TP_STRUCT__entry(
        __field(    unsigned long, start    )
    ),

    TP_fast_assign(
        __entry->start = (unsigned long)start.tv64;
    ),

    TP_printk("time=%lu", __entry->start)
);

TRACE_EVENT(exit_mtier_queue_pages_range,

    TP_PROTO(ktime_t end),

    TP_ARGS(end),

    TP_STRUCT__entry(
        __field(    unsigned long, end    )
    ),

    TP_fast_assign(
        __entry->end = (unsigned long)end.tv64;
    ),

    TP_printk("time=%lu", __entry->end)
);

TRACE_EVENT(enter_mtier_shrink_pagelist,

    TP_PROTO(ktime_t start),

    TP_ARGS(start),

    TP_STRUCT__entry(
        __field(    unsigned long, start    )
    ),

    TP_fast_assign(
        __entry->start = (unsigned long)start.tv64;
    ),

    TP_printk("time=%lu", __entry->start)
);

TRACE_EVENT(exit_mtier_shrink_pagelist,

    TP_PROTO(ktime_t end),

    TP_ARGS(end),

    TP_STRUCT__entry(
        __field(    unsigned long, end    )
    ),

    TP_fast_assign(
        __entry->end = (unsigned long)end.tv64;
    ),

    TP_printk("time=%lu", __entry->end)
);

TRACE_EVENT(enter_migrate_pages,

    TP_PROTO(ktime_t start),

    TP_ARGS(start),

    TP_STRUCT__entry(
        __field(    unsigned long, start    )
    ),

    TP_fast_assign(
        __entry->start = (unsigned long)start.tv64;
    ),

    TP_printk("time=%lu", __entry->start)
);

TRACE_EVENT(exit_migrate_pages,

    TP_PROTO(ktime_t end),

    TP_ARGS(end),

    TP_STRUCT__entry(
        __field(    unsigned long, end    )
    ),

    TP_fast_assign(
        __entry->end = (unsigned long)end.tv64;
    ),

    TP_printk("time=%lu", __entry->end)
);

TRACE_EVENT(unmap_and_move,

    TP_PROTO(ktime_t start, ktime_t end),

    TP_ARGS(start, end),

    TP_STRUCT__entry(
        __field(    unsigned long, duration    )
    ),

    TP_fast_assign(
        __entry->duration = (unsigned long)(end.tv64 - start.tv64);
    ),

    TP_printk("dur=%lu", __entry->duration)
);

TRACE_EVENT(unmap_and_move_huge,

    TP_PROTO(ktime_t start, ktime_t end),

    TP_ARGS(start, end),

    TP_STRUCT__entry(
        __field(    unsigned long, duration    )
    ),

    TP_fast_assign(
        __entry->duration = (unsigned long)(end.tv64 - start.tv64);
    ),

    TP_printk("dur=%lu", __entry->duration)
);

#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(bool preempt, struct task_struct *p)
{
#ifdef CONFIG_SCHED_DEBUG
    BUG_ON(p != current);
#endif
    return preempt ? TASK_RUNNING | TASK_STATE_MAX : p->state;
}
#endif

TRACE_EVENT(mtier_sched_switch,

	TP_PROTO(bool preempt,
		 struct task_struct *prev,
		 struct task_struct *next,
         ktime_t time),

	TP_ARGS(preempt, prev, next, time),

	TP_STRUCT__entry(
		__array(	char,	       prev_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	       prev_pid	              		)
		__field(	int,	       prev_prio	         		)
		__field(	long,	       prev_state	            	)
		__array(	char,	       next_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	       next_pid	            		)
		__field(	int,	       next_prio	        		)
        __field(    unsigned long, time                         )
	),

	TP_fast_assign(
		memcpy(__entry->next_comm, next->comm, TASK_COMM_LEN);
		__entry->prev_pid	= prev->pid;
		__entry->prev_prio	= prev->prio;
		__entry->prev_state	= __trace_sched_switch_state(preempt, prev);
		memcpy(__entry->prev_comm, prev->comm, TASK_COMM_LEN);
		__entry->next_pid	= next->pid;
		__entry->next_prio	= next->prio;
        __entry->time       = (unsigned long)time.tv64;
	),

	TP_printk("sched_switch prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d time=%lu",
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,
		__entry->prev_state & (TASK_STATE_MAX-1) ?
		  __print_flags(__entry->prev_state & (TASK_STATE_MAX-1), "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
				{ 128, "K" }, { 256, "W" }, { 512, "P" },
				{ 1024, "N" }) : "R",
		__entry->prev_state & TASK_STATE_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio, 
        __entry->time)
);

TRACE_EVENT(mtier_pte_offset_map_lock,

    TP_PROTO(int lock, ktime_t time),

    TP_ARGS(lock, time),

    TP_STRUCT__entry(
        __field(              int, locked    )
        __field(    unsigned long, time      )
    ),

    TP_fast_assign(
        __entry->locked = lock;
        __entry->time   = (unsigned long)time.tv64;
    ),

    TP_printk("fn=mtier_queue_pages_pte_range lock=%d time=%lu", __entry->locked,
        __entry->time)
);

TRACE_EVENT(mtier_mmap_sem_read,

    TP_PROTO(int lock, ktime_t time),

    TP_ARGS(lock, time),

    TP_STRUCT__entry(
        __field(              int, locked    )
        __field(    unsigned long, time      )
    ),

    TP_fast_assign(
        __entry->locked = lock;
        __entry->time   = (unsigned long)time.tv64;
    ),

    TP_printk("fn=mtier_do_migrate_pages lock=%d time=%lu", __entry->locked,
        __entry->time)
);

TRACE_EVENT(mtier_rcu_read_lock,

    TP_PROTO(int lock, int err, ktime_t time),

    TP_ARGS(lock, err, time),

    TP_STRUCT__entry(
        __field(              int, locked    )
        __field(              int, err       )
        __field(    unsigned long, time      )
    ),

    TP_fast_assign(
        __entry->locked = lock;
        __entry->err    = err;
        __entry->time   = (unsigned long)time.tv64;
    ),

    TP_printk("fn=mtier_migrate_pages lock=%d err=%d time=%lu", 
        __entry->locked, __entry->err, __entry->time)
);

TRACE_EVENT(mtier_try_to_unmap_page,

    TP_PROTO(ktime_t time, struct page *p, struct page *np),

    TP_ARGS(time, p, np),

    TP_STRUCT__entry(
        __field(    unsigned long, time       )
        __field(    struct page *, page       )
        __field(    struct page *, newpage    )
    ),

    TP_fast_assign(
        __entry->time = (unsigned long)time.tv64;
        __entry->page = p;
        __entry->newpage = np;
    ),

    TP_printk("time=%lu", __entry->time)
);

#endif
#include <trace/define_trace.h>
