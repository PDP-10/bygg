/*
** This module implements the memory and status routines.
*/

#include "disass.h"
#include "addrint.h"

/*
** Local data types we need in this module:
*/

typedef enum {
  styp_com,			/* Comment. */
  styp_dsc,			/* Description. */
  styp_exp,			/* Expansion. */
  styp_lbl,			/* Label. */
  maxstring			/* must be last */
} strtype;

/*
** The infoblock holds information on things like labels, comments, etc.
** for an address.
*/

typedef struct info {
  struct info* next;		/* Next infoblock for this PDB */
  struct info* lblnext;		/* Next infoblock in this label chain. */
  struct pdb* pdb;		/* Which pdb we are linked from. */
  address*   addr;		/* *** to be removed *** */
  char*      str[maxstring];	/* Comment/Description/... */
  byte*      flags;		/* Flag vector. */
  pattern*   args;		/* Inline arguments for routine. */
  byte       explength;		/* Number of bytes expansion expands. */
  byte       pdboffset;		/* Address offset from pdb start. */
  byte       flagcount;		/* Number of flags. */
  byte       bits;		/* Various bits/flags.  See ib_xxx below. */
#   define ib_noreturn 0x01	/*   This routine does not return, "longjmp" */
#   define ib_lblref   0x40	/*   This label has been referenced. */
#   define ib_lbldef   0x80	/*   This label has been defined. */
} infoblock;

/*
** The pmp (Page Map Page) contains pointers to either the pdb or the next
** pmp for a given address.
*/

typedef struct {
  void* ptr[256];
} pmp;

/*
** The pdb (Page Data Block) contains pointers to the data buffer (if it
** exists) and status buffer (if it exists) for this page, along with a
** pointer to the first infoblock for addresses in this page.
*/

typedef struct pdb {
  address* addr;		/* Address of this page (first byte). */
  byte* data;			/* Pointer to data buffer, or nil. */
  byte* status;			/* Pointer to status buffer, or nil. */
  infoblock* info;		/* Pointer to info blocks for this page. */
} pdb;

/*
** The byte in the status buffer contains:
*/

#define stb_nxm   0x80		/* This location is unmapped. */
#define stb_ldly  0x40		/* This instruction is the last delay slot. */
#define stb_0x20  0x20		/* Unused bit. */
#define stb_0x10  0x10		/* Unused bit. */
#define stb_code  0x0f		/* Four bits of status code. */

/*
** The segment holds information on one segment of memory.  We keep a list
** of these to be able to tell what memory we have mapped in.  We don't use
** these blocks to do lookups, though.
*/

typedef struct segment {
  struct segment* next;
  address* first;
  address* last;
  counter length;
} segment;

/*
** Try implementing general objects instead of separate symbols, patterns, ...
*/

#define oty_note	1
#define oty_pattern	2
#define oty_register	3
#define oty_symbol	4
#define oty_highlight   5

/*
** The object block describes a general object.
*/

typedef struct object {
  struct object* next;		/* Next in chain. */
  struct object* prev;		/* Previous ditto. */
  objindex index;		/* Numeric index. */
  char* name;			/* Name, if any. */
  void* data;			/* Data pointer. */
} object;

/*
** The object header describes a list of objects.
*/

typedef struct {
  object* first;		/* First item in list. */
  object* last;			/* Last item in list. */
  object* cache;		/* Last object referenced. */
  longword count;		/* Number of objects in list. */
  longword nextindex;		/* Next index to be assigned. */
  int type;			/* Type of data in this object list.*/
} objheader;

#define nullheader(type) {nil, nil, nil, 0, 0, type }

/*
** The regsub block contains information about register sub-values.
*/

typedef struct regsub {
  struct regsub* next;
  struct regsub* prev;
  address* addr;
  value* val;
} regsub;

/*
** The regdb (register data block) contains information about registers.
** It is pointed to by an object block.
*/

typedef struct {
  int type;			/* Type of register, vty_xxx */
  value* mainval;		/* Main value, if any. */
  regsub* sublist;		/* List of special values. */
  regsub* subcache;		/* Last subval referenced. */
} regdb;

/*
** The hlpoint struct contains information about highlight points, like
** what address we are set at, and an index into common's database.
*/

typedef struct {
  int handle;			/* Handle used by common.c */
  address* addr;		/* Where we are set. */
} hlpoint;

/**********************************************************************/
/*
** Our local variables:
*/

static objheader hlheader  = nullheader(oty_highlight);
static objheader notheader = nullheader(oty_note);
static objheader patheader = nullheader(oty_pattern);
static objheader regheader = nullheader(oty_register);
static objheader symheader = nullheader(oty_symbol);

static segment* segmentlist = nil;
static counter segmentcount = 0;

static counter strcount[maxstring]; /* Zeroed out by m_init(). */

#define hashsize 1007

static infoblock* labelhash[hashsize]; /* Zeroed out by m_init(). */

static infoblock* infocache = nil;

static counter uniqlong = 0L;

static byte pageoffset;

static pmp page0;		/* Zeroed out by m_init(). */
static int pmpdepth = 1;

static pdb* nextpdb;
static byte nextoffset;

/*
** global variables we delcare:
*/

/* none for now. */

/**********************************************************************/

/*
** m_init() inits this module.
*/

void m_init(void)
{
  int i;

  for (i = 0; i < 256; i += 1) {
    page0.ptr[i] = nil;
  }
  pmpdepth = 1;

  for (i = 0; i < maxstring; i += 1) {
    strcount[i] = 0;
  }

  for (i = 0; i < hashsize; i += 1) {
    labelhash[i] = nil;
  }
}

/**********************************************************************/

/*
** newpmp() is a routine that allocates and inits a new Page Map Page.
*/

static pmp* newpmp(void)
{
  pmp* p;
  int i;

  p = malloc(sizeof(pmp));
  for (i = 0; i < 256; i += 1) {
    p->ptr[i] = nil;
  }
  return(p);
}

/*
** incpmpdepth() increments the pmp depth by one.
*/

static void incpmpdepth(void)
{
  pmp* p;
  int i;

  p = malloc(sizeof(pmp));
  for (i = 0; i < 256; i += 1) {
    p->ptr[i] = page0.ptr[i];
    page0.ptr[i] = nil;
  }
  page0.ptr[0] = p;
  pmpdepth += 1;
}

/*
** makestatus() makes sure that a given pdb has a status buffer.
*/

static void makestatus(pdb* p)
{
  if (p->status == nil) {
    p->status = malloc(256);
    memset(p->status, (stb_nxm | (stb_code & st_none)), 256);
  }
}

/*
** makedata() makes sure that a given pdb has data/status buffers.
*/

static void makedata(pdb* p)
{
  makestatus(p);		/* Data buffer, must have status. */
  if (p->data == nil) {
    p->data = malloc(256);
    bzero(p->data, 256);
  }
}

/*
** findpdb() finds the pdb for a specified address, possibly creating
** it and all the page maps needed.  As a side effect it sets up the
** global variable "pageoffset" to make retreiving data/status easier.
*/

static pdb* findpdb(address* addr, bool create)
{
  longword l;
  byte b[4];
  int depth;
  pmp* pmptr;
  pmp* pmnxt;
  pdb* pdptr;

/* TODO: implement a one-entry cache for pdb's */

  if (true) {			/* Since the addr is just a longword... */
    l = a_a2l(addr);
    pageoffset = l & 0xff;
    b[1] = (l >> 8)  & 0xff;
    b[2] = (l >> 16) & 0xff;
    b[3] = (l >> 24) & 0xff;
    depth = 1;
    if (b[2] != 0) depth = 2;
    if (b[3] != 0) depth = 3;
  }

  while (depth > pmpdepth) {
    if (!create) {
      return(nil);
    }
    incpmpdepth();
  }
  depth = pmpdepth;
  pmptr = &page0;
  while (depth > 1) {
    pmnxt = pmptr->ptr[b[depth]];
    /* pmnxt = pmptr->ptr[addr->addr[depth]]; */
    if (pmnxt == nil) {
      if (!create) {
	return(nil);
      }
      pmnxt = newpmp();
      if (pmnxt == nil) {
	return(nil);
      }
      pmptr->ptr[b[depth]] = pmnxt;
    }
    depth -= 1;
    pmptr = pmnxt;
  }
  pdptr = pmptr->ptr[b[1]];
  if ((pdptr == nil) && (create)) {
    pdptr = malloc(sizeof(pdb));
    pdptr->addr = a_copy(a_fip(addr), nil);
    pdptr->data = nil;
    pdptr->status = nil;
    pdptr->info = nil;
    pmptr->ptr[b[1]] = pdptr;
  }
  return(pdptr);
}

/*
** getstc() returns the status byte for a specified address.  If there
** is no status byte to get, we make one up.
*/

static byte getstc(address* addr)
{
  pdb* p;

  p = findpdb(addr, false);
  if ((p != nil) && (p->status != nil)) {
    return(p->status[pageoffset]);
  }		 
  return(stb_nxm | (stb_code & st_none));
}

/*
** hashstring() returns a hash code for the given string.
*/

static int hashstring(char* name)
{
  longword hash = 0x123456ff;
  char c;

  while ((c = *name++) != (char) 0) {
    hash = hash ^ c;
    hash = (hash << 5) ^ hash;
  }
  hash = hash & 0x7fffffff;
  return(hash % hashsize);
}

/*
** ibaddr() computes the address of an infoblock.
*/

static address* ibaddr(infoblock* i)
{
  return(a_offset(i->pdb->addr, i->pdboffset));
}

/*
** makeinfo() allocates and inits a new infoblock.
*/

static infoblock* makeinfo(address* addr, pdb* p)
{
  infoblock* i;
  strtype type;
  infoblock* prev;
  infoblock* next;

  i = malloc(sizeof(infoblock));

  i->addr = a_copy(addr, nil);

  i->lblnext = nil;
  for(type = 0; type < maxstring; type += 1) {
    i->str[type] = nil;
  }
  i->pdb = p;
  i->pdboffset = a_diff(addr, p->addr);
  i->flags = nil;
  i->args = nil;
  i->explength = 0;
  i->flagcount = 0;
  i->bits = 0;

  prev = nil;
  next = p->info;
  
  while ((next != nil) && (next->pdboffset < i->pdboffset)) {
    prev = next;
    next = next->next;
  }

  if (prev == nil) {
    p->info = i;
  } else {
    prev->next = i;
  }
  i->next = next;
  return(i);
}

/*
** findinfo() finds the infoblock for a given address, possibly
** creating it if there was none to begin with.
*/

static infoblock* findinfo(address* addr, bool create)
{
  infoblock* i;
  pdb* p;

  if (infocache != nil) {
    if (a_eq(addr, infocache->addr)) {
      return(infocache);
    }
  }

  p = findpdb(addr, create);

  if (p != nil) {
    i = p->info;

    while (i != nil) {
      if (i->pdboffset == pageoffset) {
	infocache = i;
	return(i);
      }
      i = i->next;
    }

    if (create) {
      i = makeinfo(addr, p);
      infocache = i;
      return(i);
    }
  }
  return(nil);
}

/*
** freeseg() deallocates a segment block.
*/

static void freeseg(segment* seg)
{
  free(seg->first);
  free(seg->last);
  free(seg);
  segmentcount -= 1;
}

/*
** joinseg() checks if a given segment runs into the next one, if this is
** the case the segments will be joined into one.  This process is repeated
** as long as needed.
*/

static void joinseg(segment* this)
{
  segment* next;

  next = this->next;
  while (next != nil) {
    if (a_compare(this->last, next->last) >= 0) {
      this->next = next->next;
      freeseg(next);
      next = this->next;
    } else if (a_compare(this->last, next->first) >= 0) {
      this->length += a_diff(next->last, this->last);
      this->last = a_copy(next->last, this->last);
      this->next = next->next;
      freeseg(next);
      next = this->next;
    } else if (a_adjacent(this->last, next->first)) {
      this->length += next->length;
      this->last = a_copy(next->last, this->last);
      this->next = next->next;
      freeseg(next);
      next = nil;
    } else {
      next = nil;
    }
  }
}

/*
** makeseg() creates a new segment block and links it in at the right
** place.  If the previous block ends just before us, we extend that
** block instead.  After doing this, we check for running into the
** next block.
*/

static void makeseg(address* addr, int size, segment* prev, segment* next)
{
  segment* seg;

  if ((prev != nil) && a_adjacent(prev->last, addr)) {
    a_inc(prev->last, size);
    prev->length += size;
    seg = prev;
  } else {
    seg = malloc(sizeof(segment));
    seg->first = a_copy(addr, nil);
    seg->last =  a_copy(addr, nil);
    a_inc(seg->last, size - 1);
    seg->length = size;
    if (prev == nil) {
      segmentlist = seg;
    } else {
      prev->next = seg;
    }
    seg->next = next;
    segmentcount += 1;
  }
  joinseg(seg);
}

/*
** meminclude() updates the segment list to include a given block of mem.
*/

static void meminclude(address* addr, int size)
{
  segment* prev;
  segment* this;
  address* newend;

  prev = nil;
  this = segmentlist;
  while (this != nil) {
    if (a_compare(this->last, addr) >= 0) {           /* before or inside? */
      if (a_compare(this->first, addr) <= 0) {        /* inside? */
	newend = a_offset(addr, size - 1);            /* Yes, compute end. */
	if (a_compare(newend, this->last) > 0) {      /* All inside? */
	  this->length += a_diff(newend, this->last); /* No, extend this. */
	  this->last = a_copy(newend, this->last);    /* Update last addr. */
	  joinseg(this);
	}
      } else {
	makeseg(addr, size, prev, this);
      }
      wc_segment();
      return;
    }
    prev = this;
    this = this->next;
  }
  makeseg(addr, size, prev, nil);
  wc_segment();			/* Tell the window handler. */
}

/*
** memstore() stores a buffer of data at address addr.  The address will
** be updated to point to the next address.
*/

static void memstore(address* addr, byte* buffer, int size)
{
  pdb* p;
  int count;

  meminclude(addr, size);	/* Update segment buffers. */

  while (size > 0) {
    p = findpdb(addr, true);	/* Find (possibly creating) pdb. */
    makedata(p);		/* Make sure we have data/status buffers. */
    count = 0;
    while (size-- > 0) {
      p->status[pageoffset] &= ~stb_nxm;
      p->data[pageoffset] = *buffer++;
      count += 1;
      if (pageoffset == 0xff) {
	break;
      }
      pageoffset += 1;
    }
    a_inc(addr, count);
  }
}

typedef void (pdbhandler)(pdb*);

static void scanlevel(pmp* page, int level, pdbhandler* handler)
{
  int i;

  for (i = 0; i < 256; i += 1) {
    if (page->ptr[i] != nil) {
      if (level > 1) {
	scanlevel(page->ptr[i], level - 1, handler);
      } else {
	(*handler)(page->ptr[i]);
      }
    }
  }
}

static void scanmemory(pdbhandler* handler)
{
  scanlevel(&page0, pmpdepth, handler);
}

/*
** copystring() allocates a new string, and copies its argument to it.
*/

char* copystring(char* src, char* dst)
{
  if (dst != nil) {
    free(dst);
    dst = nil;
  }
  if (src != nil) {
    dst = malloc(strlen(src) + 1);
    strcpy(dst, src);
  }
  return(dst);
}

/**********************************************************************/

static bool str_exist(address* addr, strtype type)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    if (i->str[type] != nil) {
      return(true);
    }
  }
  return(false);
}

static char* str_find(address* addr, strtype type)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    return(i->str[type]);
  }
  return(nil);
}

static infoblock* str_insert(address* addr, char* name, strtype type)
{
  infoblock* i;

  i = findinfo(addr, true);
  if (i != nil) {
    if (i->str[type] != nil) {
      free(i->str[type]);
      strcount[type] -= 1;
    }
    i->str[type] = copystring(name, nil);
    strcount[type] += 1;
    wc_local(addr);
  }
  return(i);
}

static void str_delete(address* addr, strtype type)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    if (i->str[type]) {
      free(i->str[type]);
      i->str[type] = nil;
      strcount[type] -= 1;
      wc_local(addr);
    }
  }
}

static void sclr_level(pmp* page, int level, strtype type)
{
  int index;
  pdb* p;
  infoblock* ib;
  
  for (index = 0; index < 256; index += 1) {
    if (page->ptr[index] != nil) {
      if (level > 1) {
	sclr_level(page->ptr[index], level - 1, type);
      } else {
	p = page->ptr[index];
	for (ib = p->info; ib != nil; ib = ib->next) {
	  if (ib->str[type] != nil) {
	    free(ib->str[type]);
	    ib->str[type] = nil;
	  }
	}
      }
    }
  }
}

static void str_clear(strtype type)
{
  sclr_level(&page0, pmpdepth, type); /* Call recursive hack. */
  strcount[type] = 0;
  wc_total();		/* Changes everywhere. */
}

/******************************************/

bool c_exist(address* addr)
{
  return(str_exist(addr, styp_com));
}

char* c_find(address* addr)
{
  return(str_find(addr, styp_com));
}

void c_insert(address* addr, char* name)
{
  (void) str_insert(addr, name, styp_com);
}

void c_delete(address* addr)
{
  str_delete(addr, styp_com);
}

void c_clear(void)
{
  str_clear(styp_com);
}

counter c_count(void)
{
  return(strcount[styp_com]);
}

/**********************************************************************/

bool d_exist(address* addr)
{
  return(str_exist(addr, styp_dsc));
}

char* d_find(address* addr)
{
  return(str_find(addr, styp_dsc));
}

void d_insert(address* addr, char* text)
{
  (void) str_insert(addr, text, styp_dsc);
}

void d_delete(address* addr)
{
  str_delete(addr, styp_dsc);
}

void d_clear(void)
{
  str_clear(styp_dsc);
}

counter d_count(void)
{
  return(strcount[styp_dsc]);
}

/**********************************************************************/

bool e_exist(address* addr)
{
  return(str_exist(addr, styp_exp));
}

char* e_find(address* addr)
{
  return(str_find(addr, styp_exp));
}

void e_insert(address* addr, char* text, int length)
{
  infoblock* i;

  i = str_insert(addr, text, styp_exp);
  if (i != nil) {
    i->explength = length;
  }
}

void e_delete(address* addr)
{
  str_delete(addr, styp_exp);
}

void e_clear(void)
{
  str_clear(styp_exp);
}

counter e_count(void)
{
  return(strcount[styp_exp]);
}

int e_length(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i == nil) {
    return(1);			/* Default lengt if we have no exp. */
  }
  return(i->explength);
}

/******************************************/

byte* f_read(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    return(i->flags);
  }
  return(nil);
}

void f_write(address* addr, byte* flags)
{
  infoblock* i;
  byte* newflags;
  int pos;

  i = findinfo(addr, true);
  if (i != nil) {
    if (i->flags != nil) {
      free(i->flags);
      i->flags = nil;
    }
    newflags = malloc(flags[0]);
    for (pos = 0; pos <= flags[0]; pos += 1) {
      newflags[pos] = flags[pos];
    }
    i->flags = newflags;
  }
  wc_local(addr);
}

void f_delete(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    if (i->flags != nil) {
      free(i->flags);
      i->flags = nil;
    }
  }
  wc_local(addr);
}

static void fclr_level(pmp* page, int level)
{
  int index;
  pdb* p;
  infoblock* ib;
  
  for (index = 0; index < 256; index += 1) {
    if (page->ptr[index] != nil) {
      if (level > 1) {
	fclr_level(page->ptr[index], level - 1);
      } else {
	p = page->ptr[index];
	for(ib = p->info; ib != nil; ib = ib->next) {
	  if (ib->flags != nil) {
	    free(ib->flags);
	    ib->flags = nil;
	  }
	}
      }
    }
  }
}

void f_clear(void)
{
  fclr_level(&page0, pmpdepth); /* Call recursive hack. */
  wc_total();		/* Changes everywhere. */
}

/**********************************************************************/

pattern* ia_read(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    return(i->args);
  }
  return(nil);
}

void ia_write(address* addr, pattern* pat)
{
  infoblock* i;

  i = findinfo(addr, true);
  if (i != nil) {
    i->args = p_copy(pat, i->args);
  }
  wc_total();
}

void ia_delete(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    p_free(i->args);
    i->args = nil;
  }
  wc_total();
}

static void iaclr_level(pmp* page, int level)
{
  int index;
  pdb* p;
  infoblock* ib;

  for (index = 0; index < 256; index += 1) {
    if (page->ptr[index] != nil) {
      if (level > 1) {
	iaclr_level(page->ptr[index], level - 1);
      } else {
	p = page->ptr[index];
	for (ib = p->info; ib != nil; ib = ib->next) {
	  if (ib->args != nil) {
	    p_free(ib->args);
	    ib->args = nil;
	  }
	}
      }
    }
  }
}

void ia_clear(void)
{
  iaclr_level(&page0, pmpdepth); /* Call recursive hack. */
  wc_total();
}

/**********************************************************************/

bool nrf_read(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    if (i->bits && ib_noreturn) {
      return(true);
    }
  }
  return(false);
}

void nrf_write(address* addr, bool flag)
{
  infoblock* i;

  i = findinfo(addr, true);
  if (i != nil) {
    if (flag) {
      i->bits |= ib_noreturn;
    } else {
      i->bits &= ~ib_noreturn;
    }
  }
}

static void nrfclr_level(pmp* page, int level)
{
  int index;
  pdb* p;
  infoblock* ib;

  for (index = 0; index < 256; index += 1) {
    if (page->ptr[index] != nil) {
      if (level > 1) {
	nrfclr_level(page->ptr[index], level - 1);
      } else {
	p = page->ptr[index];
	for (ib = p->info; ib != nil; ib = ib->next) {
	  ib->bits &= ~ib_noreturn;
	}
      }
    }
  }
}

void nrf_clear(void)
{
  nrfclr_level(&page0, pmpdepth);
}

/**********************************************************************/

bool l_exist(address* addr)
{
  return(str_exist(addr, styp_lbl));
}

char* l_find(address* addr)
{
  return(str_find(addr, styp_lbl));
}

void l_insert(address* addr, char* name)
{
  address* prev;
  infoblock* i;
  int bucket;

  prev = l_lookup(name);
  if (prev != nil) {
    l_delete(prev);
  }
  l_delete(addr);

  i = str_insert(addr, name, styp_lbl);

  if (i != nil) {
    bucket = hashstring(l_canonical(name));
    i->lblnext = labelhash[bucket];
    labelhash[bucket] = i;
  }
  wc_total();
}

void l_delete(address* addr)
{
  int bucket;
  char* name;
  infoblock* prev;
  infoblock* this;

  name = str_find(addr, styp_lbl);
  if (name != nil) {
    bucket = hashstring(l_canonical(name));
    prev = nil;
    this = labelhash[bucket];
    while (this != nil) {
      if (strcmp(name, this->str[styp_lbl]) == 0) {
	if (prev == nil) {	/* First entry in chain */
	  labelhash[bucket] = this->lblnext;
	} else {		/* Not first entry. */
	  prev->lblnext = this->lblnext;
	}
	this->lblnext = nil;	/* Wipe current ptr, we are clean. */
      }
      prev = this;
      this = this->lblnext;
    }
    wc_total();
  }
  str_delete(addr, styp_lbl);
}

void l_clear(void)
{
  int bucket;
  infoblock* info;
  infoblock* next;

  for (bucket = 0; bucket < hashsize; bucket += 1) {
    for (info = labelhash[bucket]; info != nil; info = next) {
      next = info->lblnext;
      info->lblnext = nil;
      if (info->str[styp_lbl] != nil) {
	free(info->str[styp_lbl]);
	info->str[styp_lbl] = nil;
      }
    }
    labelhash[bucket] = nil;
  }
  strcount[styp_lbl] = 0;
  wc_total();
}

void l_cflags(void)
{
  int bucket;
  infoblock* i;

  for (bucket = 0; bucket < hashsize; bucket += 1) {
    for (i = labelhash[bucket]; i != nil; i = i->lblnext) {
      i->bits &= ~(ib_lbldef+ib_lblref);
    }
  }
}

void l_def(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    i->bits |= ib_lbldef;
  }
}

void l_ref(address* addr)
{
  infoblock* i;

  i = findinfo(addr, false);
  if (i != nil) {
    i->bits |= ib_lblref;
  }
}

counter l_count(void)
{
  return(strcount[styp_lbl]);
}

address* l_lookup(char* name)
{
  static address* work = nil;
  int bucket;
  infoblock* i;
  
  name = copystring(l_canonical(name), nil);
  bucket = hashstring(name);

  for (i = labelhash[bucket]; i != nil; i = i->lblnext) {
    if (strcmp(name, l_canonical(i->str[styp_lbl])) == 0) {
      free(name);
      work = a_copy(i->addr, work);
      return(work);
    }
  }
  free(name);
  return(nil);
}

void l_rehash(void)
{
  int bucket;
  infoblock* this;
  infoblock* chain;

  chain = nil;

  for (bucket = 0; bucket < hashsize; bucket += 1) {
    while (labelhash[bucket] != nil) {
      this = labelhash[bucket];
      labelhash[bucket] = this->lblnext;
      this->lblnext = chain;
      chain = this;
    }
  }

  while (chain != nil) {
    this = chain;
    chain = chain->lblnext;
    /* check for empty string here? */
    bucket = hashstring(l_canonical(this->str[styp_lbl]));
    this->lblnext = labelhash[bucket];
    labelhash[bucket] = this;
  }
}

/**********************************************************************/
/*
** Local routines to handle objects and object lists.
*/
/**********************************************************************/

/*
** allocate and init a new register data block:
*/

static regdb* newregdb(void)
{
  regdb* r;

  r = malloc(sizeof(regdb));
  if (r != nil) {
    r->type = vty_long;
    r->mainval = nil;
    r->sublist = nil;
    r->subcache = nil;
  }
  return(r);
}

/*
** allocate and init a new highlight point block:
*/

static hlpoint* newhlpoint(void)
{
  hlpoint* hlp;

  hlp = malloc(sizeof(hlpoint));
  if (hlp != nil) {
    hlp->addr = nil;
    hlp->handle = 0;
  }
  return(hlp);
}

/*
** Return a pointer to a new data item for an object of specified type.
*/

static void* newobjdata(int type)
{
  switch (type) {
    case oty_register: return(newregdb());
    case oty_highlight: return(newhlpoint());
  }
  return(nil);
}

/*
** Create a new object in an object list.
*/

static object* makeobject(objheader* hdr, char* name)
{
  object* obj;

  obj = malloc(sizeof(object));

  obj->next = nil;
  obj->prev = hdr->last;
  if (hdr->last != nil) {
    hdr->last->next = obj;
    hdr->last = obj;
  } else {
    hdr->first = obj;
    hdr->last = obj;
  }

  hdr->count += 1;
  hdr->cache = obj;
  hdr->nextindex += 1;

  obj->index = hdr->nextindex;
  obj->name = copystring(name, nil);
  obj->data = newobjdata(hdr->type);

  return(obj);
}

/*
** zap (deallocate) a hlpoint block.
*/

static void zaphlp(hlpoint* hlp)
{
  a_free(hlp->addr);		/* Dealloc address block. */
  clearwatch(hlp->handle);	/* Tell common.c to forget this. */
  free(hlp);			/* Dealloc main structure. */
}

/*
** zap (deallocate) a regsub block.  Does not unlink it.
*/

static void zapregsub(regsub* rs)
{
  a_free(rs->addr);
  v_free(rs->val);
  free(rs);
}

/*
** zap (deallocate) a regdb block.
*/

static void zapregdb(regdb* r)
{
  regsub* rs;

  v_free(r->mainval);
  while (r->sublist != nil) {
    rs = r->sublist;
    r->sublist = r->sublist->next;
    zapregsub(rs);
  }
  free(r);			/* Dealloc regdb itself. */
}

/*
** zapobject() zaps an object and all data it points to.
*/

static void zapobject(objheader* hdr, object* this)
{
  switch(hdr->type) {
  case oty_note:		/* Note, data is a string. */
    free(this->data);
    break;
  case oty_pattern:		/* Pattern, data is a pattern chain. */
    p_free(this->data);
    break;
  case oty_register:		/* Register, data is regdb block. */
    zapregdb(this->data);
    break;
  case oty_symbol:		/* Symbol, data is a string. */
    free(this->data);
    break;
  case oty_highlight:		/* Highlight point, data is highlight block */
    zaphlp(this->data);
    break;
  }
  free(this->name);
  free(this);
}

/*
** Find an object in an object list, given the name.  Create if needed.
*/

static object* findobjname(objheader* hdr, char* name, bool create)
{
  object* obj;

  if (hdr->cache != nil) {
    if (strcmp(hdr->cache->name, name) == 0) {
      return(hdr->cache);
    }
  }

  obj = hdr->first;
  while (obj != nil) {
    if (strcmp(obj->name, name) == 0) {
      hdr->cache = obj;
      return(obj);
    }
    obj = obj->next;
  }

  if (create) {
    obj = makeobject(hdr, name);
  }

  return(obj);
}

/*
** Find an object given its index.
*/

static object* findobjindex(objheader* hdr, objindex index)
{
  object* obj;
  
  if (hdr->cache != nil) {
    if (hdr->cache->index == index) {
      return(hdr->cache);
    }
  }

  obj = hdr->first;
  while (obj != nil) {
    if (obj->index == index) {
      hdr->cache = obj;
      return(obj);
    }
    obj = obj->next;
  }
  return(nil);
}

/*
** Find object data given object index.
*/

static void* findobjdata(objheader* hdr, objindex index)
{
  object* obj;

  obj = findobjindex(hdr, index);
  if (obj != nil) {
    return(obj->data);
  }
  return(nil);
}

/*
** obj_name() translates an object index to a name.
*/

static char* obj_name(objheader* hdr, objindex index)
{
  object* obj;

  obj = findobjindex(hdr, index);
  if (obj != nil) {
    return(obj->name);
  }
  return(nil);
}

/*
** obj_index() translates an object name to an index.
*/

static objindex obj_index(objheader* hdr, char* name)
{
  object* obj;

  obj = findobjname(hdr, name, false);
  if (obj != nil) {
    return(obj->index);
  }
  return(0);
}

/*
** obj_define() defines an object.
*/

static objindex obj_define(objheader* hdr, char* name)
{
  object* obj;

  obj = findobjname(hdr, name, true);
  if (obj != nil) {
    return(obj->index);
  }
  return(0);
}

/*
** obj_clear() removes all objects from an object list.
*/

static void obj_clear(objheader* hdr)
{
  object* this;
  object* next;

  this = hdr->first;
  while (this != nil) {
    next = this->next;
    zapobject(hdr, this);
    this = next;
  }
  hdr->first = nil;
  hdr->last = nil;
  hdr->cache = nil;
  hdr->count = 0;
  hdr->nextindex = 0;
}

/*
** obj_delete() deletes the specified object.  If the specified object
** does not exist, we do nothing.
*/

static void obj_delete(objheader* hdr, objindex index)
{
  object* obj;

  obj = findobjindex(hdr, index);
  if (obj != nil) {		/* Does it exist? */

    if (obj->prev != nil) {	/* Yes, unlink from header. */
      obj->prev->next = obj->next;
    } else {
      hdr->first = obj->next;
    }
    if (obj->next != nil) {
      obj->next->prev = obj->prev;
    } else {
      hdr->last = obj->prev;
    }

    zapobject(hdr, obj);	/* Goodbye. */
    hdr->cache = nil;		/* Play safe. */

    hdr->count -= 1;		/* One less to care about. */
  }
}

/*
** obj_next() is used to step over all objects in a list.  If the argument
** is zero, we return the first object.  If the argument is non-zero,
** we return the next known object after that one.  If there are no
** more objects, we return zero.
*/

static objindex obj_next(objheader* hdr, objindex index)
{
  object* obj;

  if (index == 0) {
    obj = hdr->first;
    while ((obj != nil) && (obj->index == 0)) {
      obj = obj->next;
    }
    if (obj == nil) {
      return(0);
    }
    hdr->cache = obj;		/* Store in cache. */
    return(obj->index);
  }

  obj = findobjindex(hdr, index);
  if (obj == nil) {
    return(0);
  }

  obj = obj->next;
  while ((obj != nil) && (obj->index == 0)) {
    obj = obj->next;
  }

  if (obj == nil) {
    return(0);
  }

  hdr->cache = obj;		/* Store in cache. */
  return(obj->index);
}

/**********************************************************************/

/*
** hl_count() returns the number of defined highlight points.
*/

counter hl_count(void)
{
  return(hlheader.count);
}

/*
** hl_next() is used to step over all defined highlight points.  If the
** argument is zero, we return the first point.  If the argument is non-zero,
** we return the next defined point after that one.  If there are no
** more points, we return zero.
*/

objindex hl_next(objindex index)
{
  return(obj_next(&hlheader, index));
}

/*
** hl_delete() deletes the specified highlight point.
*/

void hl_delete(objindex index)
{
  obj_delete(&hlheader, index);
  wc_highlight();
}

/*
** hl_clear() removes all defined highlight points from the database.
*/

void hl_clear(void)
{
  obj_clear(&hlheader);
  wc_highlight();
}

/*
** hl_read() returns the highlight address given the index.
*/

address* hl_read(objindex index)
{
  hlpoint* hlp;

  hlp = findobjdata(&hlheader, index);
  if (hlp != nil) {
    return(hlp->addr);
  }
  return(nil);			/* Maybe return a_zero()? */
}

/*
** hl_write() creates a new highlight point, and sets up data structures.
*/

void hl_write(address* addr)
{
  object* obj;
  hlpoint* hlp;

  obj = makeobject(&hlheader, nil);
  if (obj != nil) {
    hlp = obj->data;
    if (hlp != nil) {
      hlp->addr = a_copy(addr, nil);
      hlp->handle = setwatch(addr, w_highlight, true);
      wc_highlight();
    }
  }
}

/**********************************************************************/

/*
** n_count() returns the number of defined notes.
*/

counter n_count(void)
{
  return(notheader.count);
}

/*
** n_next() is used to step over all defined notes.  If the argument
** is zero, we return the first note.  If the argument is non-zero,
** we return the next defined note after that one.  If there are no
** more notes, we return zero.
*/

objindex n_next(objindex index)
{
  return(obj_next(&notheader, index));
}

/*
** n_delete() deletes the specified note.
*/

void n_delete(objindex index)
{
  obj_delete(&notheader, index);
  wc_notes();
}

/*
** n_clear() removes all defined notes from the database.
*/

void n_clear(void)
{
  obj_clear(&notheader);
  wc_notes();
}

/*
** n_read() returns the actual note given the index.
*/

char* n_read(objindex index)
{
  return(findobjdata(&notheader, index));
}

/*
** n_write() creates a new note block, and fills it in.
*/

void n_write(char* txt)
{
  object* obj;

  obj = makeobject(&notheader, nil);
  if (obj != nil) {
    obj->data = copystring(txt, obj->data);
    wc_notes();
  }
}

/**********************************************************************/

/*
** p_length() returns the length of a pattern chain.
*/

int p_length(pattern* p)
{
  int i;

  i = 0;
  while (p != nil) {
    /* i += p->count; */
    i += 1;
    p = p->next;
  }
  return(i);
}

/*
** p_free() deallocates a pattern chain.
*/

void p_free(pattern* p)
{
  pattern* next;

  while (p != nil) {
    next = p->next;
    free(p);
    p = next;
  }
}

/*
** p_new() allocates a new pattern element.
*/

pattern* p_new(void)
{
  pattern* p;

  p = (pattern*) malloc(sizeof(pattern));
  p->next = nil;
  p->status = st_none;
  p->length = 0;
  return(p);
}

/*
** p_copy() makes (allocates) a copy of a pattern.
*/

pattern* p_copy(pattern* src, pattern* dst)
{
  if (src == nil) {
    p_free(dst);
    return(nil);
  }
  if (dst == nil) {
    dst = p_new();
  }
  if (dst != nil) {
    dst->status = src->status;
    dst->length = src->length;
    dst->next = p_copy(src->next, dst->next);
  }
  return(dst);
}

/*
** p_name() translates a pattern index to a name.  If there is no
** pattern with the given index, we return nil.
*/

char* p_name(patindex index)
{
  return(obj_name(&patheader, index));
}

/*
** p_index() translates a pattern name to an index.
*/

patindex p_index(char* name)
{
  return(obj_index(&patheader, name));
}

/*
** p_define() creates (defines) a pattern with the specified name,
** and returns the index of the new pattern.  If the pattern already
** existed, nothing (except returning the index) will happen.
*/

patindex p_define(char* name)
{
  return(obj_define(&patheader, name));
}

/*
** p_count() returns the number of currently defined patterns.
*/

counter p_count(void)
{
  return(patheader.count);
}

/*
** p_next() is used to step over all known patterns.  If the argument
** is zero, we return the first pattern.  If the argument is non-zero,
** we return the next known pattern after that one.  If there are no
** more patterns, we return zero.
*/

patindex p_next(patindex index)
{
  return(obj_next(&patheader, index));
}

/*
** p_delete() deletes the specified pattern.  If the specified pattern
** does not exist, we do nothing.
*/

void p_delete(patindex index)
{
  obj_delete(&patheader, index);
  wc_patterns();
}

/*
** p_clear() removes all patterns from the database.
*/

void p_clear(void)
{
  obj_clear(&patheader);
  wc_patterns();
}

/*
** p_read() returns the actual pattern given an index.
*/

pattern* p_read(patindex index)
{
  return(findobjdata(&patheader, index));
}

/*
** p_write() sets the a new pattern for the given index.
*/

void p_write(patindex index, pattern* p)
{
  object* obj;

  obj = findobjindex(&patheader, index);
  if (obj != nil) {
    obj->data = p_copy(p, obj->data);
    wc_patterns();
  }
}

/**********************************************************************/

/*
** makeregsub() creates a new regsub block.  No value is assigned at
** this point, and we don't link the block into anything.
*/

static regsub* makeregsub(address* addr)
{
  regsub* rs;

  rs = malloc(sizeof(regsub));

  if (rs != nil) {
    rs->addr = a_copy(addr, nil);
    rs->val = nil;
  }
  return(rs);
}

/*
** linkregsub() links a regsub block into the chain for a register.
*/

static void linkregsub(regsub* rs, regdb* r)
{
  if (r->sublist == nil) {	/* Simple case? */
    rs->prev = nil;
    rs->next = nil;
    r->sublist = rs;
  } else {
    regsub* prev;
    regsub* next;
    /*
    ** start with linking this block into the list, sorted. If the new
    ** block starts at the same address as an existing one, the new block
    ** goes last.
    */
    prev = nil;
    next = r->sublist;
    while ((next != nil) && (a_ge(rs->addr, next->addr))) {
      prev = next;
      next = next->next;
    }
    /*
    ** now "prev" and "next" is set up.  Build the links.
    */
    rs->prev = prev;
    rs->next = next;
    if (prev != nil) {
      prev->next = rs;
    } else {
      r->sublist = rs;
    }
    if (next != nil) {
      next->prev = rs;
    }
    /*
    ** Now we check if the previous block overlaps with this one in any way.
    */
    if (prev != nil) {
      /*
      ** we have a previous block.  check for overlap.
      */
      if (a_le(rs->addr, a_last(prev->addr))) {
	/*
	** we start before end of previous, have to fix.
	*/
	if (a_gt(rs->addr, prev->addr)) {
	  /*
	  ** we start after beginning of previous, split off beginning of
	  ** previous and set up new previous to remainder of block.
	  */
	}
	/*
	** now we start at same address as previous.  delete either
	** first part of previous or whole block.
	*/
      }
    }
    /*
    ** now fixup all trailing blocks that we touch.
    */

    /* ... */

  }
}

/*
** findregsub() looks up the subval block corresponding to the given
** address, possibly creating it if needed.
*/

static regsub* findregsub(regdb* r, address* addr, bool create)
{
  regsub* rs;

  /* This might be needed for overlapping writes to work... */

  if (create) {
    rs = makeregsub(addr);
    linkregsub(rs, r);
    r->subcache = rs;
    return(rs);
  }

  if (r->subcache != nil) {
    if (a_inside(addr, r->subcache->addr)) {
      return(r->subcache);
    }
  }

  for (rs = r->sublist; rs != nil; rs = rs->next) {
    if (a_inside(addr, rs->addr)) {
      r->subcache = rs;
      return(rs);
    }
  }

  return(nil);
}

/*
** r_name() translates a register index to a name.  If there is no
** register with the given index, we return nil.
*/

char* r_name(regindex index)
{
  return(obj_name(&regheader, index));
}

/*
** r_type() returns the type of a register.
*/

int r_type(regindex index)
{
  regdb* r;

  r = findobjdata(&regheader, index);
  if (r != nil) {
    return(r->type);
  }
  return(vty_none);
}

/*
** r_index() translates a register name to an index.
*/

regindex r_index(char* name)
{
  return(obj_index(&regheader, name));
}

/*
** r_define() creates (defines) a register with the specified name,
** and returns the index of the new register.  If the register already
** existed, nothing (except returning the index) will happen.
*/

regindex r_define(char* name, int type)
{
  objindex index;
  regdb* r;

  index = obj_define(&regheader, name);
  r = findobjdata(&regheader, index);
  if (r != nil) {
    r->type = type;
  }
  com_rcre(index);
  return(index);
}

/*
** r_delete() deletes the specified register.  If the specified register
** does not exist, we do nothing.
*/

void r_delete(regindex index)
{
  com_rdel(index);
  obj_delete(&regheader, index);
  com_rdel(0);
}

/*
** r_next() is used to step over all known registers.  If the argument
** is zero, we return the first register.  If the argument is non-zero,
** we return the next known register after that one.  If there are no
** more registers, we return zero.
*/

regindex r_next(regindex index)
{
  return(obj_next(&regheader, index));
}

/*
** r_subrange() returns the next subrange in which a register has a non-
** standard value.  If the address argument is nil, we return the first
** range, and so on much like r_next() does with registers.
*/

address* r_subrange(regindex index, address* addr)
{
  regdb* r;
  regsub* rs;

  r = findobjdata(&regheader, index);
  if (r == nil) {
    return(nil);
  }

  if (addr == nil) {
    rs = r->sublist;
    if (rs != nil) {
      return(rs->addr);
    }
    return(nil);
  }

  rs = findregsub(r, addr, false);
  if (rs != nil) {
    rs = rs->next;
    if (rs != nil) {
      return(rs->addr);
    }
    return(nil);
  }

  return(nil);
}

/*
** r_read() returns the value of the specified register.
*/

value* r_read(regindex index, address* addr)
{
  regdb* r;
  regsub* rs;

  r = findobjdata(&regheader, index);
  if (r != nil) {
    if (addr != nil) {
      rs = findregsub(r, addr, false);
      if (rs != nil) {
	return(rs->val);
#if 0
      } else {
	return(r->mainval);
#endif
      }
    }
    return(r->mainval);
  }
  return(nil);
}

/*
** r_write() stores a value in the specified register.
*/

void r_write(regindex index, address* addr, value* val)
{
  regdb* r;
  regsub* rs;

  r = findobjdata(&regheader, index);
  if (r != nil) {
    if (addr == nil) {
      r->mainval = v_copy(val, r->mainval);
    } else {
      address* this;

      while (addr != nil) {
	this = a_car(addr);
	addr = a_cdr(addr);

	rs = findregsub(r, this, true);
	if (rs != nil) {
	  rs->val = v_copy(val, rs->val);
	}	
      }
    }
    wc_register(index);
  }
}

/*
** r_isdef() checks if the specified register is defined for the
** given address.
*/

bool r_isdef(regindex index, address* addr)
{
  regdb* r;
  regsub* rs;

  r = findobjdata(&regheader, index);

  if (r == nil) {
    return(false);
  }

  if (r->mainval != nil) {
    return(true);
  }

  if (addr != nil) {
    rs = findregsub(r, addr, false);
    if (rs != nil) {
      return(true);
    }
  }
  return(false);
}

/*
** r_clear() removes all registers from the database.
*/

void r_clear(void)
{
  object* this;

  for (this = regheader.first; this != nil; this = this->next) {
    com_rdel(this->index);
  }
  obj_clear(&regheader);
  com_rdel(0);
}

/*
** r_count() returns the number of currently defined registers.
*/

counter r_count(void)
{
  return(regheader.count);
}

/**********************************************************************/

/*
** s_name() translates a symbol index to a name.  If there is no
** symbol with the given index, we return nil.
*/

char* s_name(symindex index)
{
  return(obj_name(&symheader, index));
}

/*
** s_index() translates a symbol name to an index.
*/

symindex s_index(char* name)
{
  return(obj_index(&symheader, name));
}

/*
** s_define() creates (defines) a symbol with the specified name,
** and returns the index of the new symbol.  If the symbol already
** existed, nothing (except returning the index) will happen.
*/

symindex s_define(char* name)
{
  symindex index;

  index = obj_define(&symheader, name);
  com_scre(index);
  return(index);
}

/*
** s_delete() deletes the specified symbol.  If the specified symbol
** does not exist, we do nothing.
*/

void s_delete(symindex index)
{
  com_sdel(index);
  obj_delete(&symheader, index);
  com_sdel(0);
}

/*
** s_clear() removes all symbols from the database.
*/

void s_clear(void)
{
  object* this;

  for (this = symheader.first; this != nil; this = this->next) {
    com_sdel(this->index);
  }
  obj_clear(&symheader);
  com_sdel(0);
}

/*
** s_count() returns the number of currently defined symbols.  This
** routine needs to be fixed as soon as s_delete() starts to work.
*/

counter s_count(void)
{
  return(symheader.count);
}

/*
** s_next() is used to step over all known symbols.  If the argument
** is zero, we return the first symbol.  If the argument is non-zero,
** we return the next known symbol after that one.  If there are no
** more symbols, we return zero.
*/

symindex s_next(symindex index)
{
  return(obj_next(&symheader, index));
}

/*
** s_read() returns the actual symbol given an index.
*/

char* s_read(symindex index)
{
  return(findobjdata(&symheader, index));
}

/*
** s_write() sets the value of a symbol.
*/

void s_write(symindex index, char* val)
{
  object* obj;

  obj = findobjindex(&symheader, index);
  if (obj != nil) {
    obj->data = copystring(val, obj->data);
    com_sset(index);
  }
}

/**********************************************************************/

/*
** v_new() allocates a new value block.
*/

value* v_new(int type)
{
  value* v;

  v = malloc(sizeof(value));
  if (v != nil) {
    v->type = type;
    v->pdata = nil;
    v->idata = 0;
  }
  return(v);
}

/*
** v_copy() copies a value block.
*/

value* v_copy(value* src, value* dst)
{
  if (src == nil) {
    v_free(dst);
    return(nil);
  }
  if (dst == nil) {
    dst = v_new(src->type);
  }
  if (dst != nil) {
    if (src->type != dst->type) {
      v_free(dst);
      dst = v_new(src->type);
    }
    switch (src->type) {
    case vty_addr:
      dst->pdata = a_copy(src->pdata, dst->pdata);
      break;
    case vty_long:
      dst->idata = src->idata;
      break;
    }
  }
  return(dst);
}

/*
** v_free() deallocates a value block.
*/

void v_free(value* v)
{
  if (v != nil) {
    switch (v->type) {
    case vty_addr:
      a_free(v->pdata);
      break;
    }
    free(v);
  }
}

/*
** v_type() returns the type of a value.
*/

int v_type(value* v)
{
  if (v != nil) {
    return(v->type);
  }
  return(vty_none);
}

/*
** v_a2v() returns a pointer to a value block, with the given address
** as content.
*/

value* v_a2v(address* a)
{
  static value* work = nil;

  if (work != nil) {
    v_free(work);
  }
  work = v_new(vty_addr);
  work->pdata = a_copy(a, work->pdata);
  return(work);
}

/*
** v_v2a() returns the address from a value.
*/

address* v_v2a(value* v)
{
  if (v != nil) {
    if (v->type == vty_addr) {
      return((address*) v->pdata);
    }
  }
  return(nil);
}

/*
** v_l2v() returns a pointer to a value block, with the given longword
** as contents.
*/

value* v_l2v(longword l)
{
  static value* work = nil;

  if (work != nil) {
    v_free(work);
  }
  work = v_new(vty_long);
  work->idata = l;
  return(work);
}

/*
** v_v2l() returns the longword from a value.
*/

longword v_v2l(value* v)
{
  if (v != nil) {
    if (v->type == vty_long) {
      return(v->idata);
    }
  }
  return(0);
}

/*
** v_eq() compares two values.
*/

bool v_eq(value* a, value* b)
{
  if ((a == nil) || (b == nil)) {
    return(false);
  }
  if (a->type != b->type) {
    return(false);
  }
  switch (a->type) {
  case vty_none:
    return(true);		/* ??? */
  case vty_addr:
    return(a_eq(a->pdata, b->pdata));
  case vty_long:
    return(a->idata == b->idata);
  }
  return(false);
}

/**********************************************************************/

void m_purge(void)
{
  /* nothing at the moment. */
}

/**********************************************************************/

static addresshandler* itemhandler;

static void scaninfo(pdb* p)
{
  infoblock* info;

  info = p->info;
  while (info != nil) {
    infocache = info;
    itemhandler(info->addr);	/* *** get addr from PDB *** */
    info = info->next;
  }
}

void foreach(addresshandler* handler)
{
  itemhandler = handler;
  scanmemory(scaninfo);
}

/**********************************************************************/

/*

  The "saved status" file is a text file, containing a number
  of lines, each looking thus:

  Caddr:value

  The first character is a letter (usually) telling what type of
  line this is.  Next comes an address, in hex, with as many hex
  digits as are needed.  The address is delimited by a colon, and
  after that comes the data that we save.  For functions that do
  not have an address the address field is simply blank, and the
  colon is missing.

  This is not quite true at the moment, but it will be.

  The line types and the corresponding characters are:

  A -- Inline arguments for routine at address.
  B -- Various bits, noreturn flag, delay slot info, ...
  C -- comment.
  D -- description.
  E -- expansion.
  F -- flags.  (right now a longword, will be a bit vector.)
  L -- label.
  M -- memory data.  (if we are saving it.)
  N -- notice/note.
  P -- processor (cpu) type.
  R -- register.
  S -- status.
  U -- unique number seed, if non-zero.

  $ -- symbol.
  # -- pattern.
  ; -- comment line.

  All other initial characters are reserved.  Please note that the hash
  mark ("#") does NOT start a comment.

*/

/*
** save all the status bytes from a pdb:
*/

static void save_status(pdb* p)
{
  int i;

  if (p->status == nil) return;

  wf_wchar('S');
  wf_waddr(p->addr);
  for (i = 0; i < 64; i += 1) {
    wf_wchar("-.IBWLQOCTPMFD??"[(p->status[i] & stb_code)]);
  }
  wf_newline();

  wf_wchar('S');
  wf_waddr(a_offset(p->addr, 64));
  for (i = 64; i < 128; i += 1) {
    wf_wchar("-.IBWLQOCTPMFD??"[(p->status[i] & stb_code)]);
  }
  wf_newline();

  wf_wchar('S');
  wf_waddr(a_offset(p->addr, 128));
  for (i = 128; i < 192; i += 1) {
    wf_wchar("-.IBWLQOCTPMFD??"[(p->status[i] & stb_code)]);
  }
  wf_newline();

  wf_wchar('S');
  wf_waddr(a_offset(p->addr, 192));
  for (i = 192; i < 256; i += 1) {
    wf_wchar("-.IBWLQOCTPMFD??"[(p->status[i] & stb_code)]);
  }
  wf_newline();
}

static void save_bits(infoblock* i)
{
  wf_wchar('B');
  wf_waddr(i->addr);
  wf_w2hex(i->bits);		/* FIX THIS! */
  wf_newline();
}

static void save_string(infoblock* i, strtype s)
{
  if (i->str[s] != nil) {
    switch (s) {
      case styp_com: wf_wchar('C'); break;
      case styp_dsc: wf_wchar('D'); break;
      case styp_exp: wf_wchar('E'); break;
      case styp_lbl: wf_wchar('L'); break;
    }
    wf_waddr(i->addr);
    if (s == styp_exp) {
      wf_whex(i->explength);
      wf_wchar(':');
    }
    wf_wstr(i->str[s]);
    wf_newline();
  }
}

/* *** In the code below, get addr from PDB *** */

static void save_attributes(pdb* p)
{
  infoblock* info;
  pattern* pe;

  for (info = p->info; info != nil; info = info->next) {
    if (info->str[styp_lbl] != nil) {
      save_string(info, styp_lbl);
    }
    if (info->args != nil) {
      wf_wchar('A');
      wf_waddr(info->addr);
      for (pe = info->args; pe != nil; pe = pe->next) {
	wf_wchar(' ');
	wf_wchar("-.IBWLQOCTPMFD??"[pe->status]);
	wf_whex(pe->length);
      }
      wf_newline();
    }
    if (info->bits != 0) {
      save_bits(info);
    }
    if (info->str[styp_com] != nil) {
      save_string(info, styp_com);
    }
    if (info->str[styp_dsc] != nil) {
      save_string(info, styp_dsc);
    }
    if (info->str[styp_exp] != nil) {
      save_string(info, styp_exp);
    }
    /* Save flags here. */
  }
}

static void savelevel(pmp* page, int level)
{
  int i;
  
  for (i = 0; i < 256; i += 1) {
    if (page->ptr[i] != nil) {
      if (level > 1) {
	savelevel(page->ptr[i], level - 1);
      } else {
	save_status(page->ptr[i]);
	save_attributes(page->ptr[i]);
      }
    }
  }
}

static void save_pdbs(void)
{
  l_cflags();			/* No need to save internal flags. */
  savelevel(&page0, pmpdepth);
}

/*
** save_notes() saves all note blocks to the file.
*/

static void save_notes(void)
{
  object* obj;

  obj = notheader.first;

  while (obj != nil) {
    if (obj->index != 0) {
      if (obj->data != nil) {
	wf_wchar('N');
	wf_wstr(obj->data);
      }
      wf_newline();
    }
    obj = obj->next;
  }
}

/*
** save_patterns() saves all named patterns to the file.
*/

static void save_patterns(void)
{
  object* pat;
  pattern* pe;

  pat = patheader.first;

  while (pat != nil) {
    if (pat->index != 0) {
      wf_wchar('#');
      wf_wname(pat->name);
      wf_wchar(':');
      pe = pat->data;
      while (pe != nil) {
	wf_wchar(' ');
	wf_wchar("-.IBWLQOCTPMFD??"[pe->status]);
	wf_whex(pe->length);
	pe = pe->next;
      }
      wf_newline();
    }
    pat = pat->next;
  }
}

/*
** save_registers() saves all known registers.  Format:
**
** R<name>:<type>[:<value>]
** R<name>:@<addr>:<value>
**
** Example:
**
** Rcs:L:1000
** Rcs:@4000-4fff:2000
*/

static void save_registers(void)
{
  object* reg;
  regdb* r;
  regsub* rs;

  for (reg = regheader.first; reg != nil; reg = reg->next) {
    if (reg->index != 0) {
      r = reg->data;
      wf_wchar('R');
      wf_wname(reg->name);
      wf_wchar(':');
      switch (r->type) {
	case vty_long: wf_wchar('L'); break;
	case vty_addr: wf_wchar('A'); break;
	default:  wf_wchar('?'); break;
      }
      if (r->mainval != nil) {
	wf_wchar(':');
	switch (r->type) {
	  case vty_long: wf_whex(v_v2l(r->mainval)); break;
	  case vty_addr: wf_waddr(v_v2a(r->mainval)); break;
	}
      }
      wf_newline();
      for (rs = r->sublist; rs != nil; rs = rs->next) {
	wf_wchar('R');
	wf_wname(reg->name);
	wf_wstr(":@");
	wf_waddr(rs->addr);
	switch (r->type) {
	  case vty_long: wf_whex(v_v2l(rs->val)); break;
	  case vty_addr: wf_waddr(v_v2a(rs->val)); break;
	}
	wf_newline();
      }
    }
  }
}

static void save_symbols(void)
{
  object* sym;

  for (sym = symheader.first; sym != nil; sym = sym->next) {
    if (sym->index != 0) {
      wf_wchar('$');
      wf_wname(sym->name);
      if (sym->data != nil) {
	wf_wchar(':');
	wf_wstr(sym->data);
      }
      wf_newline();
    }
  }
}

static void save_data(void)
{
  segment* s;
  int i, j;

  for (s = segmentlist; s != nil; s = s->next) {
    wf_wstr("; data segment, 0x");
    wf_whex(s->length);
    wf_wstr(" bytes");
    wf_newline();
    setpc(s->first);
    for (i = 0; i < s->length; i += 32) {
      j = s->length - i;
      if (j > 32) j = 32;
      wf_wchar('M');
      wf_waddr(pc);
      while (j-- > 0) {
	wf_w2hex(getnext());
      }
      wf_newline();
    }
  }
}

/*
** m_save() implements the SAVE command.
*/

void m_save(char* filename)
{
  if (iocheck(wf_wopen(filename))) {
    if (processor != nil) {
      wf_wchar('P');
      wf_wstr(processor->name);
      wf_newline();
    }
    if (uniqlong != 0) {
      wf_wchar('U');
      wf_whex(uniqlong);
      wf_newline();
    }

    save_notes();
    save_symbols();
    save_patterns();
    save_registers();
    save_pdbs();
    save_data();

    wf_close();
  }
}

/**********************************************************************/

static pattern* parspat(char* line)
{
  pattern* p;

  p = p_new();

  /* code missing */

  return(p);
}

static void statvector(address* addr, char* codes)
{
  char c;
  pdb* p;
  int count;
  stcode s;

  while (*codes != (char) 0) {
    p = findpdb(addr, true);
    makestatus(p);		/* Make sure there is a status buffer. */
    count = 0;
    while ((c = *(codes++)) != (char) 0) {

      /* use char2st here. */

      switch (c) {
	case '-': s = st_none;   break;
	case '.': s = st_cont;   break;
	case 'B': s = st_byte;   break;
	case 'C': s = st_char;   break;
	case 'D': s = st_double; break;
	case 'F': s = st_float;  break;
	case 'I': s = st_inst;   break;
	case 'L': s = st_long;   break;
	case 'M': s = st_mask;   break;
	case 'O': s = st_octa;   break;
	case 'P': s = st_ptr;    break;
	case 'Q': s = st_quad;   break;
	case 'T': s = st_text;   break;
	case 'W': s = st_word;   break;
	default:  s = st_none;   break;
      }
      if (s != st_none) {
	p->status[pageoffset] &= ~stb_code;
	p->status[pageoffset] |= s;
      }
      count += 1;
      if (pageoffset == 0xff) {
	break;
      }
      pageoffset += 1;
    }
    if (c == (char) 0) {
      break;
    }
    a_inc(addr, count);
  }
}

static void memobj(void)
{
  address* addr;
  int count;
  byte buffer[256];		/* Should be at most 32, but... */

  addr = wf_raddr();
  if (addr != nil) {
    count = 0;
    while (wf_is2hex() && (count < 256)) {
      buffer[count++] = wf_rchar();
    }
    memstore(addr, buffer, count);
  }
}

static void addrobj(char type)
{
  address* addr;
  char* line;
  longword length;

  addr = wf_raddr();
  if (addr != nil) {
    if (type == 'E') {
      length = wf_rhex();
      if (wf_rchar() != ':') {
	return;
      }
    }
    line = wf_rstr();
    if (line != nil) {
      switch (type) {
      case 'A':
	ia_write(addr, parspat(line));
	break;
      case 'B':
	/* set bits. */
	break;
      case 'C':
	c_insert(addr, line);
	break;
      case 'D':
	d_insert(addr, line);
	break;
      case 'E': 
	e_insert(addr, line, length);
	break;
      case 'F':
	/* waiting for new format. */
	break;
      case 'L':
	l_insert(addr, line);
	break;
      case 'S':
	statvector(addr, line);
	break;
      }
    }
  }
}

/*
** regobj() reads a register spec from the input file.
*/

static void regobj(char* name)
{
  /* write this! */
}

/*
** nameobj() reads a named object of the given type from the input file.
*/

static void nameobj(char type)
{
  static char* name = nil;
  
  name = copystring(wf_rname(), name);
  if (name != nil) {
    switch (type) {
    case '#':
      p_write(p_define(name), parspat(wf_rstr()));
      break;
    case '$':
      s_write(s_define(name), wf_rstr());
      break;
    case 'R':
      regobj(name);
      break;
    }
  }
}

/*
** m_restore() implements the RESTORE command.
*/

void m_restore(char* filename)
{
  char type;

  if (iocheck(wf_ropen(filename))) {
    while (!wf_ateof()) {
      type = wf_rchar();
      switch (type) {
      case ';':			/* Ignore comment lines in file. */
	break;
      case '#':			/* Pattern. */
      case '$':			/* Symbol. */
      case 'R':			/* Register. */
	nameobj(type);
	break;
      case 'N':			/* Note. */
	n_write(wf_rstr());
	break;
      case 'A':			/* Arguments (inline). */
      case 'B':			/* Bits. */
      case 'C':			/* Comment. */
      case 'D':			/* Description. */
      case 'E':			/* Expansion. */
      case 'F':			/* Flags. */
      case 'L':			/* Label. */
      case 'S':			/* Status vector. */
	addrobj(type);
	break;
      case 'M':			/* Memory data. */
	memobj();
	break;
      case 'P':			/* Processor type. */
	setproc(findproc(wf_rstr()));
	break;
      case 'U':			/* Unique seed. */
	uniqlong = wf_rhex();
	break;
      }
      wf_nextline();
    }
    wf_close();
  }
}

/**********************************************************************/

/*
** uniq() returns a unique number, useful for generating labels.
*/

longword uniq(void)		/* Return a unique number. */
{
  uniqlong += 1;
  return(uniqlong);
}

/*
** getscount() returns the number of segmens we have at the moment.
*/

int getscount(void)
{
  int i;
  segment* s;

  i = 0;
  s = segmentlist;
  while (s != nil) {
    i += 1;
    s = s->next;
  }
  return(i);
}

/*
** getsaddr() returns segment number of a given address, or zero.
*/

int getsaddr(address* addr)
{
  segment* s;
  int n;

  n = 0;
  s = segmentlist;
  while (s != nil) {
    n += 1;
    if (a_compare(s->first, addr) <= 0) {
      if (a_compare(s->last, addr) >= 0) {
	return(n);
      }
    }
    s = s->next;
  }
  return(0);
}

/*
** getsfirst() returns the first address of segment n.
*/

address* getsfirst(int n)
{
  static address* work = nil;
  segment* s;

  s = segmentlist;
  while (s != nil) {
    if (--n == 0) {
      work = a_copy(s->first, work);
      return(work);
    }
    s = s->next;
  }
  return(nil);
}

/*
** getslast() returns the last address of segment n.
*/

address* getslast(int n)
{
  static address* work = nil;
  segment* s;

  s = segmentlist;
  while (s != nil) {
    if (--n == 0) {
      work = a_copy(s->last, work);
      return(work);
    }
    s = s->next;
  }
  return(nil);
}

/*
** getslength() returns the length of segment n.
*/

longword getslength(int n)
{
  segment* s;

  s = segmentlist;
  while (s != nil) {
    if (--n == 0) {
      return(s->length);
    }
    s = s->next;
  }
  return(0L);
}

/*
** getsrest() returns the number of remaining bytes of the segment
** that an address belongs to, from that address.  If the address
** in question is not mapped, i.e. has no segment, we return zero.
*/

longword getsrest(address* addr)
{
  int seg;

  seg = getsaddr(addr);		/* Select segment. */
  if (seg != 0) {		/* If we have one, return remaining bytes. */
    return(a_diff(getslast(seg), addr) + 1);
  }
  return(0);
}

/*
** getmemory() returns the memory contents of the specified address.  If
** there is no memory mapped at that address, we return zero.
*/

byte getmemory(address* addr)
{
  pdb* p;

  p = findpdb(addr, false);
  if ((p != nil) && (p->data != nil)) {
    return(p->data[pageoffset]);
  }		 
  return(0x00);
}

/*
** setnext()/getnext() is an attempt to speed up the getting of
** sequential bytes of memory.
*/

void setnext(address* addr)
{
  nextpdb = findpdb(addr, false);
  nextoffset = pageoffset;
}

byte getnext(void)
{
  byte b;

  a_inc(pc, 1);
  if ((nextpdb != nil) && (nextpdb->data != nil)) {
    b = nextpdb->data[nextoffset];
    if (nextoffset == 0xff) {
      nextpdb = findpdb(pc, false);
      nextoffset = 0x00;
    } else {
      nextoffset += 1;
    }
    return(b);
  }
  return(0x00);
}

/*
** getstatus() returns the status code for the specified address.  If
** there is no memory mapped at that address, we return st_none.
*/

stcode getstatus(address* addr)
{
  return(getstc(addr) & stb_code);
}

/*
** mapnostat() returns true if the given address is mapped and the status
** is st_none.
*/

bool mapnostat(address* addr)
{
  byte status;

  status = getstc(addr);
  if (status & stb_nxm) return(false);
  if ((status & stb_code) != st_none) return(false);
  return(true);
}

/*
** mapped() returns true if the given address is mapped.
*/

bool mapped(address* addr)
{
  if (getstc(addr) & stb_nxm) {
    return(false);
  }
  return(true);
}

/*
** setstatus() sets the status code for the given address to the given
** value, with the following (size-1) bytes set to status st_cont.
*/

void setstatus(address* addr, stcode status, int size)
{
  pdb* p;
  int offset;

  offset = 0;
  status &= stb_code;		/* Junk excess bits, if any. */
  p = findpdb(addr, true);	/* Initial PDB. */
  while (size > 0) {
    makestatus(p);
    while (size > 0) {
      size -= 1;
      offset += 1;
      p->status[pageoffset] &= ~stb_code;
      p->status[pageoffset] |= status;
      status = st_cont;
      if (pageoffset == 0xff) {
	break;
      }
      pageoffset += 1;
    }
    if (size > 0) {
      p = findpdb(a_offset(addr, offset), true);
    }
  }
}

void suggest(address* addr, stcode status, int size)
{
  if (updateflag) {
    if (addr != nil) {
      if (getstatus(addr) == st_none) {
	setstatus(addr, status, size);
      }
    }
  }
}

/**********************************************************************/

/*
** load_binary is the routine that loads plain binary files.
*/

bool load_binary(char* filename, address* addr, int offset)
{
  byte buffer[256];
  int size;
  
  if (wf_bopen(filename)) {
    if (offset > 0) {
      wf_rskip(offset);
    }
    while ((size = wf_rblock(buffer, 256)) > 0) {
      memstore(addr, buffer, size);
      /* memstore() updates addr for us. */
    }
    wf_close();
    return(true);
  }
  return(false);
}

/*
** help variables for load_intel:
*/

static longword checksum;
static bool ok;

static byte intelbyte(void)
{
  byte b;

  if (wf_is2hex()) {
    b = wf_rchar();
    checksum += b;
  } else {
    ok = false;
    b = 0;
  }
  return(b);
}

bool load_intel(char* filename)
{
  byte buffer[256];
  byte size, type;
  word addr;
  char c;
  int i;
  
  if (iocheck(wf_ropen(filename))) {
    while (!wf_ateof()) {
      c = wf_rchar();
      if (c == '\n') {
	wf_nextline();
      } else if (c == ':') {
	ok = true;
	checksum = 0;
	size = intelbyte();
	addr = intelbyte() << 8;
	addr += intelbyte();
	type = intelbyte();
	for (i = 0; i < size; i += 1) {
	  buffer[i] = intelbyte();
	}
	(void) intelbyte();	/* Checksum. */
	if (ok && ((checksum & 0xff) == 0)) {
	  switch (type) {
	  case 0x00:
	    memstore(a_l2a(addr), buffer, size);
	    break;
	  /* check for other types here, symbol defs.... */
	  }
	}
      }
    }
    wf_close();
    return(true);
  }
  return(false);
}

bool load_motorola(char* filename)
{
  /* To load motorola S-records.  Not yet functional. */

  UNUSED(filename);

  return(false);
}

/*
** m_clear() does the job of the CLEAR STATUS command.
*/

static void mclr_level(pmp* page, int level)
{
  int index;
  pdb* p;

  for (index = 0; index < 256; index += 1) {
    if (page->ptr[index] != nil) {
      if (level > 1) {
	mclr_level(page->ptr[index], level - 1);
      } else {
	p = page->ptr[index];
	if (p->status != nil) {
	  int i;

	  for (i = 0; i <= 256; i += 1) {
	    p->status[i] &= ~stb_code;
	    /* since st_none is 0 we are done. */
	  }
	}
      }
    }
  }
}

void m_clear(void)
{
  mclr_level(&page0, pmpdepth); /* Call recursive hack. */
  wc_total();
}

/*
** m_copy() does the main job of the MEMORY COPY command.
*/

void m_copy(address* fromaddr, address* toaddr)
{
  /* to be written (again). */

  notyeterror();

#if 0

  byte buffer[256];
  int i;

  while (count >= 256) {
    for (i = 0; i < 256; i += 1) {
      buffer[i] = getmemory(fromaddr);
      a_inc(fromaddr, 1);
    }
    memstore(toaddr, buffer, 256);
    count -= 256;
  }
  if (count > 0) {
    for (i = 0; i < count; i += 1) {
      buffer[i] = getmemory(fromaddr);
      a_inc(fromaddr, 1);
    }
    memstore(toaddr, buffer, count);
  }

#endif

}

/*
** m_exclude() does the main job of the MEMORY EXCLUDE command.
*/

void m_exclude(address* addr)
{
  /* to be written. */

  notyeterror();
}

/*
** m_include() does the main job of the MEMORY INCLUDE command.
*/

void m_include(address* addr)
{
  /* to be (re)written. */

  notyeterror();
}

/*
** m_move() does the main job of the MEMORY MOVE command.
*/

void m_move(address* fromaddr, address* toaddr)
{
  /* to be written. */

  notyeterror();
}

/*
** m_relocate() does the main job of the MEMORY RELOCATE command.
*/

void m_relocate(address* srcaddr, address* dstaddr)
{
  /* to be written. */

  notyeterror();
}

/*
** m_set() sets (a single byte of) memory to the specified value.
*/

void m_set(address* addr, int value)
{
  byte buffer[1];

  buffer[0] = value;
  memstore(addr, buffer, 1);
}

/*
** m_test() is the test function for this module.
*/

void m_test(void)
{
  /* nothing to test right now. */

  object* obj;
  regdb*  r;
  regsub* rs;

  for (obj = regheader.first; obj != nil; obj = obj->next) {
    r = obj->data;
    bufstring("register ");
    bufnumber(obj->index);
    bufstring(": def=");
    if (r->mainval != nil) {
      bufvalue(r->mainval);
    } else {
      bufstring("nil");
    }
    bufnewline();
    for (rs = r->sublist; rs != nil; rs = rs->next) {
      tabto(8);
      bufaddress(rs->addr);
      bufstring(": def=");
      bufvalue(rs->val);
      bufnewline();
    }
  }
}