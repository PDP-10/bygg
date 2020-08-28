/*
** This module implements all routines that handle addresses.  No other
** module should have to know what the inside of an address looks like.
*/

#include "disass.h"
#include "addrint.h"

/*
** our local variable:
*/

static int abc = 0;

/*
** copy1() is an internal routine to copy one address block, and ignore
** the next pointer if it is set.
*/

static address* copy1(address* src)
{
  address* dst;

  dst = a_new();
  if (dst != nil) {
    *dst = *src;		/* Copy the whole structure. */
    dst->next = nil;
  }
  return(dst);
}

/*
** a_copy() makes (allocates) a copy of an address.
*/

address* a_copy(address* src, address* dst)
{
  address* next;

  if (src == nil) {
    a_free(dst);
    return(nil);
  }
  if (dst == nil) {
    dst = a_new();
  }
  if (dst != nil) {
    next = dst->next;		/* Save next pointer. */
    *dst = *src;		/* Copy the whole structure. */
    dst->next = a_copy(src->next, next);
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
  if (a != nil) {
    abc += 1;
    a->next = nil;
    a->foobar = 0L;
    a->count = 0;
    a->type = aty_single;
    a->depth = 0;
  }
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
    abc -= 1;
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
  a->type = aty_single;
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
** a_inside() returns true if a is inside the given address range.
*/

bool a_inside(address* a, address* range)
{
  while (range != nil) {
    switch (range->type) {
    case aty_single:
      if (a_eq(a, range)) {
	return(true);
      }
      break;
    case aty_count:
      if (a_ge(a, range)) {
	if (a_le(a, a_offset(range, range->count))) {
	  return(true);
	}
      }
      break;
    case aty_range:
      if (a_ge(a, range)) {
	if (range->next != nil) {
	  if (a_le(a, range->next)) {
	    return(true);
	  }
	}
      }
      break;
    default:
      bug("a_inside", "bad address type");
    }
    range = a_cdr(range);
  }
  return(false);
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
** a_prev() returns the previous address, without modifying its argument.
*/

address* a_prev(address* a)
{
  static address* work = nil;

  work = a_copy(a, work);
  a_inc(work, -1);
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
** a_count() makes an address a counted range, like 1000:16.
** No new address blocks are allocated, the arguments are modified.
*/

void a_count(address* first, unsigned int count)
{
  if (first != nil) {
    first->count = count - 1;
    first->type = aty_count;
  }
}

/*
** a_set() sets the value of the first argument to the value of the second.
*/

void a_set(address* a, address* b)
{
  a->foobar = b->foobar;
  a->type = b->type;
  /* ... */
}

/*
** a_range() makes an address a range, like 1000-1fff.
** No new address blocks are allocated, the arguments are modified.
*/

void a_range(address* first, address* last)
{
  if ((first != nil) && (last != nil)) {
    last->next = first->next;
    first->next = last;
    first->type = aty_range;
  }
}

/*
** a_split() takes an address range, and splits it in two.  The argument
** address is reduced to end just before the split point, and the second
** range, starting at the split point, is returned.
*/

address* a_split(address* addr, address* split)
{
  static address* work = nil;

  work = a_copy(split, work);	/* Keep a copy before mods. */

  switch (addr->type) {
  case aty_single:		/* should not happen. */
    return(split);
  case aty_range:		/* range, addr-last. */
    a_range(split, a_copy(a_last(addr), nil));
    a_set(addr->next, a_prev(work));
    return(split);
  case aty_count:		/* count, addr:n */
    a_range(split, a_copy(a_last(addr), nil));
    a_range(addr, a_copy(a_prev(work), nil));
    return(split);
  }
  bug("a_split", "bad address type");
}

/*
** a_first() returns the first address in an address element.
*/

address* a_first(address* a)
{
  static address* work = nil;

  work = a_copy(a, work);
  if (work != nil) {
    a_free(work->next);
    work->next = nil;
    work->type = aty_single;
  }
  return(work);
}

/*
** a_last() returns the last address in an address element.
*/

address* a_last(address* a)
{
  if (a != nil) {
    switch (a->type) {
      case aty_single: return(a);
      case aty_count: return(a_offset(a, a->count));
      case aty_range: return(a->next);
      default: bug("a_last", "bad address type");
    }
  }
  return(nil);
}

/*
** a_cons() hangs the second address, which is possibly a list, onto
** the tail of the first argument.  If the first argument is nil,
** nothing happens.
*/

void a_cons(address* a, address* b)
{
  if (a != nil) {
    while (a->next != nil) {
      a = a->next;
    }
    a->next = b;
  }
}

/*
** a_car() returns a copy of the first address element in the given list.
*/

address* a_car(address* a)
{
  static address* work = nil;

  if (a == nil) {
    return(nil);
  }

  if (work != nil) {
    a_free(work);
  }

  work = copy1(a);		/* Copy first block. */

  if ((a->type == aty_range) && (work != nil)) {
    work->next = copy1(a->next);
  }

  return(work);
}

/*
** a_cdr() returns a pointer to the next address element in a list.
*/

address* a_cdr(address* a)
{
  if (a == nil) {
    return(nil);
  }

  if ((a->type == aty_range) && (a->next != nil)) {
    return(a->next->next);
  }

  return(a->next);
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
  if (a != nil) {
    switch (a->type) {
    case aty_single:
      return(false);
    case aty_count:
      return(true);
    case aty_range:
      return(true);
    default:
      bug("a_ismulti", "bad address type");
    }
  }
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

/*
** a_print() prints an address in textual format.
*/

void a_print(address* a, bool filio)
{
  while (a != nil) {
    if (filio) {
      wf_whex(a_a2l(a));
    } else {
      bufstring(a_a2str(a));
    }
    if ((a->type == aty_range) && (a->next != nil)) {
      a = a->next;
      if (filio) {
	wf_wchar('-');
	wf_whex(a_a2l(a));
      } else {
	bufchar('-');
	bufstring(a_a2str(a));
      }
    } else if (a->type == aty_count) {
      if (filio) {
	wf_wchar(':');
	wf_whex(a->count + 1);
      } else {
	bufchar(':');
	bufnumber(a->count + 1);
      }
    }
    if (a->next != nil) {
      if (filio) {
	wf_wchar(',');
      } else {
	bufchar(',');
      }
    }
    a = a->next;
  }
}

/*
** a_test() is an internal test routine.
*/

void a_test(void)
{
  /* nothing to test right now. */

  static address* a = nil;
  static address* b = nil;
  static address* c = nil;

  a = a_copy(a_l2a(0x1000), a);
  a_count(a, 0x1000);

  bufstring("before: a = ");
  bufaddress(a);
  bufnewline();

  b = a_copy(a_split(a, a_l2a(0x1400)), b);
  c = a_copy(a_split(b, a_l2a(0x1600)), c);

  bufstring("after:  a = ");
  bufaddress(a);
  bufstring(", b = ");
  bufaddress(b);
  bufstring(", c = ");
  bufaddress(c);
  bufnewline();

  bufstring("abc = ");
  bufnumber(abc);
  bufnewline();
}
