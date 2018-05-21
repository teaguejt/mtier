#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main( int argc, char **argv ) {
    void *v;
    int *arr;
    size_t p, c;

    /* Allocate a page */
    arr = malloc( sizeof( char ) * 4096 );
    printf( "arr addr = 0x%p\n", arr );
    
    v = (char *)arr - 16;
    printf( "v   addr = 0x%p\n", v );

    printf( "prev. size = %ld\n", (long)(*(size_t*)v ) );

    v = (char *)v + sizeof( size_t );
    printf( "v   addr = 0x%p\n", v );

    printf( "curr. size = %ld\n", (long)(*(size_t*)v ) );
    printf( "corrected  = %ld\n", (long)((*(size_t*)v) & 0xFFFFFFFFFFFFFFFC ) );
    return 0;
}
