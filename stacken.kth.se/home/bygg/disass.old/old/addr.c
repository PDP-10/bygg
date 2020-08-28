/*
** This module implements all routines that handle addresses.  No other
** module should have to know what the inside of an address looks like.
*/

#include "disass.h"

/*
** a_copy() makes (allocates) a copy of an address.
*/

address* a_copy(address* src, address* dst)
{
  if (src == nil) {
    a_free(dst);
    return(nil);
  }
  if (dst == nil) {
    dst = a_new();
  }
  if (dst != nil) {
    dst->foobar = src->foobar;
    dst->count = src->count;
    dst->flags = src->flags;
    dst->depth = src->depth;
    dst->next = a_copy(src->next, dst->next);
  }
  return(dst);
}

/*
** a_new() allocates a new address, and inits it to zero.
*/

address* a_new(void)
{
  address* a;

  a = (address*) malloc(sizeof(address));
  a->next = nil;
  a->foobar = 0L;
  a->count = 0;
  a->flags = 0;
  a->depth = 0;
  return(a);
}

/*
** a_free() deallocates an address.
*/

void a_free(address* a)
{
  address* next;

  while (a != nil) {
    next = a->next;
    free(a);
    a = next;
  }
}

/*
** a_clear() makes an address point to address 0.
*/

void a_clear(address* a)
{
  a_free(a->next);
  a->next = nil;
  a->foobar = 0L;
  a->count = 0;
  a->flags = 0;
  a->depth = 0;
}

/*
** a_inc() increments an address with a specified amount.
*/

void a_inc(address* a, int i)
{
  a->foobar += i;
}

/*
** a_offset() returns an address with the specified offset.
*/

address* a_offset(address* a, long offset)
{
  static address* work = nil;

  work = a_copy(a, work);
  work->foobar += offset;
  return(work);
}

/*
** a_fip() returns an address which is the first address of the page
** the argument points to.  In scalar terms, it returns its argument
** with the lower eight bits set to zero.
*/

address* a_fip(address* addr)
{
  static address* work = nil;

  work = a_copy(addr, work);
  work->foobar &= ~0xff;	/* Wipe the lower eigth bits. */
  return(work);
}

/*
** a_a2l() converts an address (if possible) to a (32 bit) longword.
*/

longword a_a2l(address* a)
{
  return(a->foobar);
}

/*
** a_a2w() converts an address (if possible) to a (16 bit) word.
*/

word a_a2w(address* a)
{
  return(a->foobar & 0xffff);
}

/*
** a_l2a() converts a longword to an address.
*/

address* a_l2a(longword l)
{
  static address* a = nil;
  
  a = a_copy(a_zero(), a);
  a_inc(a, l);
  return(a);
}

/*
** a_mod() does a modulo operation on an address, with argument 2^n.
*/

int a_mod(address* a, int n)
{
  return (a->foobar & ((2<<n) - 1));
}

/*
** a_diff() returns the difference between two addresses
*/

int a_diff(address* a, address* b)
{
  return((a->foobar) - (b->foobar));
}

/*
** a_compare() compares two addresses, and returns:
**     -1 if (a < b),
**      0 if (a == b),
**     +1 if (a > b).
*/

int a_compare(address* a, address* b)
{
  if ((a->foobar) < (b->foobar)) {
    return(-1);
  }
  if ((a->foobar) > (b->foobar)) {
    return(1);
  }
  return(0);
}

/*
** a_eq() returns true if a == b.
*/

bool a_eq(address* a, address* b)
{
  return(a_compare(a, b) == 0);
}

/*
** a_ne() returns true if a != b.
*/

bool a_ne(address* a, address* b)
{
  return(a_compare(a, b) != 0);
}

/*
** a_gt() returns true if a > b.
*/

bool a_gt(address* a, address* b)
{
  return(a_compare(a, b) > 0);
}

/*
** a_ge() returns true if a >= b.
*/

bool a_ge(address* a, address* b)
{
  return(a_compare(a, b) >= 0);
}

/*
** a_lt() returns true if a < b.
*/

bool a_lt(address* a, address* b)
{
  return(a_compare(a, b) < 0);
}

/*
** a_le() returns true if a <= b.
*/

bool a_le(address* a, address* b)
{
  return(a_compare(a, b) <= 0);
}

/*
** a_adjacent() returns true if "a + 1" == "b".
*/

bool a_adjacent(address* a, address* b)
{
  if ((a->foobar + 1) == b->foobar) {
    return(true);
  } else {
    return(false);
  }
}

/*
** a_zero() returns a pointer to a zero address.
*/

address* a_zero(void)
{
  static address* work = nil;

  if (work == nil) {
    work = a_new();
  }
  a_clear(work);
  return(work);
}

/*
** a_next() returns the next address, without modifying its argument.
*/

address* a_next(address* a)
{
  static address* work = nil;

  work = a_copy(a, work);
  a_inc(work, 1);
  return(work);
}

/*
** a_step() ...
*/

void a_step(address* a, int i)
{
  a_inc(a, i);
}

bool a_more(address* a)
{
  UNUSED(a);

  return(false);
}

bool a_ismulti(address* a)
{
  UNUSED(a);

  return(false);
}

bool a_iscount(address* a)
{
  UNUSED(a);

  return(false);
}

bool a_isrange(address* a)
{
  UNUSED(a);

  return(false);
}

void a_test(void)
{
  /* nothing to test right now. */
}
