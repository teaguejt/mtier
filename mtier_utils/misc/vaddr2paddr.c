#define __STDC_FORMAT_MACROS
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
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
