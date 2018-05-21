#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
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
    long rv, pts;
    pid_t child, wrv;

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
        {0, 0, 0, 0}
    };

    if( argc < 2 ) {
        fprintf( stderr, "usage: mtier flags program options\n" );
        return -1;
    }

    while( 1 ) {
        int option_index = 0;
        c = getopt_long( argc, argv, "l:f:t:n:r",
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


    /* Make sure everything is being parsed correctly */
#if 0
    printf( "Options index is %d\n", optind );
    printf( "Options to new program are:\n" );
    for( i = optind; i < argc; i++ ) {
        printf( "\t%d: %s\n", i, argv[i] );
    }
#endif
    /* Fork a child. */
#if 0
    printf( "Forking...\n" );
#endif
    child = fork();

    /* CHILD */
    if( child == 0 ) {
        /* Delay for testing purposes */
#if 0
        printf( "Child here!\n" );
#endif
        
        /* Exec the requested program; handle errors and exit
         * as needed */
        execvp( argv[optind], &argv[optind] );
        perror( argv[optind] );
        exit( -1 );
    }
    /* PARENT */
    else {
        /* Attach a ptrace tracer to the child so we can intercept and 
         * handle certain signals on its behalf. */
        printf( "Child spawned with pid %ld!\n", (long)child );
        printf( "Attaching tracer to child... " );
        if( ( pts = ptrace( PTRACE_ATTACH, child, 0, 0 ) ) == -1 ) {
            printf( "error attaching tracer: %d\n", errno );
            return -1;
        }
        else {
            printf( "success!\n" );
        }

        /* Enqueu the child process in the mtier framework */
        MTIER_ENQUEUE(rv, child);
        if( rv != 0 ) {
            fprintf( stderr, "Error enqueuing process. Return code from syscall was %ld\n", rv );
        }
        /* Handle events that occur when something happens with the child */
        while( ( wrv = waitpid( child, &wstatus, 0 ) ) ) {
            /* The first three options handle normal and abnormal child exits.
             * We want to make sure this process terminates correctly when the
             * child terminates,since its life has no purpose if its child
             * dies. */
            if( WIFEXITED( wstatus ) ) {
                printf( "Child exited normally.\n" );
                MTIER_DEQUEUE( rv, child );
                return 0;
            }
            else if( WIFSIGNALED( wstatus ) ) {
                printf( "Child terminated by signal.\n" );
                MTIER_DEQUEUE( rv, child );
                return 0;
            }
#ifdef WCOREDUMP
            else if( WCOREDUMP( wstatus ) ) {
                printf( "Child terminated by segmentation fault.\n" );
                MTIER_DEQUEUE( rv, child );
                return 0;
            }
#endif
            /* Otherwise, if the parent gets stopped by a signal, catch
             * it and, if it's SIGUSR1 or 2, handle it. Otherwise, don't 
             * override the default behavior. */
            else if( WIFSTOPPED( wstatus ) ) {
                sig = WSTOPSIG( wstatus );

                switch( sig ) {
                    case SIGSEGV:
                        printf( "Segmentation fault! Aborting.\n" );
                        MTIER_DEQUEUE( rv, child );
                        return -1;
                    case SIGUSR1:
                        printf( "SIGUSR1\n" );
                        break;
                    case SIGUSR2:
                        printf( "SIGUSR2\n" );
                        break;
                    default:
                        printf( "Signal %d\n", sig );
                        break;
                }
            }

            /* Restart the child */
            if( ( pts = ptrace( PTRACE_CONT, child, 0, 0 ) ) == -1 ) {
                printf( "Error continuing trace: %d\n", errno );
                return -1;
            }
        }
    }

    return 0;
}   
