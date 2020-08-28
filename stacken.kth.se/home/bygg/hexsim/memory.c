/*
 * This module implements the memory and paging systems.
 */

#include "hexsim.h"
#include <sys/mman.h>

#define MEMSIZE (16 * 1024 * 1024)

static uint8_t * memory;	/* Mapped in. */
static size_t memsize;		/* Size we got. */

static int mcflag = 0;		/* Changed memory? */
static hexaword mcfirst;
static hexaword mclast;

#ifndef MAP_ANON
#  define MAP_ANON MAP_ANONYMOUS
#endif

void mem_init(void)
{
  void* ptr;

  ptr = mmap(NULL, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);

  if (ptr == MAP_FAILED) {
    perror("mmap");
    return;
  }

  memory = ptr;
  memsize = MEMSIZE;
}

/*
 *  Routines to read and write memory.
 */

int mem_examine(int asp, hexaword addr)
{
  /*
   *  Should check for ASP_EXEC and ASP_USER here, when we do paging.
   */

  if (memory == NULL || addr >= memsize)
    return -1;
  return memory[addr];
}

int mem_deposit(int asp, hexaword addr, uint8_t data)
{
  if (memory == NULL || addr >= memsize)
    return -1;
  memory[addr] = data;

  wc_memory();

  return 0;
}

void memc_clear(void)
{
  mcflag = 0;
}

int memc_get(hexaword addr)
{
  if (mcflag) {
    if ((addr >= mcfirst) && (addr <= mclast))
      return 1;
  }

  return 0;
}

int mem_read(int size, int asp, hexaword addr, hexaword * res)
{
  switch (size) {
  case 1:
  case 2:
  case 4:
  case 8:
    break;
  default:
    return MST_NOMEM;
  }

  if (memory == NULL || addr >= memsize)
    return MST_NOMEM;

  if (addr + size > memsize)
    return MST_NOMEM;

  *res = 0;

  while (size-- > 0) {
    *res <<= 8;
    *res += memory[addr++];
  }

  return MST_OK;
}

int mem_write(int size, int asp, hexaword addr, hexaword val)
{
  if (memory == NULL || addr >= memsize)
    return MST_NOMEM;

  if (addr + size > memsize)
    return MST_NOMEM;

  if (!(mcflag && addr == mclast + 1)) {
    mcflag = 1;
    mcfirst = addr;
  }
  addr += size - 1;
  mclast = addr;
  
  while (size-- > 0) {
    memory[addr--] = val & 0xff;
    val >>= 8;
  }

  wc_memory();

  return MST_OK;
}

/*
 *  Load a file into memory.
 */

void mem_load_file(char* filename)
{
  FILE* f;
  int ret;

  if ((f = fopen(filename, "rb")) == NULL) {
    perror("fopen");
    return;
  }
  ret = fread(memory, 1, memsize, f);
  printf("read %d bytes\n", ret);
  fclose(f);

  wc_memory();
}
