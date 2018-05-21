#ifndef _LINUX_MTIER_H
#define _LINUX_MTIER_H

#ifdef CONFIG_MTIER 

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/timekeeping.h>
#include <linux/list.h>
//#include <linux/nodemask.h>
#include <asm-generic/uaccess.h>
#define MS_TO_NS(x) (x * 1E6L)
#define MTIER_INTERVAL_MS 1L

struct mtier_struct {
    int is_duplicated;      /* Has the page been duplicated? */
    int duplicate_used;     /* Is the duplicate page in use? */
    struct page *dpage;     /* The duplicated page */
    struct page *opage;     /* The original page */
    struct mm_struct *mm;   /* The process' mm_struct */
    pte_t *dpte;            /* PTE for duplicated page */
    pte_t *opte;            /* PTE for original page */
};


long mod_do_mtier_management(pid_t __user pid, unsigned long __user nPages,
        unsigned long __user *oldNode, unsigned long __user *newNode,
        unsigned long __user maxNode);

struct list_head *mtier_get_ro_pagelist(struct mm_struct *mm, unsigned long slow, 
        unsigned long fast, struct list_head *pl);

struct page *alloc_page_interleave(gfp_t *gfp, unsigned order, unsigned nid);
void __free_pages(struct page *page, unsigned int order);

bool mtier_page_is_young(struct page *);
void mtier_set_page_young(struct page *);
bool mtier_page_is_idle(struct page *);
void mtier_set_page_idle(struct page *);
void ksm_migrate_page(struct page *, struct page *);
void end_page_writeback(struct page *);
void putback_movable_pages(struct list_head *);

#endif  /* CONFIG_MTIER */
#endif  /* _LINUX_MTIER_H */
