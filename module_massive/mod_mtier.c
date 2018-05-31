#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/timekeeping.h>
#include <linux/security.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/page-flags.h>
#include <linux/vmalloc.h>
#include <linux/random.h>
#include <linux/hashtable.h>
#include <linux/semaphore.h>
//#include <linux/nodemask.h>
//#include <asm-generic/memory_model.h>
#include <asm/page.h>
#include <asm/bitops.h>
#include <asm/tlbflush.h>
#include "mod_mtier.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph Teague");
MODULE_DESCRIPTION("The module for the mtier kthread worker");

/* Debug output because, let's face it, we need this for tasks of this
 * complexity. */
//#define MTIER_DEBUG
#ifdef MTIER_DEBUG
#define DEBUG_CODE(x)   x
#else
#define DEBUG_CODE(x)
#endif

#define MTIER_TIMING

//#define SIZE_MB 3000        /* Used for testing 100% migration with STREAM */
//#define SIZE_MB 1024  /* Standard size > 100% STREAM RO RSS */
#define SIZE_MB 92      /* 12.5% STREAM RO RSS */
//#define SIZE_MB 2         /* Fun size, for testing */
#define ENTRY_NAME  "mod_mtier"
#define PROCFS_NAME "mod_mtier"
#define PROCFS_SIZE 16<<10
#define PERMS 0644
#define PARENT NULL
#define PAGES(x) (((x) << 20) / 4096)
#define MTIER_HASH_BITS 24
#define MIN2(x,y) ( (x < y) ? x : y )
#define MTIER_HASH_KEY(x) ((unsigned long)((unsigned long)x >> PAGE_SHIFT))

/* Apparently I have to include these two definitions here, since trying to
 * include rmap.h or pagemap.h in this module causes it to no longer build
 * with a phone-book of error messages. */
struct anon_vma {
    struct anon_vma *root;      /* Root of this anon_vma tree */
    struct rw_semaphore rwsem;  /* W: modification, R: walking the list */
    /*
     * The refcount is taken on an anon_vma when there is no
     * guarantee that the vma of page tables will exist for the
     * duration of the operation. A caller that takes the reference
     * is responsible for clearing up the anon_vma if they are the
     * last user on release
     */
    atomic_t refcount;

    /*
     * Count of child anon_vmas and VMAs which points to this
     * anon_vma.  This counter is used for making decision about
     * reusing anon_vma instead of forking new one. See comments in
     * function anon_vma_clone.
     */
    unsigned degree;

    struct anon_vma *parent;    /* Parent of this anon_vma */

    /*
     * NOTE: the LSB of the rb_root.rb_node is set by
     * mm_take_all_locks() _after_ taking the above lock. So the
     * rb_root must only be read/written after taking the above lock
     * to be sure to see a valid next pointer. The LSB bit itself is
     * serialized by a system wide lock only visible to
     * mm_take_all_locks() (mm_all_locks_mutex).
     */
    struct rb_root rb_root; /* Interval tree of private "related" vmas */
};

struct anon_vma_chain {
    struct vm_area_struct *vma;
    struct anon_vma *anon_vma;
    struct list_head same_vma;   /* locked by mmap_sem & page_table_lock */
    struct rb_node rb;          /* locked by anon_vma->rwsem */
    unsigned long rb_subtree_last;
#ifdef CONFIG_DEBUG_VM_RB
    unsigned long cached_vma_start, cached_vma_last;
#endif
};

struct tier_list_struct {
    pid_t pid;
    pte_t pte;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
};

/* A struct to store information about a tier entry. This is needed for fast
 * swap-back from the fast tier to the slow tier. */
struct tier_struct {
    pid_t owner;
    int fast_in_use;
    int fast_valid;
    unsigned long fast_pfn;
    unsigned long fast_vaddr;
    unsigned long slow_pfn;
    unsigned long slow_vaddr;
    unsigned long usr_vaddr;
    pte_t fast_pte;
    pte_t slow_pte;
    pte_t *pte;
    spinlock_t *ptl;
    struct page *slow_page;
    struct page *fast_page;
    struct hlist_node hl_node;
    struct list_head list;
};

/* User-specified variables:
 * ft_size_mb: the size of the fast tier, up to a max of module claimed pages
 * ft_frac: fraction of fast tier to fill each iter (ft_size_mb / ft_fraction)
 * ft_delay: delay between module iterations in milliseconds
 * ft_bk_iters: how often bookkeeping/list rebuilding will be performed
 *
 * The module will claim SIZE_MB pages when it starts; these have to be 
 * statically allocated AND contiguous; vmalloc() doesn't guarantee 
 * contiguousness and kmalloc() can't handle the large sizes. We restrict this
 * with ft_size_mb, which has a MAXIMUM of SIZE_MB. Otherwise, the module will
 * only use what's specified by the user. Unfortunately, there's not a better
 * way to do this given the kernel's allocation functions' limitations.
 */
static unsigned long ft_size_mb = 0;
module_param(ft_size_mb, ulong, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
static unsigned long ft_fraction = 1;
module_param(ft_fraction, ulong, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
static unsigned long ft_delay = 1000;
module_param(ft_delay, ulong, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
static unsigned long ft_bk_iters = 10;
module_param(ft_bk_iters, ulong, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
static unsigned long ft_size_pages = 0;

/* User-configurable node information */
static unsigned long fast_node = 1;
module_param(fast_node, ulong, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
static unsigned long slow_node = 0;
module_param(slow_node, ulong, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

static struct task_struct *kthread;
static int counter;
static int mod_iter   = 0;
static int tot_iter   = 0;
static int pagecount  = 0;
int moved_once = 0;
//static int pers_node = 2;    /* Unused, for now */
struct tier_struct tier_entries[(SIZE_MB << 20) / PAGE_SIZE];
static struct list_head *pagelist;
static struct list_head *freelist;
static struct list_head *usedlist;
static struct page **shuffled_pages;
DEFINE_HASHTABLE(side_tbl, MTIER_HASH_BITS);

void print_tier_struct_info(struct tier_struct *ts) {
    printk("=== TIER STRUCT DEBUG PRINT ===\n");
    if(!ts) {
        printk("\t!ts\n");
        return;
    }
    printk("\tpid = %d\n", (int)ts->owner);
    printk("\tfast_in_use = %d, valid = %d\n", ts->fast_in_use,
            ts->fast_valid);
    printk("\tfast pfn = 0x%lx, fast vaddr = 0x%lx\n", ts->fast_pfn,
            ts->fast_vaddr);
    printk("\tslow pfn = 0x%lx, slow vaddr = 0x%lx\n", ts->slow_pfn,
            ts->slow_vaddr);
    printk("\tfast pte val = 0x%lx, slow pte val = 0x%lx\n",
            (unsigned long)ts->fast_pte.pte, (unsigned long)ts->slow_pte.pte);
    if(!ts->pte) {
        printk("\t!pte\n");
    }
    else {
        printk("\tpte address = 0x%lx\n", (unsigned long)ts->pte);
    }
    if(!ts->ptl) {
        printk("\t!ptl\n");
    }
    else {
        printk("\tptl address = 0x%lx\n", (unsigned long)ts->ptl);
    }
    if(!ts->fast_page) {
        printk("\t!fast page\n");
    }
    if(!ts->slow_page) {
        printk("\t!slow page\\n");
    }
    if(ts->fast_page && ts->slow_page) {
        printk("\tfast page = 0x%lx, slow page = 0x%lx\n",
                (unsigned long)ts->fast_page, (unsigned long)ts->slow_page);
    }
    printk("===============================\n");
}

int get_list_size(struct list_head *list) {
    struct list_head *cursor;
    int i = 0;
    
    list_for_each(cursor, list) {
        ++i;
        if(i > 1000000000)
            break;
    }

    return i;
}
/*struct tier_struct *get_free_fast_entry(void) {
    unsigned long i;

    for(i = 0; i < ft_size_pages; i++) {
        if(tier_entries[i].fast_in_use == 0) {
            return &tier_entries[i];
        }
    }

    return NULL;
}*/

/* This should be the most self-explanatory thing here. Get the first entry
 * on the free list, or return NULL. */
struct tier_struct *get_free_fast_entry(void) {
    struct tier_struct *entry = list_first_entry_or_null(freelist,
            struct tier_struct, list);
    return entry;
}

/* To get the anon_vma of a heap page, we have to reconstruct the anon_vma
 * from the page mapping. This is done here. This is a pared-down version of
 * the kernel function that performs the same task, only local. */
struct anon_vma *get_page_anon_vma(struct page *page) {
    unsigned long mapping = (unsigned long)page->mapping;
    if((mapping & PAGE_MAPPING_FLAGS) != PAGE_MAPPING_ANON) {
        DEBUG_CODE(printk("mtier: page is not anonymous!\n"));
        return NULL;
    }

    mapping &= ~PAGE_MAPPING_FLAGS;

    return (struct anon_vma *)mapping;
}

/* The pgoff is used to reconstruct the userspace virtual address of the page
 * that is being manipulated. */
unsigned long get_page_pgoff(struct page *p) {
    return p->index << (PAGE_CACHE_SHIFT - PAGE_SHIFT);
}

/* Get the userspace virtual address of the page that's being manipulated. */
unsigned long get_page_usr_addr(struct vm_area_struct *vma,
        unsigned long pgoff) {
    return vma->vm_start + ((pgoff - vma->vm_pgoff) << PAGE_SHIFT);
}

/* Chase through the page table hierarchy to find the page middle directory of
 * the page that's being manipulated. The PMD is the (a, since pages can be 
 * shared) USERSPACE page table entry. */
pmd_t *get_pmd(struct mm_struct *mm, unsigned long addr) {
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd = NULL;

    pgd = pgd_offset(mm, addr);
    if(!pgd_present(*pgd)) {
        DEBUG_CODE(printk("\tpgd not found\n"));
        goto out;
    }
    DEBUG_CODE(printk("\tpgd = 0x%lx, pgd index = 0x%lx\n", 
                (unsigned long)pgd, *(unsigned long *)pgd));

    pud = pud_offset(pgd, addr);
    if(!pud_present(*pud)) {
        DEBUG_CODE(printk("\tpud not found\n"));
        goto out;
    }
    DEBUG_CODE(printk("\tpud = 0x%lx, pud index = 0x%lx\n", 
                (unsigned long)pud, *(unsigned long *)pud));

    pmd = pmd_offset(pud, addr);
    if(!pmd_present(*pmd)) {
        DEBUG_CODE(printk("\tpmd not found\n"));
        pmd = NULL;
        goto out;
    }
    DEBUG_CODE(printk("\tpmd = 0x%lx, pmd index = 0x%lx\n", 
                (unsigned long)pmd, *(unsigned long *)pmd));

out:
    return pmd;
}

static int move_page_mapping(struct page *page, struct page *newpage) {
    newpage->mem_cgroup = page->mem_cgroup;
    newpage->index = page->index;
    newpage->mapping = page->mapping;
    page->mem_cgroup = NULL;
    return 0;
}

static int copy_page_contents(struct page *from, struct page *to) {
    int cpupid;
    void *vfrom, *vto;

    /* The kernel uses kmap_atomic() and kunmap_atomic() to get vaddrs here.
     * Since this module only really works on 64-bit NUMA systems that don't
     * have highmem, we can safely just use the raw linear address obtained by
     * calling page_address.
     *
     * Get the virtual address of the CONTENTS of each of these pages, then
     * call copy_page to perform the actual memcpy. */
    vfrom = page_address(from);
    vto   = page_address(to);
    memcpy(vto, vfrom, PAGE_SIZE);

    if(PageError(from))
        SetPageError(to);
    if (PageReferenced(from))
        SetPageReferenced(to);
    if (PageUptodate(from))
        SetPageUptodate(to);
    if (TestClearPageActive(from)) {
        VM_BUG_ON_PAGE(PageUnevictable(from), from);
        SetPageActive(to);
    } else if (TestClearPageUnevictable(from))
        SetPageUnevictable(to);
    if (PageChecked(from))
        SetPageChecked(to);
    if (PageMappedToDisk(from))
        SetPageMappedToDisk(to);

    /* Move dirty on pages not done by migrate_page_move_mapping() */
    if (PageDirty(from))
        SetPageDirty(to);

    if (mtier_page_is_young(from))
        mtier_set_page_young(to);

    if (mtier_page_is_idle(from))
        mtier_set_page_idle(to);
    
    /*
     * Copy NUMA information to the new page, to prevent over-eager
     * future migrations of this same page.
     */
    cpupid = page_cpupid_xchg_last(from, -1);
    page_cpupid_xchg_last(to, cpupid);

    ksm_migrate_page(to, from);
    /*
     * Please do not reorder this without considering how mm/ksm.c's
     * get_ksm_page() depends upon ksm_migrate_page() and PageSwapCache().
     */
     if (PageSwapCache(from))
        ClearPageSwapCache(from);
     ClearPagePrivate(from);
     set_page_private(from, 0);
     
     /*
      * If any waiters have accumulated on the new page then
      * wake them up.
      */
      if (PageWriteback(to))
        end_page_writeback(to);

    return 0;
}

/* Move a page to another page. This is the heavyweight function, so it
 * includes a full copy, and is typically used for moving from the slow tier
 * to the fast tier. */
int heavy_copy(struct page *origin, struct tier_struct *ts, 
        struct vm_area_struct *vma, struct mm_struct *mm, 
        unsigned long pgoff) {
    int rv = 0;
    unsigned long usr_addr; 
    unsigned long pte_flags;
    struct page *dest = ts->fast_page;
    pmd_t *pmd;
    pte_t *ptep, pte;
    spinlock_t *ptl;
    
    /* Get the page userspace virtual address for the origin page, then make
     * sure it's a valid address. */
    usr_addr = get_page_usr_addr(vma, pgoff);
    DEBUG_CODE(printk("mtier heavy_copy: user address = 0x%lx\n", usr_addr));
    if(usr_addr < vma->vm_start || usr_addr >= vma->vm_end) {
        DEBUG_CODE(printk("mtier heavy_copy: bad user address.\n"));
        rv = -EINVAL;
        goto out;
    }

    /* Get the pmd for the page and make sure it's valid. */
    pmd = get_pmd(mm, usr_addr);
    if(!pmd) {
        DEBUG_CODE(printk("mtier heavy_copy: bad pmd.\n"));
        rv = -EINVAL;
        goto out;
    }

    /* Get the PTE and the page table lock. */
    ptep = pte_offset_map(pmd, usr_addr);
    if(!ptep) {
        DEBUG_CODE(printk("mtier heavy_copy: invalid pte.\n"));
        rv = -EINVAL;
        goto out;
    }
    ptl = pte_lockptr(mm, pmd);

    DEBUG_CODE(printk("\tmtier heavy_copy: pte = 0x%lx, pte value = 0x%lx\n",
                (unsigned long)ptep, *(unsigned long *)ptep));
    DEBUG_CODE(printk("\tmtier_heavy_copy: physical address = 0x%lx\n",
                (unsigned long)page_to_phys(origin)));

    /* Point of no return. */
    spin_lock(ptl);
    /* Create the new pte */
    pte_flags = (*ptep).pte & 0xF0000000000000FF;
    if(pte_flags ^ 0x8000000000000065) {
        DEBUG_CODE(printk("mtier heavy_copy: skipping read-write page.\n"));
        rv = -EINVAL;
        goto out_unlock;
    }

    DEBUG_CODE(printk("mtier heavy_copy: pte flags = 0x%lx\n", pte_flags));
    pte.pte = (*ptep).pte & 0x8000000000000065;
    pte.pte |= (ts->fast_pfn << 12);

    /* Set up the tier struct */
    ts->fast_in_use = 1;
    ts->fast_valid = 1;
    ts->slow_pfn = page_to_phys(origin) >> 12;
    ts->usr_vaddr = usr_addr;
    ts->slow_pte = *ptep;
    ts->fast_pte = pte;
    ts->pte = ptep;
    ts->slow_page = origin;
    ts->ptl = ptl;
    DEBUG_CODE(printk("mtier heavy_copy: ts->fast_pfn = 0x%lx, new pte = "
                "0x%lx\n", ts->fast_pfn, pte.pte));

    /* Copy page contents */
    if(copy_page_contents(origin, dest)) {
        DEBUG_CODE(printk("mtier heavy_copy: page copy failed.\n"));
        ts->fast_in_use = 0;
        ts->fast_valid = 0;
        rv = -EIO;
        goto out_unlock;
    }
    move_page_mapping(origin, dest);
    *ptep = pte;
    DEBUG_CODE(printk("mtier heavy_copy: success.\n"));

    /* Move the entry from the freelist to the usedlist. */
    list_del(&(ts->list));
    list_add(&(ts->list), usedlist);

    /* Add the struct to the side table with the appropriate key for fast,
     * vaddr-based lookups. */
    hash_add(side_tbl, &(ts->hl_node), MTIER_HASH_KEY(page_address(origin)));
    ts->slow_vaddr = (unsigned long)page_address(origin);
    ts->fast_vaddr = (unsigned long)page_address(ts->fast_page);

    //DEBUG_CODE(print_tier_struct_info(ts));

out_unlock:
    spin_unlock(ptl);
out:
    return rv;
}

/* Given a page, walk its anon_vma chain and copy all entries to the fast
 * memory tier */
/* This function is BROKEN. It works just fine for heap pages that are, by
 * definition, only used by one process. Trying to use it on a shared page
 * would be, as the Germans say, toericht. */
int avma_walk_and_copy(struct page *origin, struct task_struct *tsk) {
    int rv = 0;
    unsigned long pgoff = 0;
    struct anon_vma *avma;
    struct anon_vma_chain *avc;
    struct vm_area_struct *vma;
    struct tier_struct *ts;

    /* Get the anon_vma and make sure its valid, then get the page offset */
    avma = get_page_anon_vma(origin);
    if(!avma || avma == NULL) {
        DEBUG_CODE(printk("mtier avma_walk_and_copy: invalid avma.\n"));
        rv = -EINVAL;
        goto out;
    }
    pgoff = get_page_pgoff(origin);
    DEBUG_CODE(printk("mtier avma_walk_and_copy: avma = 0x%lx, pgoff = 0x%lx\n",
                (unsigned long)avma, pgoff));

    /* Lock the avma, walk its interval tree, and copy the page in each VMA
     * in which it appears. */
    down_read(&avma->root->rwsem);
    anon_vma_interval_tree_foreach(avc, &avma->rb_root, pgoff, pgoff) {
        DEBUG_CODE(printk("mtier avma_walk_and_copy: avc = 0x%lx\n", 
                    (unsigned long)avc));
        if(!avc->vma) {
            DEBUG_CODE(printk("mtier avma_walk_and_copy: invalid vma.\n"));
            continue;
        }
        vma = avc->vma;
        DEBUG_CODE(printk("mtier avma_walk_and_copy: vma = 0x%lx\n", 
                    (unsigned long)vma));

        /* Get a free tier_struct and set it up, then call heavy_copy. */
        ts = get_free_fast_entry();
        if(!ts) {
            DEBUG_CODE(printk("mtier avma_walk_and_copy: no free entry.\n"));
            rv = -ENOMEM;
            break;
        }
        ts->owner = tsk->pid;
        if(!mtier_trylock_page(ts->fast_page)) {
            DEBUG_CODE(printk("mtier avma_walk_and_copy: could not lock fast "
                        "page.\n"));
            continue;
        }
        heavy_copy(origin, ts, vma, tsk->mm, pgoff);
        unlock_page(ts->fast_page);
    }

    up_read(&avma->root->rwsem);
out:
    return rv;
}

/* Given a tier struct, swap its fast entry to its slow entry and invalidate
 * the tier struct so it can be reused later. */
int swap_fast_to_slow(struct tier_struct *ts) {
    int rv = 0;
    struct page *source = ts->fast_page;
    struct page *dest = ts->slow_page;

    if(!ts->fast_page) {
        printk("mtier swap_fast_to_slow: invalid fast page.\n");
        return -EINVAL;
    }
    else if(!ts->slow_page) {
        printk("mtier swap_fast_to_slow: invalid slow page.\n");
        return -EINVAL;
    }

    /* Testing stuff */
    DEBUG_CODE(printk("mtier swap_fast_to_slow: entry\n"));
    //DEBUG_CODE(print_tier_struct_info(ts));

    /* Swap the mappings and the PTE */
    mtier_unmap_and_move(source, dest, 0, 2);
    ts->fast_in_use = 0;
    ts->fast_valid = 0;
    hash_del(&ts->hl_node);
    if(ts->fast_page) {
        clear_bit(PG_active, &(ts->fast_page)->flags);
        __free_pages(ts->fast_page, 0);
        ts->fast_page = NULL;
    }
    return rv;
}

/* The following functions are eviction functions. The types of eviction are:
 * 1. Evict all pages marked as unused (e.g. during a standard iteration).
 * 2. Evict all pages belonging to a pid (e.g. after a process exit).
 * 3. Evict all pages (e.g. during a bookkeeping iteration). */

/* This MUST be called after the invalidation/revalidation sweeps during
 * duplication. Failure to do so will result in unused entries still being
 * present in the usedlist, which can cause some gross issues on process exit
 * and when you need to reclaim additional pages for duplication. */
int evict_unused_tier_structs(void) {
    struct tier_struct *cursor, *tmp;
    int unfreed_count = 0;
    int freed_count = 0;
#ifdef MTIER_TIMING
    ktime_t start, end;
#endif

    DEBUG_CODE(printk("mtier evict_unused_tier_structs: entry\n"));
#ifdef MTIER_TIMING
    start = ktime_get();
#endif
    list_for_each_entry_safe(cursor, tmp, usedlist, list) {
        /* Try to swap - nonzero rv indicates failure and the entry should not
         * be returned to the freelist in this case. */
        if(cursor->fast_in_use)
            continue;
        
        if(swap_fast_to_slow(cursor)) {
            ++unfreed_count;
        }
        else {
            list_del(&(cursor->list));
            list_add(&(cursor->list), freelist);
            ++freed_count;
        }
    }
    DEBUG_CODE(printk("mtier evict_unused_tier_structs: failed to free %d\n",
                unfreed_count));
#ifdef MTIER_TIMING
    end = ktime_get();
    printk("mtier evict_unused_tier_structs: %d pages in %lu nanoseconds\n",
            freed_count, (unsigned long)(end.tv64 - start.tv64));
#endif

    return unfreed_count;
}

int evict_pid_tier_structs(pid_t pid) {
    struct tier_struct *cursor, *tmp;
    int unfreed_count = 0;
    int freed_count =0 ;
    int i = 0;

    DEBUG_CODE(printk("mtier evict_pid_tier_structs: entry\n"));
    //printk("mtier evict_pid_tier_structs: calculated usedlist size = %d\n",
    //         get_list_size(usedlist));
    list_for_each_entry_safe(cursor, tmp, usedlist, list) {
        ++i;
        if(cursor->owner != pid) {
            DEBUG_CODE(printk("FREE FAILURE FOR PID %d:\n", (int)pid));
            DEBUG_CODE(print_tier_struct_info(cursor));
            continue;
        }

        if(swap_fast_to_slow(cursor)) {
            ++unfreed_count;
        }
        else {
            list_del(&(cursor->list));
            list_add(&(cursor->list), freelist);
            ++freed_count;
        }
    }
    DEBUG_CODE(printk("mtier evict_pid_tier_structs: failed to free %d\n",
                unfreed_count));
    DEBUG_CODE(printk("\tbut freed: %d\n", freed_count));
    DEBUG_CODE(printk("\titerated over %d\n", i));

    return unfreed_count;
}

int evict_all_tier_structs(void) {
    struct tier_struct *cursor, *tmp;
    int unfreed_count = 0;

    DEBUG_CODE(printk("mtier evict_all_tier_structs: entry\n"));
    list_for_each_entry_safe(cursor, tmp, usedlist, list) {
        if(swap_fast_to_slow(cursor)) {
            ++unfreed_count;
        }
        else {
            list_del(&(cursor->list));
            list_add(&(cursor->list), freelist);
        }
    }
    DEBUG_CODE(printk("mtier evict_all_tier_structs: failed to free %d\n",
                unfreed_count));

    return unfreed_count;
}

static int mod_mtier_generate_pagelist(struct mm_struct *mm) {
    struct list_head *iter;
#ifdef MTIER_TIMING
    ktime_t start, end;

    start = ktime_get();
#endif

    pagelist = mtier_get_ro_pagelist(mm, 0, 1, pagelist);
    //printk("mtier: call complete, pagelist=0x%lx\n", (unsigned long)pagelist);
    pagecount = 0;
    list_for_each(iter, pagelist) {
        ++pagecount;
    }
    //printk("mtier: page count is %d\n", pagecount);
#ifdef MTIER_TIMING
    end = ktime_get();
    printk("mtier mod_mtier_generate_pagelist: rebuilt pagelist f size %d "
           "in %lu nanoseconds.\n", pagecount,
           (unsigned long)(end.tv64 - start.tv64));
#endif

    return 0;
}

static void shuffle_pagelist (void) {
    struct page *cur;
    int i;
    unsigned int index, rand, tmp;

    if (pagecount == 0) {
        return;
    }

    if (shuffled_pages != NULL) {
        vfree(shuffled_pages);
    }

    shuffled_pages = vmalloc(sizeof(struct page*)*pagecount);

    i = 0;
    list_for_each_entry(cur, pagelist, lru) {
        shuffled_pages[i] = cur;
        i++;
    }
    
    get_random_bytes_arch(&rand, sizeof(unsigned int));
    for (i = pagecount - 1; i > 0; i--) {
      //get_random_bytes_arch(&rand, sizeof(unsigned int));
      //index = rand % (i + 1);
      tmp   = (i ^ rand);
      index = tmp % (i + 1);

      // Simple swap
      cur = shuffled_pages[index];
      shuffled_pages[index] = shuffled_pages[i];
      shuffled_pages[i] = cur;
    }
}

/* Iterate over the pagelist and:
 * 1. Mark each fast entry as free
 * 2. Determine which entries are going to be moved back to the fast tier
 *    mark them as in use to prevent them from being swapped back and moved
 *    again (i.e. performance considerations). */ 
unsigned long check_used_pages(int start, int cutoff) {
    unsigned long stay_count = 0;
    int i, j, found;
    int num_to_count;
    struct tier_struct *tier_cursor;
    struct page *page_cursor = NULL;
#ifdef MTIER_TIMING
    ktime_t tstart, end;
    
    tstart = ktime_get();
#endif

    num_to_count = MIN2(pagecount, PAGES(SIZE_MB));
    /* Invalidate each fast entry */
    for(i = 0; i < num_to_count; i++) {
        tier_entries[i].fast_in_use = 0;
    }

    for(i = start; i < start + cutoff; i++) {
        found = 0;
        j = 0;
        
        page_cursor = shuffled_pages[i];
        if(!page_cursor) {
            DEBUG_CODE(printk("mtier: ERROR -- invalid page "
                        "in check_used_pages.\n"));
            break;
        }

        hash_for_each_possible(side_tbl, tier_cursor, hl_node,
                MTIER_HASH_KEY(page_address(page_cursor))) {
            if(tier_cursor->slow_vaddr ==
                    (unsigned long)page_address(page_cursor)) {
                tier_cursor->fast_in_use = 1;
                shuffled_pages[i] = NULL;
                found = 1;
                ++stay_count;
                DEBUG_CODE(printk("mtier check_used_pages: found page 0x%lx\n",
                            tier_cursor->slow_vaddr));
                break;
            }
        }
    }

    /* Now clear the hash table entries for found pages. */
    for(i = 0; i < num_to_count; i++) {
        if(!tier_entries[i].fast_in_use) {
            hash_del(&(tier_entries[i].hl_node));
        }
    }

#ifdef MTIER_TIMING
    end = ktime_get();
    printk("mtier check_used_pages completed in %lu nanoseconds.\n",
           (unsigned long)(end.tv64 - tstart.tv64));
#endif

    return stay_count;
}

/* Empty the freelist then fill it with tier_struct entries that are available
 * to be used. */
unsigned long populate_freelist(void) {
    unsigned long free_count = 0;
    int i;

    INIT_LIST_HEAD(freelist);
    for(i = 0; i < ft_size_pages; i++) {
        if(tier_entries[i].fast_in_use == 0) {
            list_add(&(tier_entries[i].list), freelist);
            ++free_count;
        }
    }

    return free_count;
}

/* Handle the actual duplication/fast swap stuff here. */
static int mod_mtier_duplicate(struct task_struct *tsk) {
    struct mm_struct *mm = NULL;
    const struct cred *cred = current_cred(), *tcred;
    int err, i, stay_cnt, swap_cnt, cutoff;
    int num_copied = 0;
    struct tier_struct *ts;
#ifdef MTIER_TIMING
    ktime_t start, end;
#endif

    ++tot_iter;
    /* Check credentials and make sure we're authorized to perform memory 
     * operations. We should be since this is a kernel module, but we don't
     * want to take any unnecessary risks. */
    rcu_read_lock();
    get_task_struct(tsk);
    tcred = __task_cred(tsk);
    if( !uid_eq(cred->euid, tcred->suid) && !uid_eq(cred->euid, tcred->uid) &&
        !uid_eq(cred->uid,  tcred->suid) && !uid_eq(cred->uid,  tcred->uid) &&
        !capable(CAP_SYS_NICE)) {

        rcu_read_unlock();
        printk("mtier: credential failure.\n");
        goto fail;
    }

    rcu_read_unlock();

    err = security_task_movememory(tsk);
    if(err) {
        printk("mtier: security_task_movememory failure.\n");
        goto fail_put;
    }

    mm = get_task_mm(tsk);
    put_task_struct(tsk);
    printk("mod_iter: %d\n", mod_iter);

    /* Call next function here */
    if(mod_iter % ft_bk_iters == 0) {
        /* Bookkeeping iteration */
        mod_iter = 1;

        /* Evict the fast tier */
        for(i = 0; i < 5; i++) {
            int count = evict_all_tier_structs();
            if(count == 0)
                break;
        }
        if(i == 5) {
            DEBUG_CODE(printk("mtier: may have failed to free some fast "
                        "entries during bookkeeping iteration.\n"));
        }

        /* Clear out anything remaining in the pagelist. */
        if (!(list_empty(pagelist))) {
            putback_movable_pages(pagelist);
            INIT_LIST_HEAD(pagelist);
        }
        mod_mtier_generate_pagelist(mm);
        DEBUG_CODE(printk("pagelist size = %d\n", get_list_size(pagelist)));
    }
    else {
        int count;
        /* Standard iteration */

        ++mod_iter;
        /* We want to re-shuffle the pagelist for each iteration, for now */
        shuffle_pagelist();
        
        /* Determine the cutoff size and prepare pages and tier entries for 
         * duplication by invalidating all and re-validating anything that's
         * already in the fast tier. */
        cutoff = MIN2(ft_size_pages / ft_fraction, pagecount);
        stay_cnt = check_used_pages(0, cutoff);
        printk("mtier: stay_entries: %d\n", stay_cnt);
        count = evict_unused_tier_structs();
        if(count)
            DEBUG_CODE(printk("mtier: failed to free %d entries\n", count));

        swap_cnt = 0;
        DEBUG_CODE(printk("mtier: starting swap loop.\n"));
#ifdef MTIER_TIMING
        start = ktime_get();
#endif
        for(i = 0; i < cutoff; i++) {
            if(!shuffled_pages[i])
                continue;

            /* The _heavy variant uses a barely-modified variant of stock
             * migration. Used as a comparison of mtier's duplication. */
            ts = get_free_fast_entry();
            if(!ts) break;
            ts->fast_page = alloc_page_interleave(
                    (gfp_t *)GFP_HIGHUSER_MOVABLE, 0, 1);
            if(!ts->fast_page)
                break;

            mtier_unmap_and_move(shuffled_pages[i], ts->fast_page, 0, 2);
            ts->fast_in_use = 1;
            ts->slow_page = shuffled_pages[i];
            hash_add(side_tbl, &(ts->hl_node),
                    MTIER_HASH_KEY(page_address(ts->slow_page)));
            ts->slow_vaddr = (unsigned long)page_address(ts->slow_page);
            ts->fast_vaddr = (unsigned long)page_address(ts->fast_page);
            ts->fast_pfn = (unsigned long)page_to_phys(ts->fast_page);
            ts->fast_pfn >>= 12;
            list_del(&(ts->list));
            list_add(&(ts->list), usedlist);
            ++num_copied;
        }
#ifdef MTIER_TIMING
        end = ktime_get();
        printk("mtier duplicate heavy-copied %d pages in %lu nanoseconds\n",
                num_copied, (unsigned long)(end.tv64 - start.tv64));
#endif
        DEBUG_CODE(printk("mtier: swap loop finished.\n"));
    }

    /* This needs to be the last thing the module does. */
    mmput(tsk->mm);
    return 0;

fail_put:
    put_task_struct(tsk);
fail:
    return -1;

}

int thr_run(void *dat) {
    //int numpages = 100;
    //int nsdelay  = 1000;
    struct task_struct *tsk;
    pid_t pid = 0;
    int pages;
    ktime_t call_start;
    //ktime_t call_end;
    NODEMASK_ALLOC(nodemask_t, old, GFP_KERNEL);
    NODEMASK_ALLOC(nodemask_t, new, GFP_KERNEL);
    nodes_clear(*old);
    nodes_clear(*new);
    node_set(0, *old);
    node_set(1, *new);
    printk("old: %lu\nnew: %lu\n", *(unsigned long *)old, *(unsigned long *)new);
   
    printk("mod_mtier: thread entry\n"); 
    while(!kthread_should_stop()) {
        //ndelay(nsdelay);
        msleep(ft_delay);
        for_each_process(tsk) {
            if(tsk->should_tier == 1) {
                DEBUG_CODE(printk("mtier: tsk: %lu\n",
                            (unsigned long)tsk->pid));
                /* If we can't get the lock, that means do_exit is attempting
                 * to clean the process up. do_exit takes this lock, then waits
                 * on it. So, the module must restore the process to its
                 * default state and then release the lock. */
                if(down_trylock(&tsk->exit_sem)) {
                    printk("mtier: module failed to acquire exit lock.\n");
                    DEBUG_CODE(printk("freelist size: %d\n",
                                get_list_size(freelist)));
                    DEBUG_CODE(printk("usedlist size: %d\n",
                                get_list_size(usedlist)));
                    evict_pid_tier_structs(tsk->pid);
                    tsk->should_tier = 0;
                    up(&tsk->exit_sem);
                    continue;
                }
                else {
                    DEBUG_CODE(printk("mtier: tiering-eligible process: %lu\n",
                            (unsigned long)tsk->pid));
                    pid = tsk->pid;
                    call_start = ktime_get();
                    //pages = mod_do_mtier_management(pid, 300000, old, new, 2);
                    //if(moved_once < 50) {
                    //    ++moved_once;
                    pages = mod_mtier_duplicate(tsk);
                    //}
                      /*call_end = ktime_get();
                      printk("mod_mtier: moved %d pages in %lu ns\n", pages, 
                      (unsigned long)call_end.tv64 - (unsigned long)call_start.tv64);*/
                      up(&tsk->exit_sem);
                }
            }
        }
    }
    
    printk("mtier: worker thread termination\n");
    return counter;
}

static int mod_mtier_init(void) {
    int curr_node, i;
    struct list_head *cursor;

    pagelist = (struct list_head *)kmalloc(sizeof(struct list_head),
            GFP_KERNEL);
    freelist = (struct list_head *)kmalloc(sizeof(struct list_head),
            GFP_KERNEL);
    usedlist = (struct list_head *)kmalloc(sizeof(struct list_head),
            GFP_KERNEL);
    if(!pagelist || !freelist || !usedlist) {
        printk("list allocation error.\n");
        return -1;
    }

    INIT_LIST_HEAD(pagelist);
    INIT_LIST_HEAD(freelist);
    INIT_LIST_HEAD(usedlist);

    shuffled_pages = NULL;
    printk("mtier: module entry\n");

    /* Set up the user-specified tier size */
    if(ft_size_mb > SIZE_MB)
        ft_size_mb = SIZE_MB;

    ft_size_pages = PAGES(ft_size_mb);

    printk("mtier: reserved %lu mb (%lu pages), using %lu mb (%lu pages)\n",
            (unsigned long)SIZE_MB, (unsigned long)PAGES(SIZE_MB), ft_size_mb,
            ft_size_pages);
    printk("mtier: will move 1/%lu of fast tier (%lu mb, %lu pages) each\n",
            ft_fraction, ft_size_mb / ft_fraction,
            ft_size_pages / ft_fraction);
    printk("mtier: delay is %lu ms with a bookeeping iter every %lu iters\n",
            ft_delay, ft_bk_iters);

    counter = 0;

    /* BEGIN INITIAL DUPLICATION CODE */
    /* This code just reserves memory on the fast tier by allocating an array
     * of pages. There's really nothing magical about it. By making sure that 
     * the entire fast-tier size is reserved from the get-go, we both improve
     * performance and prevent OOM issues from occuring in the middle of
     * process execution. If this fails, the module should abort cleanly.
     */
    hash_init(side_tbl);
    for_each_online_node(curr_node) {
        if(curr_node != fast_node) {
            printk("Node %d breaking (not %lu)\n", curr_node, fast_node);
            continue;
        }
        printk("Node %d reporting!\n", curr_node);
        for(i = 0; i < PAGES(SIZE_MB); i++) {
            tier_entries[i].owner = 0;
            tier_entries[i].fast_in_use = 0;
            tier_entries[i].fast_valid = 0;
            tier_entries[i].slow_page = NULL;
            list_add(&(tier_entries[i].list), freelist);
        }

        i = 0;
        list_for_each(cursor, freelist) {
            ++i;
        }
        printk("mtier: first page: 0x%lx\n",
                (unsigned long)tier_entries[0].fast_page);
    }
    /* END INITIAL DUPLICATION CODE */

    kthread = kthread_run(thr_run, NULL, "mtier_worker");
    if(IS_ERR(kthread)) {
        printk("mtier: your thread done blowed up\n");
        return PTR_ERR(kthread);
    }
    return 0;

oom:
    printk("Error allocating memory for page, i = %d\n", i);
    BUG_ON(1);
}

module_init(mod_mtier_init);

static void mod_mtier_exit(void) {
    int thr_rv = kthread_stop(kthread);
    int i;

    printk("mtier: entered module exit function.\n");

    if(thr_rv != -EINTR) {
        printk("mtier: worker thread has stopped.\n");
    }

    if (!list_empty(pagelist)) {
        printk("mtier: putting back pagelist.\n");
        putback_movable_pages(pagelist);
        printk("mtier: pagelist put back.\n");
    }
    else {
        printk("mtier: no pagelist to put back.\n");
    }

    /* BEGIN REPLICATION FREE CODE */
    /* Because of the way the module claims pages, we have to force them to an
     * unused status before we can free them, or else we get page dump/charge
     * errors. Because of this:
     * DO NOT REMOVE THE MODULE WHILE A TIERING PROCESS IS RUNNING! In the
     * future, this should be changed to allow clean swap backs to the slow
     * tier for any running process. */
    printk("mtier: freeing fast tier simulant.\n");
    for(i = 0; i < PAGES(SIZE_MB); i++) {
        if(tier_entries[i].fast_page) {
            tier_entries[i].fast_page->mem_cgroup = NULL;
            tier_entries[i].fast_page->mapping = NULL;
            clear_bit(PG_active, &(tier_entries[i].fast_page)->flags);
            __free_pages(tier_entries[i].fast_page, 0);
        }
    }
    printk("mtier: fast tier simulant freed.\n");

    if (shuffled_pages != NULL) {
        printk("mtier: about to free shuffled pages.\n");
        vfree(shuffled_pages);
        printk("mtier: shuffled pages freed\n");
    }
    else {
        printk("mtier: no shuffled pages to free.\n");
    }
    /* END REPLICATION FREE CODE */
    /* Free the lists */
    kfree(pagelist);
    kfree(usedlist);
    kfree(freelist);

    printk("mtier: module exit with count %d\n", counter);
}

module_exit(mod_mtier_exit);
