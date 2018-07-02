/*-----------------------------------------------------------------------*/
/* Program: STREAM                                                       */
/* Revision: $Id: stream.c,v 5.10 2013/01/17 16:01:06 mccalpin Exp mccalpin $ */
/* Original code developed by John D. McCalpin                           */
/* Programmers: John D. McCalpin                                         */
/*              Joe R. Zagar                                             */
/*                                                                       */
/* This program measures memory transfer rates in MB/s for simple        */
/* computational kernels coded in C.                                     */
/*-----------------------------------------------------------------------*/
/* Copyright 1991-2013: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*           "tuned STREAM benchmark results"                            */
/*           "based on a variant of the STREAM benchmark code"           */
/*         Other comparable, clear, and reasonable labelling is          */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/
# include <stdio.h>
# include <unistd.h>
# include <math.h>
# include <float.h>
# include <limits.h>
# include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <numaif.h>
#include <errno.h>

/*-----------------------------------------------------------------------
 * INSTRUCTIONS:
 *
 *	1) STREAM requires different amounts of memory to run on different
 *           systems, depending on both the system cache size(s) and the
 *           granularity of the system timer.
 *     You should adjust the value of 'STREAM_ARRAY_SIZE' (below)
 *           to meet *both* of the following criteria:
 *       (a) Each array must be at least 4 times the size of the
 *           available cache memory. I don't worry about the difference
 *           between 10^6 and 2^20, so in practice the minimum array size
 *           is about 3.8 times the cache size.
 *           Example 1: One Xeon E3 with 8 MB L3 cache
 *               STREAM_ARRAY_SIZE should be >= 4 million, giving
 *               an array size of 30.5 MB and a total memory requirement
 *               of 91.5 MB.  
 *           Example 2: Two Xeon E5's with 20 MB L3 cache each (using OpenMP)
 *               STREAM_ARRAY_SIZE should be >= 20 million, giving
 *               an array size of 153 MB and a total memory requirement
 *               of 458 MB.  
 *       (b) The size should be large enough so that the 'timing calibration'
 *           output by the program is at least 20 clock-ticks.  
 *           Example: most versions of Windows have a 10 millisecond timer
 *               granularity.  20 "ticks" at 10 ms/tic is 200 milliseconds.
 *               If the chip is capable of 10 GB/s, it moves 2 GB in 200 msec.
 *               This means the each array must be at least 1 GB, or 128M elements.
 *
 *      Version 5.10 increases the default array size from 2 million
 *          elements to 10 million elements in response to the increasing
 *          size of L3 caches.  The new default size is large enough for caches
 *          up to 20 MB. 
 *      Version 5.10 changes the loop index variables from "register int"
 *          to "ssize_t", which allows array indices >2^32 (4 billion)
 *          on properly configured 64-bit systems.  Additional compiler options
 *          (such as "-mcmodel=medium") may be required for large memory runs.
 *
 *      Array size can be set at compile time without modifying the source
 *          code for the (many) compilers that support preprocessor definitions
 *          on the compile line.  E.g.,
 *                gcc -O -DSTREAM_ARRAY_SIZE=100000000 stream.c -o stream.100M
 *          will override the default size of 10M with a new size of 100M elements
 *          per array.
 */
#ifndef STREAM_ARRAY_SIZE
#   define STREAM_ARRAY_SIZE	48000000
#endif

/*  2) STREAM runs each kernel "NTIMES" times and reports the *best* result
 *         for any iteration after the first, therefore the minimum value
 *         for NTIMES is 2.
 *      There are no rules on maximum allowable values for NTIMES, but
 *         values larger than the default are unlikely to noticeably
 *         increase the reported performance.
 *      NTIMES can also be set on the compile line without changing the source
 *         code using, for example, "-DNTIMES=7".
 */
#ifdef NTIMES
#if NTIMES<=1
#   define NTIMES	10
#endif
#endif
#ifndef NTIMES
#   define NTIMES	10
#endif

/*  Users are allowed to modify the "OFFSET" variable, which *may* change the
 *         relative alignment of the arrays (though compilers may change the 
 *         effective offset by making the arrays non-contiguous on some systems). 
 *      Use of non-zero values for OFFSET can be especially helpful if the
 *         STREAM_ARRAY_SIZE is set to a value close to a large power of 2.
 *      OFFSET can also be set on the compile line without changing the source
 *         code using, for example, "-DOFFSET=56".
 */
#ifndef OFFSET
#   define OFFSET	0
#endif

/*
 *	3) Compile the code with optimization.  Many compilers generate
 *       unreasonably bad code before the optimizer tightens things up.  
 *     If the results are unreasonably good, on the other hand, the
 *       optimizer might be too smart for me!
 *
 *     For a simple single-core version, try compiling with:
 *            cc -O stream.c -o stream
 *     This is known to work on many, many systems....
 *
 *     To use multiple cores, you need to tell the compiler to obey the OpenMP
 *       directives in the code.  This varies by compiler, but a common example is
 *            gcc -O -fopenmp stream.c -o stream_omp
 *       The environment variable OMP_NUM_THREADS allows runtime control of the 
 *         number of threads/cores used when the resulting "stream_omp" program
 *         is executed.
 *
 *     To run with single-precision variables and arithmetic, simply add
 *         -DSTREAM_TYPE=float
 *     to the compile line.
 *     Note that this changes the minimum array sizes required --- see (1) above.
 *
 *     The preprocessor directive "TUNED" does not do much -- it simply causes the 
 *       code to call separate functions to execute each kernel.  Trivial versions
 *       of these functions are provided, but they are *not* tuned -- they just 
 *       provide predefined interfaces to be replaced with tuned code.
 *
 *
 *	4) Optional: Mail the results to mccalpin@cs.virginia.edu
 *	   Be sure to include info that will help me understand:
 *		a) the computer hardware configuration (e.g., processor model, memory type)
 *		b) the compiler name/version and compilation flags
 *      c) any run-time information (such as OMP_NUM_THREADS)
 *		d) all of the output from the test case.
 *
 * Thanks!
 *
 *-----------------------------------------------------------------------*/

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

#ifndef STREAM_TYPE
#define STREAM_TYPE double
#endif

struct thr_params {
    unsigned int size_mb;
    unsigned int delay;
    unsigned int bk_iters;
};

struct bind_struct {
    unsigned long addr;
    unsigned int on_fast;
};

/*static STREAM_TYPE	a[STREAM_ARRAY_SIZE+OFFSET],
			b[STREAM_ARRAY_SIZE+OFFSET],
			c[STREAM_ARRAY_SIZE+OFFSET]; */
/* mtier arrays */
STREAM_TYPE *whole_arr;
STREAM_TYPE *a, *b, *c;
STREAM_TYPE d;

/* Thread vars */
pthread_t thread;
pthread_mutex_t stop_mutex;
int thread_should_stop = 0;
int thread_has_stopped = 0;

struct thr_params thr_params = {.size_mb = 92, .delay = 100, .bk_iters = 20};

static double	avgtime[4] = {0}, maxtime[4] = {0},
		mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static char	*label[4] = {"Copy:      ", "Scale:     ",
    "Add:       ", "Triad:     "};

static double	bytes[4] = {
    2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE
    };

extern double mysecond();
extern void checkSTREAMresults();
#ifdef TUNED
/* read-only protection for mtier */
extern void tuned_STREAM_Copy();
extern void tuned_STREAM_Scale(STREAM_TYPE scalar);
extern void tuned_STREAM_Add();
extern void tuned_STREAM_Triad(STREAM_TYPE scalar);
#endif
#ifdef _OPENMP
extern int omp_get_num_threads();
#endif

void *thr_func(void *args) {
    int stop = 0;
    int num_pages;
    int num_bytes;
    int arr_pages;
    int num_elems;
    int iter = 0;
    int has_run = 0;
    int err;
    unsigned long i, r, curr;
    unsigned long arr_addr;
    unsigned long fnode = 2;
    unsigned long snode = 1;
    unsigned long mnode = 4;
    unsigned long *pages;

    /* Get key values */
    num_pages = thr_params.size_mb * 1024 * 1024 / 4096;
    num_elems = STREAM_ARRAY_SIZE * 2;
    num_bytes = STREAM_ARRAY_SIZE * 2 * sizeof(STREAM_TYPE);
    arr_pages = num_bytes / 4096;
    printf("num_pages = %d, num_elems = %d, num_bytes = %d, arr_pages = %d\n",
            num_pages, num_elems, num_bytes, arr_pages);

    /* Configure the page array */
    arr_addr = (unsigned long)whole_arr;
    pages = (unsigned long *)malloc(arr_pages * sizeof(unsigned long));
    if(!pages) {
        printf("page array alloc error.\n");
        goto out_err;
    }

    for(i = 0; i < arr_pages; i++) {
        pages[i] = arr_addr + (i * 4096);
        /*if(i < 10) {
            printf("page: 0x%lx\n", pages[i]);
        }*/
    }
    
    printf("page array = 0x%lx, arr_addr = 0x%lx\n", (unsigned long)pages,
            arr_addr);

    do {
        /* Evacuate the fast tier */
        if(has_run) {
            for(i = 0; i < num_pages; i++) {
                madvise((void *)pages[i], 4096, MADV_DONTNEED);
                err = mbind((void *)pages[i], 4096, MPOL_BIND, &snode, mnode,
                    MPOL_MF_MOVE);
                if(err) {
                    printf("error moving to slow node: %d\n", errno);
                }
            }
        }
        else {
            has_run = 1;
        }

        if(iter == thr_params.bk_iters) {

            iter = 0;
        }
        else {
            /* Randomize the pagelist */
            srand(time(NULL));
            for(i = arr_pages - 1; i > 0; i--) {
                r = rand() % (i + 1);
                curr = pages[r];
                pages[r] = pages[i];
                pages[i] = curr;
            }
            /*for(i = 0; i < 50; i++) {
                printf("0x%lx\n", pages[i]);
            }
            break;*/

            /* Perform the bind */
            for(i = 0; i < num_pages; i++) {
                madvise((void *)pages[i], 4096, MADV_DONTNEED);
                err = mbind((void *)pages[i], 4096, MPOL_BIND, &fnode, mnode,
                    MPOL_MF_MOVE);
                if(err) {
                    printf("mbind error: %d\n", errno);
                    break;
                }
            }
        }
        usleep(thr_params.delay * 1000);
        pthread_mutex_lock(&stop_mutex);
        stop = thread_should_stop;
        pthread_mutex_unlock(&stop_mutex);
    } while(!stop);

    free(pages);
out_err:
    printf("Thread has stopped!\n");
    thread_has_stopped = 1;
}

int
main()
    {
    int			quantum, checktick();
    int			BytesPerWord;
    int			k;
    int i;
    int fd;
    int mp1, mp2;
    ssize_t		j;
    int thr_rv;
    STREAM_TYPE		scalar;
    double		t, times[4][NTIMES];
    int intervals;
    struct timespec time_start, time_end;
    unsigned long us_time;

#pragma omp threadprivate(d)
    
    /* Allocations and protection for mtier */
    printf("malloc: ");

    /* Get a file descriptor and mmap the file! */
    fd = open("/home/jteague6/ramdisk/rand_dat", O_RDONLY);
    if(fd == -1) {
        printf("Error opening file.\n");
        return -1;
    }
    whole_arr = mmap(NULL, 2 * STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE),
            PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    printf("whole_arr = 0x%lx\n", (unsigned long)whole_arr);
    a = (double *)whole_arr;
    b = (double *)(whole_arr + STREAM_ARRAY_SIZE);
    printf("a = 0x%lx, b = 0x%lx\n", (unsigned long)a, (unsigned long)b);
    /*for(i = 0; i < 10; i++) {
        printf("a[%d] = %f; b[%d] = %f\n", i, a[i], i, b[i]);
    }*/
    c = valloc(sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE);
    if(!a || !b || !c) {
        printf("mtier: problem allocating array *sad trombone *\n");
    }
    printf("done. a=0x%lx b=0x%lx c=0x%lx\n", (unsigned long)a,
            (unsigned long)b, (unsigned long)c);
    
    /* Start the thread! */
    thr_rv = pthread_create(&thread, NULL, thr_func, (void *)NULL);
    if(thr_rv) {
        printf("Thread failed to start: code %d\n", thr_rv);
        return -1;
    }
    printf("Thread has started!");
    
    /* --- SETUP --- determine precision and check timing --- */
    printf(HLINE);
    printf("STREAM version $Revision: 5.10 $\n");
    printf(HLINE);
    BytesPerWord = sizeof(STREAM_TYPE);
    printf("This system uses %d bytes per array element.\n",
	BytesPerWord);

    printf(HLINE);
#ifdef N
    printf("*****  WARNING: ******\n");
    printf("      It appears that you set the preprocessor variable N when compiling this code.\n");
    printf("      This version of the code uses the preprocesor variable STREAM_ARRAY_SIZE to control the array size\n");
    printf("      Reverting to default value of STREAM_ARRAY_SIZE=%llu\n",(unsigned long long) STREAM_ARRAY_SIZE);
    printf("*****  WARNING: ******\n");
#endif

    printf("Array size = %llu (elements), Offset = %d (elements)\n" , (unsigned long long) STREAM_ARRAY_SIZE, OFFSET);
    printf("Memory per array = %.1f MiB (= %.1f GiB).\n", 
	BytesPerWord * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024.0),
	BytesPerWord * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024.0/1024.0));
    printf("Total memory required = %.1f MiB (= %.1f GiB).\n",
	(3.0 * BytesPerWord) * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024.),
	(3.0 * BytesPerWord) * ( (double) STREAM_ARRAY_SIZE / 1024.0/1024./1024.));
    printf("Each kernel will be executed %d times.\n", NTIMES);
    printf(" The *best* time for each kernel (excluding the first iteration)\n"); 
    printf(" will be used to compute the reported bandwidth.\n");

#ifdef _OPENMP
    printf(HLINE);
#pragma omp parallel 
    {
#pragma omp master
	{
	    k = omp_get_num_threads();
	    printf ("Number of Threads requested = %i\n",k);
        }
    }
#endif

#ifdef _OPENMP
	k = 0;
#pragma omp parallel
#pragma omp atomic 
		k++;
    printf ("Number of Threads counted = %i\n",k);
#endif

    /* Get initial value for system clock. */
#pragma omp parallel for
    for (j=0; j<STREAM_ARRAY_SIZE; j++) {
	    a[j] = 1.0;
	    b[j] = 2.0;
	}

    printf(HLINE);

    if  ( (quantum = checktick()) >= 1) 
	printf("Your clock granularity/precision appears to be "
	    "%d microseconds.\n", quantum);
    else {
	printf("Your clock granularity appears to be "
	    "less than one microsecond.\n");
	quantum = 1;
    }

    t = mysecond();
#pragma omp parallel for
    for (j = 0; j < STREAM_ARRAY_SIZE; j++)
		a[j] = 2.0E0 * a[j];
    t = 1.0E6 * (mysecond() - t);

    printf("Each test below will take on the order"
	" of %d microseconds.\n", (int) t  );
    printf("   (= %d clock ticks)\n", (int) (t/quantum) );
    printf("Increase the size of the arrays if this shows that\n");
    printf("you are not getting at least 20 clock ticks per test.\n");

    printf(HLINE);

    printf("WARNING -- The above is only a rough guideline.\n");
    printf("For best results, please be sure you know the\n");
    printf("precision of your system timer.\n");
    printf(HLINE);
    
    /*	--- MAIN LOOP --- repeat test cases NTIMES times --- */
    mp1 = mprotect((void *)a, sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE, PROT_READ);    
    mp2 = mprotect((void *)b, sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE, PROT_READ);
    if(mp1 != 0 || mp2 != 0) {
        printf("mtier: error protecting memory (%d)\n", errno);
        return -1;
    }

    scalar = 3.0;

    clock_gettime(CLOCK_MONOTONIC, &time_start);
    intervals = 0;
    do {
        for (k=0; k<NTIMES; k++)
        {
            times[0][k] = mysecond();
            d = 0;
#ifdef TUNED
            tuned_STREAM_Copy();
#else
#pragma omp parallel for
            for (j=0; j<STREAM_ARRAY_SIZE; j++) {
                d += a[j];
                //c[j] += a[j];
            }
#endif
            times[0][k] = mysecond() - times[0][k];

            times[1][k] = mysecond();
#ifdef TUNED
            tuned_STREAM_Scale(scalar);
#else
            d = 0;
#pragma omp parallel for
            for (j=0; j<STREAM_ARRAY_SIZE; j++)
                d += scalar * b[j];
                //c[j] += scalar * b[j];
#endif
            times[1][k] = mysecond() - times[1][k];

            times[2][k] = mysecond();
#ifdef TUNED
            tuned_STREAM_Add();
#else
            d = 0;
#pragma omp parallel for
            for (j=0; j<STREAM_ARRAY_SIZE; j++)
                d += a[j]+b[j];
                //c[j] += a[j]+b[j];
#endif
            times[2][k] = mysecond() - times[2][k];

            times[3][k] = mysecond();
#ifdef TUNED
            tuned_STREAM_Triad(scalar);
#else
            d = 0;
#pragma omp parallel for
            for (j=0; j<STREAM_ARRAY_SIZE; j++)
                d += b[j]+scalar*a[j];
                //c[j] += b[j]+scalar*a[j];
#endif
            times[3][k] = mysecond() - times[3][k];
        }
        clock_gettime(CLOCK_MONOTONIC, &time_end);
        ++intervals;
    } while((time_end.tv_sec - time_start.tv_sec) < 100);

    us_time = (time_end.tv_sec - time_start.tv_sec);
    us_time = us_time * 1000000000;
    us_time = us_time + (time_end.tv_nsec - time_start.tv_nsec);
    //us_time = us_time - (time_start.tv_sec + time_start.tv_nsec);
    //us_time += ((time_end.tv_nsec - time_start.tv_nsec) * 1000);
    printf("stream: completed %d intervals in %lu microseconds\n", intervals,
            us_time);

    /*	--- SUMMARY --- */

    for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{
	for (j=0; j<4; j++)
	    {
	    avgtime[j] = avgtime[j] + times[j][k];
	    mintime[j] = MIN(mintime[j], times[j][k]);
	    maxtime[j] = MAX(maxtime[j], times[j][k]);
	    }
	}
    
    printf("Function    Best Rate MB/s  Avg time     Min time     Max time\n");
    for (j=0; j<4; j++) {
		avgtime[j] = avgtime[j]/(double)(NTIMES-1);

		printf("%s%12.1f  %11.6f  %11.6f  %11.6f\n", label[j],
	       1.0E-06 * bytes[j]/mintime[j],
	       avgtime[j],
	       mintime[j],
	       maxtime[j]);
    }
    printf(HLINE);

    /* --- Check Results --- */
    checkSTREAMresults();
    printf(HLINE);
    munmap(whole_arr, 2 * STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE));
    close(fd);

    return 0;
}

# define	M	20

int
checktick()
    {
    int		i, minDelta, Delta;
    double	t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */

    for (i = 0; i < M; i++) {
	t1 = mysecond();
	while( ((t2=mysecond()) - t1) < 1.0E-6 )
	    ;
	timesfound[i] = t1 = t2;
	}

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

    minDelta = 1000000;
    for (i = 1; i < M; i++) {
	Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
	minDelta = MIN(minDelta, MAX(Delta,0));
	}

   return(minDelta);
    }



/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

#ifndef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#endif
void checkSTREAMresults ()
{
	STREAM_TYPE aj,bj,cj,scalar;
	STREAM_TYPE aSumErr,bSumErr,cSumErr;
	STREAM_TYPE aAvgErr,bAvgErr,cAvgErr;
	double epsilon;
	ssize_t	j;
	int	k,ierr,err;

    /* reproduce initialization */
	aj = 1.0;
	bj = 2.0;
	cj = 0.0;
    /* a[] is modified during timing check */
	aj = 2.0E0 * aj;
    /* now execute timing loop */
	scalar = 3.0;
	for (k=0; k<NTIMES; k++)
        {
            cj = aj;
            bj = scalar*cj;
            cj = aj+bj;
            aj = bj+scalar*cj;
        }

    /* accumulate deltas between observed and expected results */
	aSumErr = 0.0;
	bSumErr = 0.0;
	cSumErr = 0.0;
	for (j=0; j<STREAM_ARRAY_SIZE; j++) {
		aSumErr += abs(a[j] - aj);
		bSumErr += abs(b[j] - bj);
		cSumErr += abs(c[j] - cj);
		// if (j == 417) printf("Index 417: c[j]: %f, cj: %f\n",c[j],cj);	// MCCALPIN
	}
	aAvgErr = aSumErr / (STREAM_TYPE) STREAM_ARRAY_SIZE;
	bAvgErr = bSumErr / (STREAM_TYPE) STREAM_ARRAY_SIZE;
	cAvgErr = cSumErr / (STREAM_TYPE) STREAM_ARRAY_SIZE;

	if (sizeof(STREAM_TYPE) == 4) {
		epsilon = 1.e-6;
	}
	else if (sizeof(STREAM_TYPE) == 8) {
		epsilon = 1.e-13;
	}
	else {
		printf("WEIRD: sizeof(STREAM_TYPE) = %lu\n",sizeof(STREAM_TYPE));
		epsilon = 1.e-6;
	}

	err = 0;
	if (abs(aAvgErr/aj) > epsilon) {
		err++;
		printf ("Failed Validation on array a[], AvgRelAbsErr > epsilon (%e)\n",epsilon);
		printf ("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n",aj,aAvgErr,abs(aAvgErr)/aj);
		ierr = 0;
		for (j=0; j<STREAM_ARRAY_SIZE; j++) {
			if (abs(a[j]/aj-1.0) > epsilon) {
				ierr++;
#ifdef VERBOSE
				if (ierr < 10) {
					printf("         array a: index: %ld, expected: %e, observed: %e, relative error: %e\n",
						j,aj,a[j],abs((aj-a[j])/aAvgErr));
				}
#endif
			}
		}
		printf("     For array a[], %d errors were found.\n",ierr);
	}
	if (abs(bAvgErr/bj) > epsilon) {
		err++;
		printf ("Failed Validation on array b[], AvgRelAbsErr > epsilon (%e)\n",epsilon);
		printf ("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n",bj,bAvgErr,abs(bAvgErr)/bj);
		printf ("     AvgRelAbsErr > Epsilon (%e)\n",epsilon);
		ierr = 0;
		for (j=0; j<STREAM_ARRAY_SIZE; j++) {
			if (abs(b[j]/bj-1.0) > epsilon) {
				ierr++;
#ifdef VERBOSE
				if (ierr < 10) {
					printf("         array b: index: %ld, expected: %e, observed: %e, relative error: %e\n",
						j,bj,b[j],abs((bj-b[j])/bAvgErr));
				}
#endif
			}
		}
		printf("     For array b[], %d errors were found.\n",ierr);
	}
	if (abs(cAvgErr/cj) > epsilon) {
		err++;
		printf ("Failed Validation on array c[], AvgRelAbsErr > epsilon (%e)\n",epsilon);
		printf ("     Expected Value: %e, AvgAbsErr: %e, AvgRelAbsErr: %e\n",cj,cAvgErr,abs(cAvgErr)/cj);
		printf ("     AvgRelAbsErr > Epsilon (%e)\n",epsilon);
		ierr = 0;
		for (j=0; j<STREAM_ARRAY_SIZE; j++) {
			if (abs(c[j]/cj-1.0) > epsilon) {
				ierr++;
#ifdef VERBOSE
				if (ierr < 10) {
					printf("         array c: index: %ld, expected: %e, observed: %e, relative error: %e\n",
						j,cj,c[j],abs((cj-c[j])/cAvgErr));
				}
#endif
			}
		}
		printf("     For array c[], %d errors were found.\n",ierr);
	}
	if (err == 0) {
		printf ("Solution Validates: avg error less than %e on all three arrays\n",epsilon);
	}
#ifdef VERBOSE
	printf ("Results Validation Verbose Results: \n");
	printf ("    Expected a(1), b(1), c(1): %f %f %f \n",aj,bj,cj);
	printf ("    Observed a(1), b(1), c(1): %f %f %f \n",a[1],b[1],c[1]);
	printf ("    Rel Errors on a, b, c:     %e %e %e \n",abs(aAvgErr/aj),abs(bAvgErr/bj),abs(cAvgErr/cj));
#endif

    /* Stop the pthread */
    pthread_mutex_lock(&stop_mutex);
    thread_should_stop = 1;
    pthread_mutex_unlock(&stop_mutex);
    while(!thread_has_stopped)  ;
    printf("Thread stopped! STREAM is exiting.\n");
}

#ifdef TUNED
/* stubs for "tuned" versions of the kernels */
void tuned_STREAM_Copy()
{
	ssize_t j;
    double d = 0;
    printf("tuned stream copy\n");
#pragma omp parallel for
        for (j=0; j<STREAM_ARRAY_SIZE; j++)
            d += a[j];
}

void tuned_STREAM_Scale(STREAM_TYPE scalar)
{
	ssize_t j;
    double d = 0;
#pragma omp parallel for
	for (j=0; j<STREAM_ARRAY_SIZE; j++)
	    d += scalar*c[j];
}

void tuned_STREAM_Add()
{
	ssize_t j;
    double d = 0;
#pragma omp parallel for
	for (j=0; j<STREAM_ARRAY_SIZE; j++)
	    d += a[j]+b[j];
}

void tuned_STREAM_Triad(STREAM_TYPE scalar)
{
	ssize_t j;
    double d = 0;
#pragma omp parallel for
	for (j=0; j<STREAM_ARRAY_SIZE; j++)
	    a[j] = b[j]+scalar*c[j];
}
/* end of stubs for the "tuned" versions of the kernels */
#endif
