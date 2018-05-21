#ifndef _LINUX_MTIER_H
#define _LINUX_MTIER_H

#define MS_TO_NS(x) (x * 1E6L)
#define MTIER_INTERVAL_MS 1L

#include <linux/mempolicy.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/hugetlb.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/nodemask.h>
#include <linux/cpuset.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/export.h>
#include <linux/nsproxy.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/compat.h>
#include <linux/swap.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/migrate.h>
#include <linux/ksm.h>
#include <linux/rmap.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/ctype.h>
#include <linux/mm_inline.h>
#include <linux/mmu_notifier.h>
#include <linux/printk.h>

#include <linux/hashtable.h>
#include <linux/rbtree.h>
#include <linux/list.h>

#include <asm/tlbflush.h>
#include <asm/uaccess.h>
#include <linux/random.h>

#define MTIER_HT_BITS 8

#ifdef CONFIG_MTIER
#define VMA_IS_READONLY( vma ) \
  ( ( (unsigned long)(vma)->vm_flags & VM_READ ) && \
  !( (unsigned long)(vma)->vm_flags & VM_WRITE ) && \
  !( (unsigned long)(vma)->vm_flags & VM_SHARED ) )

extern struct list_head mtier_process_list;
extern struct hlist_head mtier_process_ht[1 << MTIER_HT_BITS];
struct mtier_enqueue_data {
    pid_t pid;
};

struct tierable_process {
    struct list_head list;
    struct task_struct *task;
    pid_t pid;
    struct hlist_node mtp;
    struct rb_root process_page_tree;
};

struct mtier_mapping_info {
  unsigned long vaddr;
  unsigned long pfn_slow;
  unsigned long pfn_fast;
  unsigned int is_mapped;
  unsigned int is_fast;
  struct rb_node page_tree;
};

struct mtier_mapping_info * mtier_new_mapping_info(unsigned long,
  struct tierable_process *);

int __mtier_insert_tree_node(struct rb_root *, struct mtier_mapping_info *);
struct mtier_mapping_info *__mtier_find_tree_node(struct rb_root *,
  unsigned long);

long mod_do_mtier_management(pid_t __user pid, unsigned long __user nPages,
        unsigned long __user *oldNode, unsigned long __user *newNode,
        unsigned long __user maxNode);

#endif  /* CONFIG_MTIER */
#endif  /* _LINUX_MTIER_H */
