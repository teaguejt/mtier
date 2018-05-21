#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <numa.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <bits/signum.h>
#include <linux/kernel.h>
#include "mtier.h"

static int verbose_flag = 0;
static int lookup_flag  = 1;
static char *time_file  = NULL;
extern int opterr;

int main( int argc, char **argv ) {
    int c, i, wstatus, exit_status, need_usage, sig;
    int willList = 0;
    int willKill = 0;
    int proceed  = 1;
    int slowNode = 0;
    int fastNode = 1;
    int sysconst = 0;
    int mig      = 0;
    unsigned int num_mtier_migrations = 0;
    unsigned int num_stock_migrations = 0;
    long rv, pts;
    unsigned long nPages = 100;
    unsigned long ftSize = (2 << 20) / PAGE_SIZE;
    unsigned long migrated = 0;
    unsigned long total_mtier_migrated = 0, total_stock_migrated = 0;
    /* All these doubles have to do with timing stock and mtier migrations. */
    double a_migration;
    double longest_mtier_migration = 0, longest_stock_migration = 0;
    double shortest_mtier_migration = 0, shortest_stock_migration = 0;
    double average_mtier_migration = 0, average_stock_migration = 0;
    pid_t child, wrv;
    struct timespec delay = { 0, 500 * NS_TO_MS };
    struct timeval begin, end;
    /* Handle args. I'm not gonna lie, this seems pretty arcane, 
     * and you should read the getopt docs to see exactly what's
     * going on here. */
    c = need_usage = 0;
    
    static struct option long_options[] =
    {
        {"verbose",     no_argument,        &verbose_flag,  1},
        {"help",        no_argument,        0,              'h'},
        {"list",        no_argument,        0,              'l'},
        {"remove",      no_argument,        0,              'r'},
        {"millisecs",   required_argument,  0,              'm'},
        {"secs",        required_argument,  0,              's'},
        {"pages",       required_argument,  0,              'p'},
        {"ftsize",      required_argument,  0,              'z'},
        {"ftnode",      required_argument,  0,              'f'},
        {"stnode",      required_argument,  0,              'w'},
        {"testcalls",   no_argument,        0,              't'},
        {"copy",        no_argument,        0,              'c'},
        {0, 0, 0, 0}
    };

    if( argc < 2 ) {
        fprintf( stderr, "usage: mtier flags program options\n" );
        return -1;
    }

    while( 1 ) {
        int option_index = 0;
        c = getopt_long( argc, argv, "l:f:t:m:r:s:p:z:f:w:c",
                        long_options, &option_index );
       
        /* Detect the end of the options */
        if( c == -1 )
            break;

        switch( c ) {
            case 0:
                if( long_options[option_index].flag != 0 ) {
                }
                break;

            case 'h': 
            case '?': need_usage        = 1;        break;
            case 'l': willList          = 1;        break;
            case 'r': willKill          = 1;        break;
            case 'm':
                delay.tv_nsec = (atol(optarg) * NS_TO_MS);
                break;
            case 's':
                delay.tv_sec = (time_t)atoi( optarg );
                break;
            case 'p':
                nPages = atol( optarg );
                break;
            case 'z':
                ftSize = atol(optarg);
                ftSize = (ftSize << 20) / PAGE_SIZE;
                break;
            case 'f':
                fastNode = atoi(optarg);
                break;
            case 'w':
                slowNode = atoi(optarg);
                break;
            case 't':
                /* MTIER_COPY(rv, getpid()); */
                printf("%d\n", rv);
                return 0;
            case 'c':
                sysconst = 1;
                break;
              
            default: return -1;
        }
    }

    /* If the user just wants a list of processes */
    if( willList ) {
        /* Handle issues */
        if (argc != 2 ) {
            printf( "usage: mtier --l(ist)\n" );
            return -1;
        }

        /* Get the actual list */
        MTIER_LIST( rv );
        if( rv != 0 ) {
            fprintf( stderr, "mtier_list returned nonzero value: %d\n", rv );
        }
        else {
            printf( "Check dmesg for process list.\n" );
        }

        return rv;
    }

    /* If the user wants to dequeue a process */
    if( willKill ) {
        int i;

        /* Handle invalid input */
        if( argc < 3 ) {
            printf( "usage: mtier --r(emove) <pid0, pid1, ..., pidn>\n" );
            return -1;
        }

        for( i = 2; i < argc; i++ ) {
            child = (pid_t)atoi( argv[i] );
            if( child < 1 || child > ( 2 << 16 ) ) {
                printf( "%ld is an invalid pid. Skipping.\n", (long)child );
                continue;
            }

            MTIER_DEQUEUE( rv, child );
            if( rv != 0 ) {
                fprintf( stderr, "mtier dequeue function returned nonzero "
                        "value %d. Aborting...\n", rv );
                return -1;
            }
        }
        return 0;
    }
    if(verbose_flag) {
      printf("mtier settings:\nslow node: %d\nfast node: %d\nfast size: %lu\n",
            slowNode, fastNode, ftSize, (unsigned long)delay.tv_sec);
      printf("seconds: %lu\nmilliseconds: %lu\npages per run:%d\n",
            (unsigned long)delay.tv_sec,
            ((unsigned long)delay.tv_nsec / 1000000), nPages);
      printf( "Forking...\n" );
    }
    child = fork();

    /* CHILD */
    if( child == 0 ) {
        /* Exec the requested program; handle errors and exit
         * as needed */
        execvp( argv[optind], &argv[optind] );
        perror( argv[optind] );
        exit( -1 );
    }
    /* PARENT */
    else {
        /* Set up bitmasks for the migration framework. */
        int maxnode;
        struct bitmask *old, *new;
        old = numa_allocate_nodemask();
        new = numa_allocate_nodemask();
        maxnode = numa_max_node() + 1;
        numa_bitmask_setbit( old, slowNode );
        numa_bitmask_setbit( new, fastNode );

        /* Enqueue the child process in the mtier framework
         * Even if the framework isn't being used as initially intended, this
         * allows for a clean exit of the parent upon child termination */
        MTIER_ENQUEUE(rv, child);
        if( rv != 0 ) {
            fprintf( stderr, "Error enqueuing process. Return code from syscall was %ld\n", rv );
        }
        /* Handle events that occur when something happens with the child */
        migrated = 0;
        while( proceed ) {
            wrv = waitpid( child, &wstatus, WNOHANG );
            /* Handle potential abnormal exits and such here */
            if( wrv != 0 ) {
                if( WIFEXITED( wstatus ) ) {
                    printf( "Child exited normally.\n" );
                    total_mtier_migrated += migrated;
                    proceed = 0;
                    break;
                }
                else if( WIFSIGNALED( wstatus ) ) {
                    printf( "Child terminated by signal.\n" );
                    proceed = 0;
                    total_mtier_migrated += migrated;
                    break;
                }
#ifdef WCOREDUMP
                else if( WCOREDUMP( wstatus ) ) {
                    printf( "Child terminated by core dump.\n" );
                    proceed = 0;
                    break;
                }
#endif
                else if( WIFSTOPPED( wstatus ) ) {
                    sig = WSTOPSIG( wstatus );
                    switch( sig ) {
                        case SIGSEGV:
                            printf( "Child terminated by segfault. Learn to "
                                    "malloc, n00b.\n" );
                            proceed = 0;
                            break;
                        case SIGUSR1:
                            printf( "User sig 1 intercepted.\n" );
                            break;
                        case SIGUSR2:
                            printf( "User sig 2 intercepted.\n" );
                            break;
                    }
                }
                break;
            }
            if(!mig) {
              nanosleep( &delay, NULL );
              if(migrated >= ftSize) {
                printf("fast tier full. Performing stock migration.\n");
                total_mtier_migrated += migrated;
                migrated = 0;
                gettimeofday(&begin, NULL);
                migrate_pages(child, maxnode, new->maskp, old->maskp);
                gettimeofday(&end, NULL);
                a_migration = ((double)(end.tv_sec - begin.tv_sec) +
                    (double)(end.tv_usec - begin.tv_usec) / 1000000);
                average_stock_migration += a_migration;
                if(a_migration > longest_stock_migration)
                  longest_stock_migration = a_migration;
                if(shortest_stock_migration == 0 ||
                    a_migration < shortest_stock_migration)
                  shortest_stock_migration = a_migration;
                ++num_stock_migrations;
                continue;
              }

              gettimeofday(&begin, NULL);
              if(!sysconst) {
                printf("masks: %lu %lu\n", old->maskp, new->maskp);
                MTIER_MANAGE( rv, child, nPages, old->maskp, new->maskp,
                    maxnode );
              }
              else {
                MTIER_COPY(rv, child, nPages, old->maskp, new->maskp, maxnode);
                //STOCK_MIGRATE(rv, child, old->maskp, new->maskp, maxnode);
                printf("duplication handler returned %d for child %d\n", rv,
                    (int)child);
              }
              gettimeofday(&end, NULL);
              /* If the system call indicates that zero pages were migrated,
               * don't do anything to affect the average. We don't need to time
               * what essentially amounts to a return statement. */
              /* This is solely benchmarking code and probably won't be used in
               * the final version of this system, assuming this goes 
               * anywhere. */
              if(rv < 0) {
                printf("mtier error: manage returned %ld\n", rv);
                break;
              }
              else if(rv >= 0) {
                a_migration = ((double)(end.tv_sec - begin.tv_sec) +
                    (double)(end.tv_usec - begin.tv_usec) / 1000000);
                average_mtier_migration += a_migration;
                if(a_migration > longest_mtier_migration)
                  longest_mtier_migration = a_migration;
                if(shortest_mtier_migration == 0 
                    || a_migration < shortest_mtier_migration)
                  shortest_mtier_migration = a_migration;
                ++num_mtier_migrations;
              }
              migrated += rv;
              mig = 1;
            }
        }

        /* Calculate and display summary information */
        average_mtier_migration /= num_mtier_migrations;
        average_stock_migration /= num_stock_migrations;
        printf("SUMMARY\n=======\n");
        printf("shortest mtier migration: %.6f s\n", shortest_mtier_migration);
        printf("longest mtier migration: %.6f s\n", longest_mtier_migration);
        printf("average mtier migration: %.6f s (%u nonzero calls)\n",
          average_mtier_migration, num_mtier_migrations);
        printf("shortest stock migration: %.6f s\n", shortest_stock_migration);
        printf("longest stock migration: %.6f s\n", longest_stock_migration);
        printf("averahe stock migration: %.6f s (%u nonzero calls)\n",
          average_stock_migration, num_stock_migrations);
        printf("total bytes: %lu\n", total_mtier_migrated * PAGE_SIZE);
        printf("total pages: %lu\n", total_mtier_migrated);
        MTIER_DEQUEUE( rv, child );
    }

    return 0;
}   
