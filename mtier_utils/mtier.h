#ifndef __MTIER_H__
#define __MTIER_H__

#include <sys/types.h>
#include <inttypes.h>

uint64_t get_phys_addr(uint64_t);

#define NS_TO_MS (1E6L)

#define __NR_mtier_enqueue_process      326
#define __NR_mtier_list                 327
#define __NR_mtier_kill_process         328
#define __NR_do_mtier_management        329
#define __NR_mtier_duplication_handler  330
#define __NR_migrate_pages              256

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PTR_TO_PAGE( ptr ) \
  ((unsigned long)ptr & ~(PAGE_SIZE - 1))

#define MTIER_ENQUEUE(rv, pid) \
	(rv) = (long)syscall( __NR_mtier_enqueue_process, (pid_t)(pid) );

#define MTIER_LIST(rv) \
    (rv) = (long)syscall( __NR_mtier_list );

#define MTIER_DEQUEUE(rv, pid) \
    (rv) = (long)syscall( __NR_mtier_kill_process, (pid_t)(pid) );

#define MTIER_MANAGE(rv, pid, pages, nold, nnew, maxnode) \
    (rv) = (long)syscall( __NR_do_mtier_management, (pid_t)(pid), \
        (unsigned long)(pages), (unsigned long *)(nold), \
        (unsigned long *)(nnew), (unsigned long)maxnode );

#define MTIER_COPY(rv, pid, pages, nold, nnew, maxnode) \
    (rv) = (long)syscall(__NR_mtier_duplication_handler, (pid_t)pid,  \
        (unsigned long)(pages), (unsigned long *)(nold),              \
        (unsigned long *)(nnew), (unsigned long)maxnode);

#define STOCK_MIGRATE(rv, pid, nold, nnew, maxnode) \
    (rv) = (long)syscall( __NR_migrate_pages, (pid_t)(pid), \
        (unsigned long)(maxnode), (unsigned long *)(nold), \
        (unsigned long *)(nnew) );
  

#endif 
