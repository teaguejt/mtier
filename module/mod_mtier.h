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

#define PAGE_CACHE_SHIFT PAGE_SHIFT
#define MS_TO_NS(x) (x * 1E6L)
#define MTIER_INTERVAL_MS 1L

/* PTE-related macros */
/*#define PGD_SHIFT 39
#define PUD_SHIFT 30
#define PMD_SHIFT 21
#define PTRS_PER_PAGE 512*/

struct mtier_struct {
    int is_duplicated;      /* Has the page been duplicated? */
    int duplicate_used;     /* Is the duplicate page in use? */
    struct page *dpage;     /* The duplicated page */
    struct page *opage;     /* The original page */
    struct mm_struct *mm;   /* The process' mm_struct */
    pte_t *dpte;            /* PTE for duplicated page */
    pte_t *opte;            /* PTE for original page */
};

/* I've been having some issues including anything from the rmap header in
 * the module, so I'm duplicating the rmap_walk_control struct here. */
struct rmap_walk_control {
    void *arg;
    int (*rmap_one)(struct page *page, struct vm_area_struct *vma,
            unsigned long addr, void *arg);
    int (*done)(struct page *page);
    struct anon_vma *(*anon_lock)(struct page *page);
    bool (*invalid_vma)(struct vm_area_struct *vma, void *arg);
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
void mtier_flush_tlb_mm(struct mm_struct *);
void mtier_remove_migration_ptes(struct page *, struct page *);
int mtier_unmap_and_move(struct page *, struct page *, int, int);
int move_to_new_page(struct page *newpage, struct page *page,
				     unsigned int mode);
void __lock_page(struct page *);
void unlock_page(struct page *);
int migrate_page_move_mapping(struct address_space *mapping,
         struct page *newpage, struct page *page,
         struct buffer_head *head, enum migrate_mode mode,
         int extra_count);
int try_to_unmap(struct page *page, unsigned int flags);
int mtier_try_to_unmap_one(struct page *, struct vm_area_struct *,
        unsigned long, void *);
int mtier_page_not_mapped(struct page *);
struct anon_vma *page_lock_anon_vma_read(struct page *);
int mtier_trylock_page(struct page *);
#endif  /* CONFIG_MTIER */
#endif  /* _LINUX_MTIER_H */
