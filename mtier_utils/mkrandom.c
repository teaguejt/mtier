#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define NUM_DOUBLES 48000000
#define TYPE double

int main(int argc, char **argv) {
    int i;
    unsigned char *c;
    FILE *f;
    TYPE *arr = malloc(sizeof(TYPE) * 2 * NUM_DOUBLES);
    c = (unsigned char *)arr;

    f = fopen("rand_dat", "wb");
    if(!f) {
        printf("Error opening file.\n");
        return -1;
    }

    srand(time(NULL));
    for(i = 0; i < NUM_DOUBLES * 2; i++) {
        arr[i] = (double)rand() / RAND_MAX * 2.0 - 1.0;
    }

    for(i = 0; i < 10 * sizeof(double); i++) {
        printf("%x ", c[i]);
    }

    fwrite(arr, 1, sizeof(TYPE) * 2 * NUM_DOUBLES, f);

    fclose(f);
    printf("\n");
    return 0;
}
