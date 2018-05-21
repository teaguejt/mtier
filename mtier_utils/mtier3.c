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

int main(int argc, char **argv) {
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

    if(argc < 2) {
        fprintf(stderr, "usage: mtier flags program options\n");
        return -1;
    }

    while(1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "l:f:t:m:r:s:p:z:f:w:c",
                        long_options, &option_index);
       
        /* Detect the end of the options */
        if(c == -1)
            break;

        switch(c) {
            case 0:
                if(long_options[option_index].flag != 0) {
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
                delay.tv_sec = (time_t)atoi(optarg);
                break;
            case 'p':
                nPages = atol(optarg);
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
    if(willList) {
        /* Handle issues */
        if (argc != 2) {
            printf("usage: mtier --l(ist)\n");
            return -1;
        }

        /* Get the actual list */
        MTIER_LIST(rv);
        if(rv != 0) {
            fprintf(stderr, "mtier_list returned nonzero value: %d\n", rv);
        }
        else {
            printf("Check dmesg for process list.\n");
        }

        return rv;
    }

    /* If the user wants to dequeue a process */
    if(willKill) {
        int i;

        /* Handle invalid input */
        if(argc < 3) {
            printf("usage: mtier --r(emove) <pid0, pid1, ..., pidn>\n");
            return -1;
        }

        for(i = 2; i < argc; i++) {
            child = (pid_t)atoi(argv[i]);
            if(child < 1 || child > (2 << 16)) {
                printf("%ld is an invalid pid. Skipping.\n", (long)child);
                continue;
            }

            MTIER_DEQUEUE(rv, child);
            if(rv != 0) {
                fprintf(stderr, "mtier dequeue function returned nonzero "
                        "value %d. Aborting...\n", rv);
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
      printf("Forking...\n");
    }
    child = fork();

    /* CHILD */
    if(child == 0) {
        /* Exec the requested program; handle errors and exit
         * as needed */
        execvp(argv[optind], &argv[optind]);
        perror(argv[optind]);
        exit(-1);
    }
    /* PARENT */
    else {
        /* Set up bitmasks for the migration framework. */
        int maxnode;
        int status;
        struct bitmask *old, *new;
        old = numa_allocate_nodemask();
        new = numa_allocate_nodemask();
        maxnode = numa_max_node() + 1;
        numa_bitmask_setbit(old, slowNode);
        numa_bitmask_setbit(new, fastNode);

        /* Enqueue the child process in the mtier framework
         * Even if the framework isn't being used as initially intended, this
         * allows for a clean exit of the parent upon child termination */
        MTIER_ENQUEUE(rv, child);
        if(rv != 0) {
            fprintf(stderr, "Error enqueuing process. Return code from syscall was %ld\n", rv);
        }
        /* Handle events that occur when something happens with the child */
        migrated = 0;
        wait(&status);
        /* Calculate and display summary information */
        average_mtier_migration /= num_mtier_migrations;
        average_stock_migration /= num_stock_migrations;
        printf("child pid is %d\n", (int)child);
        MTIER_DEQUEUE(rv, child);
    }

    return 0;
}   
