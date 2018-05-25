#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv ) {
    int i;
    printf( "Entering matrix multiply\n" );
    printf( "Arg count is: %d\n", argc );
    for( i = 0; i < argc; i++ ) {
        printf( "\targ %d: %s\n", i, argv[i] );
    }
    return 0;
}
