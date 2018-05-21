#include <numa.h>
#include <stdio.h>
#include <stdlib.h>

void print_bitmask( struct bitmask *bmp ) {
  int longs, i; 
  
  longs = bmp->size / (sizeof( long ) * 8);
  printf( "number of longs = %d\n", longs );
  for( i = 0; i < longs; i++ )
    printf( "%lu ", bmp->maskp[i] );

  printf( "\n" );
}

int main( int argc, char **argv ) {
  int maxnode;
  unsigned long nodes1, nodes2;
  struct bitmask *nodes1m, *nodes2m;

  nodes1m = numa_allocate_nodemask();
  nodes2m = numa_allocate_nodemask();
  nodes1 = nodes2 = 1740534958;
  printf( "bitmask sizes: 1 = %u, 2 = %u\n", nodes1m->size, nodes2m->size );
  maxnode = numa_max_node() + 1;
  numa_bitmask_setbit( nodes1m, 0 );
  numa_bitmask_setbit( nodes2m, 1 );
  print_bitmask( nodes1m );
  print_bitmask( nodes2m );
  printf( "max node = %d\n", maxnode );



  return 0;
}
