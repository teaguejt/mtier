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
//#include <linux/nodemask.h>
//#include <asm-generic/memory_model.h>
#include <asm/page.h>
#include <asm/bitops.h>
#include "mod_mtier.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph Teague");
MODULE_DESCRIPTION("The module for the mtier kthread worker");

#define SIZE_MB 1024
//#define SIZE_MB 2
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

struct tier_struct {
    pid_t owner;
    int fast_in_use;
    int fast_valid;
    unsigned long fast_pfn;
    unsigned long fast_paddr;
    unsigned long fast_vaddr;
    unsigned long slow_pfn;
    unsigned long slow_paddr;
    unsigned long slow_vaddr;
    struct page *slow_page;
    struct page *fast_page;
    struct hlist_node hl_node;
    struct list_head list;
    pte_t slow_pte;
    pte_t fast_pte;
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

static struct task_struct *kthread;
static int counter;
//static int slow_node = 0;
static int fast_node  = 1;
//static int slow_node  = 0;
static int mod_iter   = 0;
static int tot_iter   = 0;
//static int unused_idx = 0;
//static int ms_delay   = 1000;
//static int iter_pages = 1000;
static int pagecount  = 0;
//static int pers_node = 2;    /* Unused, for now */
struct tier_struct tier_entries[(SIZE_MB << 20) / PAGE_SIZE];
static struct list_head *pagelist;
static struct page **shuffled_pages;
DEFINE_HASHTABLE(side_tbl, MTIER_HASH_BITS);

struct tier_struct *get_free_fast_entry(void) {
    unsigned long i;

    for(i = 0; i < ft_size_pages; i++) {
        if(tier_entries[i].fast_in_use == 0) {
            return &tier_entries[i];
        }
    }

    return NULL;
}

struct address_space *get_page_mapping(struct page *page) {
    unsigned long mapping;                                                                                                                                                                             
    /* This happens if someone calls flush_dcache_page on slab page */
    if (unlikely(PageSlab(page)))
        return NULL;
    if (unlikely(PageSwapCache(page))) {
        swp_entry_t entry;

        entry.val = page_private(page);
        //return &swapper_spaces[swp_type(entry)];
        return (struct address_space *)-EINVAL;
        //return swap_address_space(entry);
    }

    mapping = (unsigned long)page->mapping;
    if (mapping & PAGE_MAPPING_FLAGS)
        return NULL;
    return page->mapping;
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
    ktime_t start, end;

    /* The kernel uses kmap_atomic() and kunmap_atomic() to get vaddrs here.
     * Since this module only really works on 64-bit NUMA systems that don't
     * have highmem, we can safely just use the raw linear address obtained by
     * calling page_address.
     *
     * Get the virtual address of the CONTENTS of each of these pages, then
     * call copy_page to perform the actual memcpy. */
    vfrom = page_address(from);
    vto   = page_address(to);
    printk("mtier: vfrom=0x%lx vto=0x%lx ...", (unsigned long)vfrom,
            (unsigned long)vto);
    //copy_page(to, from);
    //start = ktime_get();
    //memcpy(from, to, PAGE_SIZE);
    memcpy(vto, vfrom, PAGE_SIZE);
    //end = ktime_get();
    //printk("mtier: memcpy took %lu ns\n",
    //        (unsigned long)(end.tv64 - start.tv64));
    printk("done.\n");

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

    //printk("mtier: copied from 0x%lx to 0x%lx\n",
    //        (unsigned long)vfrom, (unsigned long)vto);

    return 0;
}

static int perform_page_swap(struct page *page, struct page *newpage) {
    struct address_space *mapping;
    int rc;

    if ((page == NULL) || (newpage == NULL)) {
        printk("mtier: error: perform_page_swap! old: %p new: %p\n",
                page, newpage);
        return -EINVAL;
    }

    mapping = get_page_mapping(page);
    if(mapping == (struct address_space *)-EINVAL) {
        printk("mtier: invalid mapping (swap)\n");
        return -EINVAL;
    }
    else if(mapping) {
        printk("mtier: mapping found, tiering only works for anon pages\n");
        return -EINVAL;
    }

    /* if !mapping, essentially */
    //rc = copy_page_contents(page, newpage);
    rc = move_page_mapping(page, newpage);    

    return 0;
}

/*static struct page *page_by_address(const struct mm_struct *mm, 
        const unsigned long addr) {
    struct page *page = NULL;
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    pgd = pgd_offset(mm, addr);
    if(!pgd_present(*pgd)) {
        goto out;
    }

    pud = pud_offset(pgd, addr);
    if(!pud_present(*pud)) {
        goto out;
    }

    pmd = pmd_offset(pud, addr);
    if(!pmd_present(*pmd)) {
        goto out;
    }

    pte = pte_offset_map(pmd, addr);
    if(!pte_present(*pte)) {
        goto out;
    }

    page = pte_page(*pte);
out:
    return page;
}*/

/*
static struct vm_area_struct *get_vma_of_page(const struct mm_struct *mm, const struct page *page) {
    struct vm_area_struct *vma = NULL, *fvma;
    struct anon_vma *avma;
    struct anon_vma_chain *avc;
    unsigned long mapping;
    pgoff_t pgoff;

    page = compound_head(page);
    mapping = (unsigned long)page->mapping;
    if((mapping & PAGE_MAPPING_FLAGS) != PAGE_MAPPING_ANON) {
        return NULL;
    }
    mapping &= ~PAGE_MAPPING_FLAGS;
    avma = (struct anon_vma *)mapping;

    if (unlikely(PageHeadHuge(page)))
        pgoff = page->index << compound_order(page);
    else if (likely(!PageTransTail(page)))
        pgoff = page->index;
    else {
        pgoff = compound_head(page)->index;
        pgoff += page - compound_head(page);
    }

    anon_vma_interval_tree_foreach(avc, &avma->rb_root, pgoff, pgoff) {
        vma = avc->vma;
        fvma = avc->vma;
    }
    
    return vma;
}*/

static int mod_mtier_generate_pagelist(struct mm_struct *mm) {
    struct list_head *iter;

    pagelist = mtier_get_ro_pagelist(mm, 0, 1, pagelist);
    printk("mtier: call complete, pagelist=0x%lx\n", (unsigned long)pagelist);
    pagecount = 0;
    list_for_each(iter, pagelist) {
        ++pagecount;
    }
    printk("mtier: page count is %d\n", pagecount);

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

/* This just swaps the mapping without actually copying the page, whereas
 * swap_mapping() moves the whole thing. I really need to rename these for 
 * clarity. Anyway, swap_mapping_back() assumes an existing page on the slow
 * tier and therefore does not copy, whereas swap_mapping assumes nothing on
 * the fast tier and then performs a copy too. */
static int swap_mapping_back(struct page *slow_page, struct tier_struct *ts) {
    int swaprv = 0;

    swaprv = perform_page_swap(slow_page, ts->fast_page);
    if(swaprv) {
        printk("mtier: error swapping page mapping back!\n");
    }

    ts->fast_in_use = 0;
    ts->slow_vaddr = 0;

    return 0;
}

static int swap_mapping(struct page *slow_page, struct tier_struct *free_entry) {
    unsigned long addr, vaddr;
    int swaprv;
    //pgd_t *pgd;
    //pud_t *pud;
    //pmd_t *pmd;
    //pte_t *ptep, pte;
    //spinlock_t ptelock;
    //struct page *newpage, *pageref;
    //struct vm_area_struct *vma;

    addr = page_to_phys(slow_page);
    vaddr = (unsigned long)page_address(slow_page);
    //avma = page_get_anon_vma(page);
    //printk("mtier: vaddr=0x%lx\n", vaddr);
    //vma = get_vma_of_page(mm, page);

    /*free_entry = get_free_fast_entry();
    if(free_entry) {
        printk("mtier: free fast entry found: 0x%lx\n",
                (unsigned long)free_entry);
    }
    else {
        printk("mtier: no free fast entry found :-(\n");
    }*/

    swaprv = copy_page_contents(slow_page, free_entry->fast_page);
    swaprv = perform_page_swap(slow_page, free_entry->fast_page);
    free_entry->fast_in_use = 1;
    free_entry->slow_page = slow_page;
    free_entry->slow_vaddr = vaddr;

    /*pgd = pgd_offset(mm, vaddr);
    if(!pgd_present(*pgd)) {
        printk("mtier: pgd not present.\n");
        return -1;
    }

    pud = pud_offset(pgd, vaddr);
    if(!pud_present(*pud)) {
        printk("mtier: bad pud\n");
        return -1;
    }
    pmd = pmd_offset(pud, vaddr);
    if(!pmd_present(*pmd)) {
        printk("mtier: pmd not present.\n");
        return -1;
    }

    //ptep = pte_offset_map_lock(mm, pmd, vaddr, ptelock);
    ptep = pte_offset_map(pmd, vaddr);
    if(!pte_present(*ptep)) {
        printk("mtier: no pte present\n");
        goto out_unlock;
    }
    if(!ptep) {
        printk("mtier: bad pte\n");
        goto out_unlock;
    }

    pageref = pte_page(*ptep);
    if(!pageref) {
        printk("mtier: bad page ref\n");
        return -1;
    }

    printk("mtier: pgd=0x%lx pud=0x%lx pmd=0x%lx pte=0x%lx page=0x%lx "
            "pageref=0x%lx\n",
            (unsigned long)pgd, (unsigned long)pud, (unsigned long)pmd,
            (unsigned long)ptep, (unsigned long)page, (unsigned long)pageref);
    printk("mtier: page=0x%lx pageref=0x%lx pte_pfn=0x%lx mm=0x%lx mm->pgd=0x%lx\n", 
            (unsigned long)page_address(page),
            (unsigned long)page_address(pageref),
            (unsigned long)ptep->pte,
            (unsigned long)mm,
            (unsigned long)mm->pgd);
    */
//out_unlock:
    //pte_unmap_unlock(ptep, ptelock);
    return 0;
}

static int mod_mtier_duplicate(struct task_struct *tsk) {
    //int pages_moved = 0;
    struct mm_struct *mm = NULL;
    const struct cred *cred = current_cred(), *tcred;
    //struct page *cursor, *tmp;
    //unsigned long start, end, cur_addr;
    int err, i, j, free_cnt, stay_cnt, swap_cnt, cutoff;
    //struct vm_area_struct *vma = NULL, *fvma;
    struct tier_struct *tier_cursor;
    ktime_t tot_start, tot_end, tot_total, total;
    struct list_head *cur;
    struct tier_struct free_entries;

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
        mod_iter = 1;
        if (!(list_empty(pagelist))) {
            putback_movable_pages(pagelist);
            INIT_LIST_HEAD(pagelist);
        }
        mod_mtier_generate_pagelist(mm);
    }

    //printk("yaka: %d\n", mod_iter);
    shuffle_pagelist();
    //printk("daka: %d\n", mod_iter);

    /* Set usage stats to zero - this marks each fast page as unused,
     * and the subsequent hash table search will re-mark any that need to be
     * preserved in the fast tier. */
    for(i = 0; i < ft_size_pages; i++) {
        tier_entries[i].fast_in_use = 0;
    }
    //printk("raka: %d\n", mod_iter);

    cutoff = MIN2(ft_size_pages / ft_fraction, pagecount);

    /* Scan the list of pages eligible for migration, and mark ones that are
     * already in the fast tier as "in use." */
    stay_cnt = 0;
    printk("mtier: cutoff: %d\n", cutoff);
    for(i = 0; i < cutoff; i++) {
        int found = 0;
        //if (i < 100) {
        //printk("mtier: i: %d sp: 0x%lx\n", i, page_address(shuffled_pages[i]));
        //}
#if 0
        if ((i % 1000) == 0) {
            printk("cutloop: i: %d\n", i);
        }
#endif
        j = 0;
        hash_for_each_possible(side_tbl, tier_cursor, hl_node,
                MTIER_HASH_KEY(page_address(shuffled_pages[i]))) {
            //if (i < 100) {
            //printk("rar: e: %p sp: 0x%lx slow: 0x%lx\n", tier_cursor,
            //        (unsigned long)page_address(shuffled_pages[i]),
            //        (unsigned long)tier_cursor->slow_vaddr);
            //}
            if (tier_cursor->slow_vaddr == 
                    (unsigned long)page_address(shuffled_pages[i])) {
                //if (i < 100) {
                //printk("mtier: page 0x%lx already in fast tier.\n",
                //        (unsigned long)page_address(shuffled_pages[i]));
                //}
                tier_cursor->fast_in_use = 1;
                shuffled_pages[i] = NULL;
                found = 1;
                stay_cnt++;
                break;
            }
            if (j > 10000) {
                printk("force break: i: %d tc: %p slow: 0x%lx pg: 0x%lx\n", i,
                        tier_cursor,
                        (unsigned long)tier_cursor->slow_vaddr,
                        (unsigned long)page_address(shuffled_pages[i]));
                break;
            }
            j++;
        }
    }
    printk("mtier: stay_entries: %d\n", stay_cnt);

    free_cnt = 0;
    INIT_LIST_HEAD(&(free_entries.list));
    for(i = 0; i < ft_size_pages; i++) {
        if ( tier_entries[i].fast_in_use == 0 ) {
            list_add( &(tier_entries[i].list), &(free_entries.list) );
            free_cnt++;
        }
    }
    printk("mtier: free_entries: %d\n", free_cnt);

    /* Swap the mapping for unused pages back to the slow tier to prevent them
     * from, you know, not existing anymore. */
    i = 0;
    list_for_each(cur, &(free_entries.list)) {
        struct tier_struct *entry;
        /*
        if ((i % 1000)==0) {
            printk("i: %d\n", i);
        }
        */
#if 1
        entry = list_entry (cur, struct tier_struct, list);
        if (entry->slow_page != NULL) {
            int swaprv = 0;
            //if (i < 100) {
            //printk("  swpb: %p %p\n", entry->slow_page, entry->fast_page);
            //}
            swaprv = swap_mapping_back(entry->slow_page, entry);
            if(swaprv) {
                printk("mtier: error migrating page back.\n");
            }
            i++;
        }
#endif
    }
    printk("mtier: swap_back: %d\n", i);
    
#if 0
    for(i = 0; i < (SIZE_MB << 20) / PAGE_SIZE; i++) {
        if( (tier_entries[i].fast_in_use == 0) &&
            (tier_entries[i].slow_page != NULL) ) {
            int swaprv = 0;
            if (i < 100) {
            printk("  swpb: %p %p\n", tier_entries[i].slow_page, tier_entries[i].fast_page);
            }
            //swaprv = swap_mapping_back(tier_entries[i].slow_page,
            //        &tier_entries[i]);
            if(swaprv) {
                printk("mtier: error migrating page back.\n");
            }
        }
    }
#endif

    /* Handle page duplication now as needed */
    printk("starting the swap loop\n");
    swap_cnt = 0;
    hash_init(side_tbl);
    for(i = 0; i < cutoff; i++) {
        ktime_t start, end;
        struct tier_struct *entry;

#if 1
        if ((i % 10)==0) {
            printk("  i: %d\n", i);
        }
#endif

        /* Ignore pages marked NULL above; they're already where they need to
         * be. */
        if(!shuffled_pages[i]) {
            continue;
        }

        entry = list_first_entry_or_null(&(free_entries.list), struct tier_struct, list);
        if (entry == NULL) {
            printk("no more free entries: %d\n", i);
            break;
        }
        list_del(&(entry->list));

#if 0
        /* Get a free fast page. If this fails, just bail out of this entire
         * loop, since there won't be a free page for any other iteration,
         * either. */
        if((entry = get_free_fast_entry()) == NULL) {
            printk("mtier: fast tier must be full; no free entry found.\n");
            break;
        }
#endif

        /* Mark the fast page as used and perform the migration */
        /*
        printk("mtier: entry: %p migrating 0x%lx (v 0x%lx) to 0x%lx\n",
                entry,
                (unsigned long)page_to_phys(shuffled_pages[i]),
                (unsigned long)page_address(shuffled_pages[i]),
                (unsigned long)page_address(entry->fast_page));
        */
        entry->fast_in_use = 1;
        entry->slow_vaddr = (unsigned long)page_address(shuffled_pages[i]);
        hash_add(side_tbl, &(entry->hl_node),
                 MTIER_HASH_KEY(page_address(shuffled_pages[i])));

        swap_mapping(shuffled_pages[i], entry);
        swap_cnt++;
    }
    printk("mtier: swap_entries: %d\n", swap_cnt);

    ++mod_iter;
    ++tot_iter;
    /*list_for_each_entry_safe(cursor, tmp, pagelist, lru) {
        printk("mtier: iter %d migrating 0x%lx to 0x%lx\n", tot_iter,
                (unsigned long)page_to_phys(tmp),
                (unsigned long)page_to_phys(tier_entries[unused_idx].fast_page));
    }*/
    /*
    list_for_each_entry_safe(cursor, tmp, pagelist, lru) {
        printk("mtier: iter %d migrating 0x%lx (v0x%lx) to 0x%lx,"
                " mm=0x%lx\n", tot_iter, 
                (unsigned long)page_to_phys(tmp),
                (unsigned long)page_address(tmp),
                0,
                (unsigned long)mm);
        //swap_mapping(mm, cursor);
        ++pages_moved;
        if(pages_moved == iter_pages) {
            break;
        }
        else if(list_empty(pagelist)) {
            printk("mtier: list exhausted!\n");
        }
        list_del(&cursor->lru);
    }*/
#if 0
        for(pages_moved = 0; pages_moved < iter_pages && 
                !list_empty(pagelist); ++pages_moved) {
            tmp = list_first_entry(pagelist, struct page, lru);
            /*printk("mtier: iter %d migrating 0x%lx (v 0x%lx) to 0x%lx, mm=0x%lx\n", tot_iter,
                    (unsigned long)page_to_phys(tmp),
                    (unsigned long)page_address(tmp),
                    (unsigned long)page_to_phys(tier_entries[unused_idx].fast_page),
                    (unsigned long)mm);*/
            swap_mapping(mm, tmp);
            list_del(&tmp->lru);
            if(list_empty(pagelist)) {
                printk("mtier: list empty :-(\n");
            }
        }
#endif
    
    /* Test walk process VMAs for addr. range confirmation */

    /* This needs to be the last thing the module does. */
    mmput(mm);
    //++mod_iter;
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
    ktime_t call_end;
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
                printk("tiering-eligible process: %lu\n", 
                        (unsigned long)tsk->pid);
                pid = tsk->pid;
                call_start = ktime_get();
                //pages = mod_do_mtier_management(pid, numpages, old, new, 2);
                pages = mod_mtier_duplicate(tsk);
                call_end = ktime_get();
                printk("mod_mtier: moved %d pages in %lu ns\n", pages, 
                        (unsigned long)call_end.tv64 - (unsigned long)call_start.tv64);
            }
        }
    }
    
    printk("mtier: worker thread termination\n");
    return counter;
}

static int mod_mtier_init(void) {
    int curr_node, i;

    pagelist = (struct list_head *)kmalloc(sizeof(struct list_head),
            GFP_KERNEL);
    INIT_LIST_HEAD(pagelist);

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
            printk("Node %d breaking (not %d)\n", curr_node, fast_node);
            continue;
        }
        printk("Node %d reporting!\n", curr_node);
        for(i = 0; i < PAGES(SIZE_MB); i++) {
            tier_entries[i].owner = 0;
            tier_entries[i].fast_in_use = 0;
            tier_entries[i].fast_valid = 0;
            tier_entries[i].slow_page = NULL;
            tier_entries[i].fast_page = alloc_page_interleave(
                    (gfp_t *)GFP_HIGHUSER_MOVABLE, 0, curr_node);
            if(!tier_entries[i].fast_page) {
                goto oom;
            }
            //printk("mtier: fast page paddr=0x%lx\n",
            //        (unsigned long)page_to_phys(tier_entries[i].fast_page));
        }
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

    if(thr_rv != -EINTR) {
        printk("mtier: worker thread has stopped.\n");
    }

    if (!list_empty(pagelist)) {
        putback_movable_pages(pagelist);
    }

    /* BEGIN REPLICATION FREE CODE */
    /* Because of the way the module claims pages, we have to force them to an
     * unused status before we can free them, or else we get page dump/charge
     * errors. Because of this:
     * DO NOT REMOVE THE MODULE WHILE A TIERING PROCESS IS RUNNING! In the
     * future, this should be changed to allow clean swap backs to the slow
     * tier for any running process. */
    for(i = 0; i < PAGES(SIZE_MB); i++) {
        tier_entries[i].fast_page->mem_cgroup = NULL;
        tier_entries[i].fast_page->mapping = NULL;
        clear_bit(PG_active, &(tier_entries[i].fast_page)->flags);
        __free_pages(tier_entries[i].fast_page, 0);
    }

    if (shuffled_pages != NULL) {
        vfree(shuffled_pages);
    }
    /* END REPLICATION FREE CODE */

    printk("mtier: module exit with count %d\n", counter);
}

module_exit(mod_mtier_exit);
