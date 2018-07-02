#include <linux/mtier.h>
#include <linux/bitops.h>
#include <linux/page-flags.h>
#include <linux/page_ext.h>
#include <linux/semaphore.h>
#include <asm/tlbflush.h>

struct list_head mtier_process_list;
LIST_HEAD( mtier_process_list );
struct list_head *mtier_curr = &mtier_process_list;
static int count = 0;
static int bigCount = 0;
DEFINE_SEMAPHORE( mtier_sem );
DEFINE_HASHTABLE(mtier_process_ht, MTIER_HT_BITS);

/* Exported Symbols */
#ifdef CONFIG_MTIER
bool mtier_page_is_young(struct page *page) {
    return false;
}
EXPORT_SYMBOL(mtier_page_is_young);

void mtier_set_page_young(struct page *page) {
    /* SetPageYoung(page); */
}
EXPORT_SYMBOL(mtier_set_page_young);

bool mtier_page_is_idle(struct page *page) {
    return false;
}
EXPORT_SYMBOL(mtier_page_is_idle);

void mtier_set_page_idle(struct page *page) {
    /* SetPageIdle(page); */
}
EXPORT_SYMBOL(mtier_set_page_idle);

int mtier_trylock_page(struct page *page)
{
	return (likely(!test_and_set_bit_lock(PG_locked, &page->flags)));
}
EXPORT_SYMBOL(mtier_trylock_page);
#endif

/* SYSTEM CALLS */
asmlinkage long sys_mtier_enqueue_process( pid_t pid ) {
#ifdef CONFIG_MTIER
    int bkt;
    struct tierable_process *tmp  = NULL;
    struct task_struct *task      = NULL;
    struct pid         *spid      = NULL;
    struct tierable_process *proc = NULL;

    printk( "mtier: enqueue process with pid %ld\n", (long)pid );
    printk( "mtier: enqueue called from pid %ld\n", (long)(current->pid) );
    printk( "mtier: checking for task... " );

    /* Get the pid struct from the pid_t */
    spid = find_vpid( pid );
    if( spid == NULL ) {
        printk( "pid struct is null. Aborting.\n" );
        return -1;
    }

    /* Get the task struct from the pid struct */
    task = pid_task( spid, PIDTYPE_PID );
    if( task == NULL ) {
        printk( "task struct is null. Aborting.\n" );
        return -2;
    }
    printk( "found task_struct with pid %ld and addr 0x%p\n",
            (long)( task->pid ), (void *)task );
    printk( "mtier: task stack located at virt. addr. 0x%lx\n",
            task->mm->start_stack );

    /* Allocate a tierable process struct to store task information and the
     * timer for this process. */
    proc = kmalloc( sizeof( struct tierable_process ), GFP_KERNEL );
    if( !proc ) {
        printk( "mtier: could not allocate memory for struct. Aborting.\n" );
        return -3;
    }

    /* Make sure the pid and the task pid match */
    if( pid != task->pid ) {
        printk( "mtier: mismatch between pid and task pid. Aborting.\n" );
        return -4;
    }
    
    /* Populate struct fields */
    proc->pid = task->pid;
    proc->task = task;
    proc->process_page_tree = RB_ROOT;
    down( &mtier_sem );
    hash_add_rcu(mtier_process_ht, &proc->mtp, (unsigned long)task);
    list_add( &(proc->list), &mtier_process_list );
    task->mm->mtier_rb = &(proc->process_page_tree);
    up( &mtier_sem );
    printk( "mtier: struct created: pid %ld\taddr 0x%lx\n", (long)( proc->pid ),
            (unsigned long)( proc->task ) );
    
    ++count;

    printk("mtier: hash table now consists of:\n");
    hash_for_each_rcu(mtier_process_ht, bkt, tmp, mtp) {
      printk("%d: %lu 0x%lx\n", bkt, (unsigned long)tmp->pid,
        (unsigned long)tmp->task);
    }
    sema_init(&task->exit_sem, 1);
    task->should_tier = 1;
#else
    printk( "memory tiering not enabled in kernel\n" );
#endif  /* CONFIG_MTIER */
    return 0;
}

asmlinkage long sys_mtier_list( void ) {
#ifdef CONFIG_MTIER
    int c = 0;
    struct list_head *pos;
    struct tierable_process *tmp;
    
    printk( "mtier: list eligible processes\n" );
    list_for_each( pos, &mtier_process_list ) {
        tmp = list_entry( pos, struct tierable_process, list );
        printk( "%d: pid %ld\ttask addr: 0x%p\n", c, (long)( tmp->pid ),
                (void *)tmp->task );
        ++c;
    }
#else 
    printk( "memory tiering not enabled in kernel\n" );
#endif  /* CONFIG_MTIER */
    return 0;
}

/* Okay, so this doesn't really kill the process - it dequeues it, stops
 * manipulating its memory, and allows it to die naturally. */
asmlinkage long sys_mtier_kill_process( pid_t pid ) {
#ifdef CONFIG_MTIER
    int found = 0;
    struct list_head *pos, *tmp;
    struct tierable_process *target;

    printk( "mtier: dequeue process\n" );
    down( &mtier_sem );
    list_for_each_safe(pos, tmp, &mtier_process_list ) {
        target = list_entry( pos, struct tierable_process, list );
        if( target->pid == pid ) {
            /* Prevent panics for great justice */
            if( mtier_curr == pos ) {
                mtier_curr = mtier_curr->next;
            }
            /* Delete the hash table entry */
            hash_del_rcu(&target->mtp);
            list_del( pos );
            kfree( target );
            found = 1;
            break;
        }
    }
    up( &mtier_sem );
    if( found ) {
        printk( "mtier: found and deleted requested process (pid %ld).\n",
                (long)pid );
    }
    else {
        printk( "mtier: could not delete requested process (pid %ld). "
                "Proces not found in mtier list.\n", (long)pid );
    }
#else
    printk( "memory tiering not enabled in kernel\n" );
#endif  /* CONFIG_MTIER */
    return 0;
}

/* Create a new mapping info struct and insert it into a tree. */
struct mtier_mapping_info *mtier_new_mapping_info(unsigned long vaddr,
    struct tierable_process *tier_entry)
{
  struct mtier_mapping_info *rs = (struct mtier_mapping_info *)NULL;
  rs = kmalloc(sizeof(struct mtier_mapping_info), GFP_KERNEL);
  if(!rs)
    return rs;
  
  rs->vaddr = vaddr;
  rs->is_mapped = 0;
  rs->is_fast   = 0;

  if(__mtier_insert_tree_node(&tier_entry->process_page_tree, rs) == -1) {
    kfree(rs);
    rs = (struct mtier_mapping_info *)NULL;
    return rs;
  }

  return rs;
  
}

/* Find and insert functions for the red-black trees in use */
/* Insert an existing mapping info into the tree */
int __mtier_insert_tree_node(struct rb_root *root, 
    struct mtier_mapping_info *info)
{
  struct rb_node **new = &(root->rb_node);
  struct rb_node *parent = NULL;
  struct mtier_mapping_info *tmp_info;

  while(*new) {
    tmp_info = container_of(*new, struct mtier_mapping_info, page_tree);
    if(info->vaddr < tmp_info->vaddr)
      new = &((*new)->rb_left);
    else if(info->vaddr > tmp_info->vaddr)
      new = &((*new)->rb_right);
    else
      return -1;
  }

  rb_link_node(&info->page_tree, parent, new);
  rb_insert_color(&info->page_tree, root);

  return 0;
}

/* See if a virtual address has been entered into the system, and get a 
 * pointer to it if it has. */
struct mtier_mapping_info *__mtier_find_tree_node(struct rb_root *root,
    unsigned long target)
{
  struct rb_node **tmp_node = &(root->rb_node);
  struct mtier_mapping_info *tmp_info;
  struct mtier_mapping_info *rs = (struct mtier_mapping_info *)NULL;
  
  while(*tmp_node) {
    tmp_info = container_of(*tmp_node, struct mtier_mapping_info, page_tree);
    if(target < tmp_info->vaddr)
      tmp_node = &((*tmp_node)->rb_left);
    else if(target > tmp_info->vaddr)
      tmp_node = &((*tmp_node)->rb_right);
    else {
      rs = tmp_info;
      break;
    }
  }

  return rs;
}

/* New function to protect static and inline keywords in original */
#define __flush_tlb() __native_flush_tlb() 

static inline void __flush_tlb_up(void)
{
	count_vm_tlb_event(NR_TLB_LOCAL_FLUSH_ALL);
	__flush_tlb();
}

void mtier_flush_tlb_mm(struct mm_struct *mm)
{
    flush_tlb_mm(mm);
}
EXPORT_SYMBOL(mtier_flush_tlb_mm);
