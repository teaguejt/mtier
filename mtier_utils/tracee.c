#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include "mtier.h"

#define ALLOC_MEMBERS (1 << 20)
#define ALLOC_SIZE    (1024)

#define SIZE(num, type) \
    ((size_t)(num * sizeof(type)))

static int *arr[ALLOC_MEMBERS];

int main(int argc, char **argv) {
    int i, j;

    printf("allocating... ");
    for(i = 0; i < ALLOC_MEMBERS; i++) {
        arr[i] = malloc(sizeof(int) * ALLOC_SIZE);
        if(!arr[i]) {
            fprintf(stderr, "Allocation error. Exiting.\n");
            return -1;
        }

        for(j = 0; j < ALLOC_SIZE; j++) {
            arr[i][j] = 0;
        }

        if(mprotect((int *)PTR_TO_PAGE(arr[i]), SIZE(1024, int), PROT_READ) != 0) {
            fprintf(stderr, "Protection error. Exiting.\n");
            return -1;
        }
    }

    printf("done! Run tests now...\n");

    while(1) {}

    return 0;
}
