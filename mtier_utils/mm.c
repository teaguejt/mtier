#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include "mtier.h"

#define P2A(dim, i, j) \
  i * dim + j

#define SIZE(dim, type) \
  ((size_t)(dim * dim * sizeof(type)))

int main(int argc, char **argv) {
  int dim, iters;
  int i, j, k, run;
  int protect = 1;            /* Should we protect the RO matrices? */
  int seed = 0;
  double *m1, *m2, *r, tmp;
  struct timeval begin, end;
  double run_time;
  double total_time = 0.0;
  
  if(argc != 5) {
    printf("usage: %s size iters protect seed\n", argv[0]);
    return 0;
  }

  dim     = atoi(argv[1]);
  iters   = atoi(argv[2]);
  protect = atoi(argv[3]);
  seed    = atoi(argv[4]);
  printf("multiplying two %dx%d matrices (%d iterations)\n",
      dim, dim, iters);

  for(run = 0; run < iters; run++) {
    srand(seed);
    /* Allocate room for the matrices, make sure they're filled*/
    m1 = malloc(sizeof(double) * dim * dim);
    m2 = malloc(sizeof(double) * dim * dim);
    r  = malloc(sizeof(double) * dim * dim);

    if(!m1 || !m2 || !r) {
      fprintf(stderr, "one or more matrix could not be allocated. Aborting\n");
      return -1;
    }

    for(i = 0; i < dim; i++) {
      for(j = 0; j < dim; j++) {
        m1[P2A(dim, i, j)] = (double)rand() / (double)RAND_MAX;
        m2[P2A(dim, i, j)] = (double)rand() / (double)RAND_MAX;
        r[P2A(dim, i, j)]  = 0;
      }
    }

    if(protect) {
      if(mprotect((int *)PTR_TO_PAGE(m1), SIZE(dim, double), PROT_READ) != 0) {
        fprintf(stderr, "failed to mprotect m1: %d\n", errno);
        return -2;
      }

      if(mprotect((int *)PTR_TO_PAGE(m2), SIZE(dim, double), PROT_READ) != 0) {
        fprintf(stderr, "failed to mprotect m2: $d\n", errno);
        return -2;
      }
      printf("m1: 0x%lx - 0x%lx\n", (unsigned long)m1,
              (unsigned long)&(m1[dim * dim - 1]));
      printf("m2: 0x%lx - 0x%lx\n", (unsigned long)m2,
              (unsigned long)&(m2[dim * dim - 1]));
    }

    /* Print small matrices */
    if(dim <= 8) {
      printf("m1:\n");
      for(i = 0; i < dim; i++) {
        for(j = 0; j < dim; j++) {
          printf("%1.4f ", m1[P2A(dim, i, j)]);
        }
        printf("\n");
      }

      printf("\nm2:\n");
      for(i = 0; i < dim; i++) {
        for(j = 0; j < dim; j++) {
          printf("%1.4f ", m2[P2A(dim, i, j)]);
        }
        printf("\n");
      }
      printf("\n\n");
    }

    gettimeofday(&begin, NULL);
    /* Perform the mulriplication */
    for(i = 0; i < dim; i++) {
      for(j = 0; j < dim; j++) {
        for(k = 0; k < dim; k++) {
          //printf("%d %d = %d\n", i, j, r[P2A(dim, i, j)]);
          r[P2A(dim, i, j)] += m1[P2A(dim, i, k)] * m2[P2A(dim, k, j)];
        }
      }
    }
    gettimeofday(&end, NULL);
    run_time = (double)(end.tv_usec - begin.tv_usec) / 1000000 +
                (double)(end.tv_sec - begin.tv_sec);

    printf("Iter %d: finished in %.5f seconds\n", run, run_time);
    total_time += run_time;

    if(dim <= 8) {
      printf("r:\n");
      for(i = 0; i < dim; i++) {
        for(j = 0; j < dim; j++) {
          printf("%1.4f ", r[P2A(dim, i, j)]);
        }
        printf("\n");
      }
    }

    if(protect) {
      if(mprotect((int *)PTR_TO_PAGE(m1), SIZE(dim, double), PROT_WRITE) != 0) {
        fprintf(stderr, "Failed to restore writability to m1: %d\n", errno);
        return -3;
      }

      if(mprotect((int *)PTR_TO_PAGE(m2), SIZE(dim, double), PROT_WRITE) != 0) {
        fprintf(stderr, "Failed to restore writability to m2: %d\n", errno);
        return -3;
      }
    }
    //free(m1);
    //free(m2);
    //free(r);
  }

  printf("%d iterations in %.4f s (%.5f s per iter)\n", iters,
    total_time, total_time / iters);

  //while(1);

  return 0;
}
