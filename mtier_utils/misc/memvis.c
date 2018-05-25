#define __STDC_FORMAT_MACROS
#define _LARGEFILE64_SOURCE
#include "mtier.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>

#define ARRAY_SIZE 32
#define ALLOC_SIZE 1024
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ADDRSIZE sizeof(uint64_t)
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_ADDR(x) (x>>PAGE_SHIFT)
#define PAGEMAP_ERROR -1
#define PAGE_INVALID 0xf005ba11

union pfn_t {
  uint64_t raw;
  struct {
    uint64_t pfn      : 55;
    uint32_t pshift   : 6;
    uint32_t res      : 1;
    uint32_t swapped  : 1;
    uint32_t present  : 1;
  } obj;
};

uint64_t gen_mask(uint32_t msb, uint32_t lsb)
{
  uint64_t mask = 0;
  if (msb == 63) {
    mask = (uint64_t)(-1);
  } else {
    mask = ((uint64_t)(1) << (msb+1)) - 1;
  }
  mask = mask >> lsb;
  return mask;
}

uint64_t extract_bits(uint64_t field, uint32_t msb, uint32_t lsb)
{
  uint64_t match = 0;
  match = field >> lsb;
  match = match & gen_mask(msb, lsb);
  return match;
}

uint32_t get_pfn_map(uint32_t mapfd, uint64_t vaddr, union pfn_t *pfnmap)
{
  off64_t lseek_ret;
  char databuf[ADDRSIZE];

  if ((lseek_ret = lseek64(mapfd, (vaddr/PAGE_SIZE)*ADDRSIZE, SEEK_SET)) == (off64_t) -1) {
    close(mapfd);
    printf("pagemap seek error\n");
    exit(1);
  }

  if (read(mapfd, databuf, ADDRSIZE) != ADDRSIZE) {
    return PAGEMAP_ERROR;
  }

  memcpy(&(pfnmap->raw), databuf, ADDRSIZE);

  /* Don't mess with swapped pages */
  if (pfnmap->obj.swapped || !(pfnmap->obj.present)) {
    return PAGEMAP_ERROR;
  }
  return 0;
}

uint64_t vaddr_to_paddr(uint64_t *vaddr, union pfn_t *pfnmap)
{
  uint64_t paddr;
  uint64_t offset;

  paddr = pfnmap->obj.pfn;
  paddr &=  0x7FFFFFFFFFFFFF;   /* Clear the dirty bit */
  paddr <<= PAGE_SHIFT;         /* Shift to convert pfn to address */
  offset = (uint64_t)vaddr & (PAGE_SIZE - 1);
  paddr |= offset;

  return paddr;

  /*printf("pfn information:\n");
  printf("pfn:     0x%016"PRIx64"\n", (pfnmap->obj.pfn & 0x7FFFFFFFFFFFFF) << 12);
  printf("pshift:  0x%016"PRIx32"\n", pfnmap->obj.pshift);
  printf("res:     0x%016"PRIx32"\n", pfnmap->obj.res);
  printf("swapped: 0x%016"PRIx32"\n", pfnmap->obj.swapped);
  printf("present: 0x%016"PRIx32"\n", pfnmap->obj.present);
  return ((pfnmap->obj.pfn << pfnmap->obj.pshift)
      | extract_bits((uint64_t) vaddr, (pfnmap->obj.pshift-1), 0));*/
}

uint64_t get_phys_addr(uint64_t addr)
{
  int pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
  if (pagemap_fd < 0) {
    fprintf(stderr, "error opening pagemap\n");
    exit(1);
  }

  uint64_t phys_addr  = 0x0;
  uint64_t phys_page = PAGE_INVALID;
  uint64_t virt_page = PAGE_ADDR(addr);
  union pfn_t pfnmap;
  if (get_pfn_map(pagemap_fd, addr, &pfnmap) != 0) {
    return 0x0;
  }
  phys_addr = vaddr_to_paddr((uint64_t*)addr, &pfnmap);
  return phys_addr;
}

/*
int main () {
  void *vaddr;
  uint64_t paddr;

  vaddr = malloc(1<<20);
  memset(vaddr, 0, 1<<20);
  paddr = get_phys_addr((uint64_t)vaddr);
  printf("vaddr: 0x%016" PRIx64 " paddr: 0x%016" PRIx64 "\n", vaddr, paddr);
}
*/

int main( int argc, char **argv ) {
    int count = 0;
    int alloc = ALLOC_SIZE;
    int array = ARRAY_SIZE;
    int i, j;
    int time;
    void *arr[ARRAY_SIZE] = {0};
    void *rm = 0, *rmo = 0;
    void *arrp, *rmp;   /* For aligned pointers */
    char opt;
    uint64_t paddr;

    printf( "Check proc for information on memory consumption. My pid is "
            "%ld\n", (long)getpid() );
    printf( "usage:\n\ta: allocate space for %d chars\n"
            "\tf: fill the array with %d allocations of %d chars\n"
            "\tr: remove (free) a single %d-char allocation\n"
            "\te: empty the array (free all %d-char allocations)\n"
            "\tm: mmap a huge chunk\n"
            "\tp: protect some memory (only do this after calling 'f' and 'm')\n"
            "\td: display the contents of the array and mmap'ed area\n"
            "\tq: quit\n",
            ALLOC_SIZE, ARRAY_SIZE, ALLOC_SIZE, ALLOC_SIZE, ALLOC_SIZE );
    do {
        opt = getc( stdin );

        switch( opt ) {
            case 'a':
            case 'A':
                if( count == ARRAY_SIZE ) {
                    printf( "Array full.\n" );
                }
                else {
                    printf( "Allocating some memory... " );
                    arr[count] = malloc( sizeof( char ) * ALLOC_SIZE );
                    if( !arr[count] ) {
                        printf( "FAILED! *sad trombone*\n" );
                    }
                    else {
                        printf( "success. Addr: 0x%p\n", arr[count] );
                        ++count;
                    }
                }
                break;

            case 'f':
            case 'F':
                printf( "Filling array...  " );
                for( i = count; i < ARRAY_SIZE; i++ ) {
                    arr[i] = malloc( sizeof( char ) * ALLOC_SIZE );
                    if( !arr[i] ) {
                        printf( "allocation of element %d failed.\n", i );
                    }
                    else {
                        printf( "success. arr[%d] = 0x%p\n", i, arr[count] );
                    }
                }
                count = ARRAY_SIZE;
                break;

            case 'r':
            case 'R':
                --count;
                printf( "Removing element %d from array... ", count );
                free( arr[count] );
                arr[count] = 0;
                printf( "done.\n" );
                break;

            case 'e':
            case 'E':
                printf( "Empying array... " );
                for( i = 0; i < count; i++ ) {
                    free( arr[i] );
                    arr[i] = 0;
                }
                printf( "done.\n" );
                count = 0;

                break;

            case 'd':
            case 'D':
                printf( "Array contents:\n" );
                for( i = 0; i < ARRAY_SIZE; i++ ) {
                   printf( "%4d: 0x%p\n", i, arr[i] );
                }
                printf( "\n\n" ); 
                break;

            case 'm':
            case 'M':
              printf( "Yuuuuge mmap... " );
              for( i = 0; i < 1 << 12; i++ ) {
                rm = malloc( 1 << 20 );
                if( !rm ) {
                  printf( "oops!\n" );
                  break;
                }
                for( j = 0; j < (1 << 20); j++ ) {
                  ((char *)rm)[j] = 1;
                  if( j > 0 )
                    ((char *)rm)[j] += ((char *)rm)[j - 1];
                }
              }
              //rm = rmo;
              printf( "done: addr is 0x%lx\n", (unsigned long *)rm );
              printf( "aligned pointer is: 0x%lx\n",
                PTR_TO_PAGE( rm ) );
              break;

            case 'p':
            case 'P':
              paddr = get_phys_addr((uint64_t)rm);
              printf("vaddr: 0x%016"PRIx64"\tpaddr: 0x%016"PRIx64"\n", (uint64_t)rm, paddr);
              arrp = (void *)PTR_TO_PAGE( arr[0] );
              rmp  = (void *)PTR_TO_PAGE( rm );
              printf( "arr  = 0x%lx,  rm = 0x%lx\n", (unsigned long *)arr,
                (unsigned long *)rm );
              printf( "arrp = 0x%lx, rmp = 0x%lx\n", (unsigned long *)arrp,
                (unsigned long *)rmp );
              printf( "Protecting memory... " );
              if( mprotect( arrp, 0x2000, PROT_READ ) != 0 )
                printf( "mprotect on array failed with error %d\n", errno );
              if( mprotect( rmp, 0x80000, PROT_READ ) != 0 )
                printf( "mprotect on mmap failed with error %d\n", errno );
              printf( "done. Check your maps\n" );
              break;

            case 'u':
            case 'U':
              paddr = get_phys_addr((uint64_t)rm);
              printf("vaddr: 0x%016"PRIx64"\tpaddr: 0x%016"PRIx64"\n", (uint64_t)rm, paddr);
        }
    } while( opt != 'q' && opt != 'Q' );

    return 0;
}
