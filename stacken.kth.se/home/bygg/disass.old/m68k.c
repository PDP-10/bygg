/*
** This module implements driver code for the Motorla 68000 family of
** processors.
*/

#include "disass.h"

evf_init m68000_init;
evf_help m68000_help;

struct entryvector m68000_vector = {
  "68000",			/* Name */
  "Motorola 68000",		/* One-liner. */
  m68000_init,			/* Init routine */
  m68000_help,			/* Help routine */
};

evf_init m68010_init;
evf_help m68010_help;

struct entryvector m68010_vector = {
  "68010",			/* Name */
  "Motorola 68010",		/* One-liner. */
  m68010_init,			/* Init routine */
  m68010_help,			/* Help routine */
};

evf_init m68020_init;
evf_help m68020_help;

struct entryvector m68020_vector = {
  "68020",			/* Name */
  "Motorola 68020",		/* One-liner. */
  m68020_init,			/* Init routine */
  m68020_help,			/* Help routine */
};

evf_init m68030_init;
evf_help m68030_help;

struct entryvector m68030_vector = {
  "68030",			/* Name */
  "Motorola 68030",		/* One-liner. */
  m68030_init,			/* Init routine */
  m68030_help,			/* Help routine */
};

evf_init m68040_init;
evf_help m68040_help;

struct entryvector m68040_vector = {
  "68040",			/* Name */
  "Motorola 68040",		/* One-liner. */
  m68040_init,			/* Init routine */
  m68040_help,			/* Help routine */
};

evf_init cpu32_init;
evf_help cpu32_help;

struct entryvector cpu32_vector = {
  "cpu32",			/* Name */
  "Motorola CPU32",		/* One-liner. */
  cpu32_init,			/* Init routine */
  cpu32_help,			/* Help routine */
};

/*
** Global variables, from module common:
*/

extern int radix;		/* Hex, decimal, ... */

/*
** Start of our variables:
*/

static int cputype;		/* 0 for 60000, 1 for 68010, ... */
static int peekserial = 0;	/* Serial number of the call to peek(). */

/*
** The following variables helps us keep track of the sequence
**
**    MOVE #addr, Ax    ;or other direct load of address reg.
**    JMP (Ax)
*/
    
static int moveaserial = 0;	    /* Serial # of peek that loaded reg. */
static address* moveatarget = nil;  /* Address that got loaded. */
static address* moveanext = nil;    /* Address of next instruction. */
static byte moveareg;		    /* Register that got loaded. */
static longword lastimmdata;	    /* Last data used in addr. mode 7/4. */

static word opcode;		/* Global, for instruction decoding. */

static word opreg;		/* bit 2-0 of opcode. */
static word opmode;		/* bit 5-3 of opcode. */
static word opsize;		/* bit 7-6 of opcode. */
static word opmod2;		/* bit 8-6 of opcode. */
static word opreg2;		/* bit 11-9 of opcode. */

/* DO NOT CHANGE THE DEF'S BELOW!  These values comes from opsize! */

#define ops_byte 0		/* Byte size instruction. */
#define ops_word 1		/* Word size instruction. */
#define ops_long 2		/* Long size instruction. */
#define ops_none 3		/* No size, other instr. */

static address* touch;		/* Set up by prea(). */

static bool goodinstruction;

#define reg_pc  -1		/* Internal register number for pc. */
#define reg_sr  -2		/* Internal register number for sr. */
#define reg_ccr -3		/* Internal register number for ccr. */
#define reg_usp -4		/* Internal register number for usp. */

/**********************************************************************/

/*
** Formatters of various kinds:
*/

static char* cstart(void)
{
  if (unixflag) {
    return("#");
  } else {
    return(";");
  }
}

static void octal(longword l)
{
  if (unixflag) {
    if (l > 7) {
      bufchar('0');
    }
    bufoctal(l, 1);
  } else {
    if (l > 7) {
      bufchar('@');
    }
    bufoctal(l, 1);
  }
}

static void decimal(longword l)
{
  bufdecimal(l, 1);  
}

static void hex(longword l)
{
  if (l > 9) {
    if (unixflag) {
      bufstring("0x");
    } else {
      bufchar('$');
    }
  }
  bufhex(l, 1);
}

static void number(longword l)
{
  switch (radix) {
    case 8:  octal(l); break;
    case 10: decimal(l); break;
    case 16: hex(l); break;
    default: hex(l); break;
  }
}

/* output a signed number */

static void signum(long l)
{
  if (l < 0) {
    bufchar('-');
    l = -l;
  }
  number(l);
}

static void genlabel(address* addr)
{
  char work[10];

  if (!l_exist(addr)) {
    sprintf(work, "L%05x", (a_a2l(addr) & 0xfffff));
    while (l_lookup(work) != nil) {
      sprintf(work, "L%x", uniq());
    }
    l_insert(addr, work);
  }
}

static void prop(char* op)
{
  startline(true);
  if (unixflag) {
    op = altstr(op);
  }
  casealtstr(op);
  tabdelim();
  argstep();
}

/*
** props() prints an opcode much like prop() does, but it also
** appends a data size letter.
*/

static void props(char* op)
{
  startline(true);
  if (unixflag) {
    casealtstr(altstr(op));
  } else {
    casealtstr(op);
    bufchar('.');
  }
  switch (opsize) {
    case ops_byte: casechar('b'); break;
    case ops_word: casechar('w'); break;
    case ops_long: casechar('l'); break;
  }
  tabdelim();
  argstep();
}

static void pcomma(void)
{
  argdelim(", ");
}

static void piconst(longword l)
{
  lastimmdata = l;
  if (unixflag) {
    bufchar('&');
  } else {
    bufchar('#');
  }
  number(l);
}

static void pqconst(int c)
{
  if (c == 0) {
    c = 8;
  }
  piconst(c);
}

static void prareg(int reg)
{
  if (unixflag) {
    bufchar('%');
  }
  if (reg < 0) {
    switch (reg) {
    case reg_pc:
      casestring("pc");
      break;
    case reg_sr:
      casestring("sr");
      break;
    case reg_ccr:
      if (unixflag) {
	casestring("cc");
      } else {
	casestring("ccr");
      }
      break;
    case reg_usp:
      casestring("usp");
      break;
    }
  } else if (reg == 7) {
    casestring("sp");
  } else {
    casechar('a');
    bufoctal(reg, 1);
  }
}

static void prdreg(int reg)
{
  if (unixflag) {
    bufchar('%');
  }
  casechar('d');
  bufoctal(reg, 1);
}

static void prreg(int reg)
{
  if (reg > 7) {
    prareg(reg - 8);
  } else {
    prdreg(reg);
  }
}

static void prireg(int reg)
{
  bufchar('(');
  prareg(reg);
  bufchar(')');
}

/**********************************************************************
**
** Instruction handling, subroutines:
*/

static bool bit(word w, int b)
{
  if (((w >> b) & 1) == 1) {
    return(true);
  } else {
    return(false);
  }
}

static void bcdregs(void)
{
  if (bit(opcode, 3)) {
    bufchar('-');
    prireg(opreg);
    pcomma();
    bufchar('-');
    prireg(opreg2);
  } else {
    prdreg(opreg);
    pcomma();
    prdreg(opreg2);
  }
}  

static void brarg(long disp)
{
  address* target;

  target = a_offset(istart, disp);
  reference(target);
  if (l_exist(target)) {
    bufstring(l_find(target));
  } else {
    if (updateflag) {
      genlabel(target);
    }
    if (unixflag) {
      bufchar('.');
    } else {
      bufchar('*');
    }
    if (disp > 0) {
      bufchar('+'); number(disp);
    }
    if (disp < 0) {
      bufchar('-'); number(-disp);
    }
  }
  endref();
  pb_detour = target;
}

static void prmask(word mask)
{
  int pos;
  bool virgin;

  virgin = true;
  for (pos = 0; pos < 16; pos += 1) {
    if (bit(mask, pos)) {
      if (!virgin) {
	bufchar('/');
      }
      if (opmode == 4) {	/* Predecrement? */
	prreg(15 - pos);
      } else {
	prreg(pos);
      }
      virgin = false;
    }
  }
}

static char* cregname(int reg)
{
  switch (reg) {

/* 010/020/030/040/CPU32 */

  case 0x000: return("sfc");	/* Source Function Code */
  case 0x001: return("dfc");	/* Destination Function Code */
  case 0x800: return("usp");	/* User Stack Pointer */
  case 0x801: return("vbr");	/* Vector Base Register */

/* 020/030/040 */

  case 0x002: return("cacr");	/* Cache Control Register */
  case 0x802: return("caar");	/* Cache Address Register (020/030 only) */
  case 0x803: return("msp");	/* Master Stack Pointer */
  case 0x804: return("isp");	/* Interrupt Stack Pointer */

/* 040/LC040 */

  case 0x003: return("tc");	/* MMU Translation Control Register */
  case 0x004: return("itt0");	/* Instruction Transparent Translation Reg 0 */
  case 0x005: return("itt1");	/* Instruction Transparent Translation Reg 1 */
  case 0x006: return("dtt0");	/* Data Transparent Translation Register 0 */
  case 0x007: return("dtt1");	/* Data Transparent Translation Register 1 */
  case 0x805: return("mmusr");	/* MMU Status Register */
  case 0x806: return("urp");	/* User Root Pointer */
  case 0x807: return("srp");	/* Supervisor Root Pointer */

/* EC040 */

#ifdef FOOBAR
  case 0x004: return("iacr0");	/* Instruction Access Control Register 0 */
  case 0x005: return("iacr1");	/* Instruction Access Control Register 1 */
  case 0x006: return("dacr0");	/* Data Access Control Register 0 */
  case 0x007: return("dacr1");	/* Data Access Control Register 1 */
#endif
  }
  return(nil);
}

static int eal7(int reg)
{
  switch (reg) {
    case 0: return(2);
    case 1: return(4);
    case 2: return(2);
    case 3: return(2);		/* Fix for 68020/30. */
    case 4: if (opsize == ops_long) {
	      return(4);
	    } else {
	      return(2);
	    }
    case 5:
    case 6:
    case 7: goodinstruction = false;
  }
  return(0);			/* Default. */
}

static int eal(int mode, int reg)
{
  switch (mode) {
    case 5: return(2);
    case 6: return(2);		/* Fix for 68020/30. */
    case 7: return(eal7(reg));
  }
  return(0);			/* Default. */
}

static bool chk1ea(void)
{
  goodinstruction = true;
  pb_length += eal(opmode, opreg);
  if ((pb_prefer == st_none) && (pb_actual == st_none)) {
    if (getstatus(a_offset(istart, pb_length - 1)) != st_none) {
      goodinstruction = false;
    }
  }
  return(goodinstruction);
}

static bool chk2ea(void)
{
  goodinstruction = true;
  pb_length += eal(opmode, opreg);
  pb_length += eal(opmod2, opreg2);
  if ((pb_prefer == st_none) && (pb_actual == st_none)) {
    if (getstatus(a_offset(istart, pb_length - 1)) != st_none) {
      goodinstruction = false;
    }
  }
  return(goodinstruction);
}

static bool ealabel(longword l)
{
  int offset;
  address* addr;

  addr = a_l2a(l);
  touch = addr;
  if (l_exist(addr)) {
    bufstring(l_find(addr));
    return(true);
  }
  if (getstatus(addr) == st_cont) {
    for (offset = 1; offset < 7; offset += 1) {
      addr = a_l2a(l - offset);
      if (getstatus(addr) != st_cont) {
	if (l_exist(addr)) {
	  bufstring(l_find(addr));
	  bufchar('+');
	  number(offset);
	  return(true);
	} else {
	  return(false);
	}
      }
    }
  }
  return(false);
}

static void eaddr(longword l)
{
  if (!ealabel(l)) {
    touch = a_l2a(l);
    if (updateflag) {
      genlabel(touch);
    }
    number(l);
  }
}

static void prea5(int reg)
{
  long disp;
  longword l;

  disp = sextw(getword());
  if (reg == reg_pc) {
    l = a_a2l(istart) + 2 + disp;
    if (ealabel(l)) {
      return;
    }
    touch = a_l2a(l);
    if (updateflag) {
      genlabel(touch);
    }
  }
  signum(disp);
  prireg(reg);
}

static void prea6(int reg)
{
  word w;
  w = getword();
  signum(sextb(w & 0xff));
  bufchar('(');
  prareg(reg);
  pcomma();
  prreg((w >> 12) & 0x0f);
  if (bit(w, 11)) {
    casestring(".l");		/* UNIX */
  } else {
    casestring(".w");		/* UNIX */
  }
  w = (w >> 9) & 3;
  if (w == 1) bufstring("*2");	/* UNIX */
  if (w == 2) bufstring("*4");	/* UNIX */
  if (w == 3) bufstring("*8");	/* UNIX */
  bufchar(')');
}

static void prea7(int mode)
{
  switch (mode) {
  case 0:
    eaddr(sextw(getword()));
    break;
  case 1:
    eaddr(sextl(getlong()));
    break;
  case 2:
    prea5(reg_pc);
    break;
  case 3:
    prea6(reg_pc);
    break;
  case 4:
    if (opsize == ops_long) {
      piconst(getlong());
    } else {
      piconst(getword());
    }
    break;
  }
}

static void prea(void)
{
  touch = nil;
  switch (opmode) {
    case 0: prdreg(opreg); break;
    case 1: prareg(opreg); break;
    case 2: prireg(opreg); break;
    case 3: prireg(opreg); bufchar('+'); break;
    case 4: bufchar('-'); prireg(opreg); break;
    case 5: prea5(opreg); break;
    case 6: prea6(opreg); break;
    case 7: prea7(opreg); break;
  }
}

static void preau(void)
{
  prea();
  if (opsize == ops_byte) suggest(touch, st_byte, 1);
  if (opsize == ops_word) suggest(touch, st_word, 2);
  if (opsize == ops_long) suggest(touch, st_long, 4);
}

/**********************************************************************
**
** Instruction handlers:
*/

/* pmldvw(): multiply/divide word. */

static bool pmldvw(char* name)
{
  opsize = ops_word;
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  preau();
  pcomma();
  prdreg(opreg2);
  return(true);
}

/* pname(): one-word fixed instructions with no args. */

static bool pname(char* name)
{
  prop(name);
  return(true);
}

/* poneb(): instructions with one effective addr, byte size. */

static bool poneb(char* name)
{
  opsize = ops_byte;
  if (!chk1ea()) {
    return(false);
  }
  prop(name);
  preau();
  return(true);
}

/* ponew(): instructions with one effective addr, word size. */

static bool ponew(char* name)
{
  opsize = ops_word;
  if (!chk1ea()) {
    return(false);
  }
  prop(name);
  preau();
  return(true);
}

/* pones(): sized instructions with one ea. */

static bool pones(char* name)
{
  if (opsize == ops_none) {
    return(false);
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  preau();
  return(true);
}

/* pshift(): shifts to data regs. */

static bool pshift(char* name)
{
  if (opsize == ops_none) {
    return(false);
  }
  props(name);
  if (bit(opcode, 5)) {
    prdreg(opreg2);
  } else {
    pqconst(opreg2);
  }
  pcomma();
  prdreg(opreg);
  return(true);
}

/* parith(): arithetic to/from data regs. */

static bool parith(char* name)
{
  if (opsize == ops_none) {
    return(false);
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  if (bit(opcode, 8)) {
    prdreg(opreg2);
    pcomma();
    preau();
  } else {
    preau();
    pcomma();
    prdreg(opreg2);
  }
  return(true);
}

/* paddra(): add/subtract address. */

static bool paddra(char* name)
{
  if (bit(opcode, 8)) {
    opsize = ops_long;
  } else {
    opsize = ops_word;
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  prea();
  pcomma();
  prareg(opreg2);
  return(true);
}

/* pbit(): bit set/clear/test etc. */

static bool pbit(char* name)
{
  word idata;

  if (bit(opcode, 8)) {
    if (!chk1ea()) {
      return(false);
    }
    prop(name);
    prdreg(opreg2);
  } else {
    pb_length += 2;
    idata = getword();
    if (!chk1ea()) {
      return(false);
    }
    prop(name);
    piconst(idata);
  }
  pcomma();
  opsize = ops_byte;
  preau();
  return(true);
}

/* piccsr(): andi etc. to ccr/sr. */

static bool piccsr(char* name)
{
  pb_length += 2;
  props(name);
  piconst(getword());
  pcomma();
  if (bit(opcode, 6)) {
    prareg(reg_sr);
  } else {
    prareg(reg_ccr);
  }
  return(true);
}

/* pmccsr(): move to/from ccr/sr */

static bool pmccsr(char* name)
{
  if (!chk1ea()) {
    return(false);
  }
  prop(name);
  if (bit(opcode, 10)) {
    prea();
    pcomma();
  }
  if ((opreg2 == 0) || (opreg2 == 3)) {
    prareg(reg_sr);
  } else {
    prareg(reg_ccr);
  }
  if (!bit(opcode, 10)) {
    pcomma();
    prea();
  }
  return(true);
}

/* pimm(): immediate arith/compare etc. */

static bool pimm(char* name)
{
  longword idata;

  if (opsize == ops_none) {
    return(false);
  }
  if (opsize == ops_long) {
    idata = getlong();
    pb_length += 4;
  } else {
    idata = getword();
    pb_length += 2;
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  piconst(idata);
  pcomma();
  preau();
  return(true);
}

/* pqadd(): addq/subq. */

static bool pqadd(char* name)
{
  if (opsize == ops_none) {
    return(false);
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  pqconst(opreg2);
  pcomma();
  preau();
  return(true);
}

/* pret() handles single-word return instructions. */

static bool pret(char* name)
{
  prop(name);
  delayblank = true;
  pb_deadend = true;
  return(true);
}

static bool Bcc(char* name)
{
  int d8;
  long disp;
  
  d8 = opcode & 0xff;		/* Extract short displacement. */

  disp = sextb(d8);

  if (d8 == 0) {
    pb_length += 2;
    disp = sextw(getword());
  }
  if (d8 == 0xff) {
    pb_length += 4;
    disp = sextl(getlong());
  }
  if ((pb_prefer == st_none) && (pb_actual == st_none)) {
    if (getstatus(a_offset(istart, pb_length - 1)) != st_none) {
      return(false);
    }
  }
  prop(name);
  brarg(disp + 2);
  return(true);
}

static bool BRA(char* name)
{
  if(Bcc(name)) {
    pb_deadend = true;
    delayblank = true;
    return(true);
  } else {
    return(false);
  }
}
  
static bool DBcc(char* name)
{
  long disp;
  
  pb_length += 2;
  disp = sextw(getword());

  prop(name);
  prdreg(opreg);
  pcomma();
  brarg(disp + 2);
  return(true);
}

static bool DBT(char* name)
{
  DBcc(name);
  pb_detour = nil;
  return(true);
}

static bool ABCD(char* name)
{
  prop(name);
  bcdregs();
  return(true);
}

static bool ADDX(char* name)
{
  if (opsize == ops_none) return(false);
  props(name);
  bcdregs();
  return(true);
}

static bool CALLM(char* name)
{
  word w;

  if (cputype != 2) {
    return(false);
  }
  w = getword();
  pb_length += 2;
  if (!chk1ea()) {
    return(false);
  }
  prop(name);
  piconst(w);
  pcomma();
  prea();
  return(true);
}

static bool CHK(char* name)
{
  if (opmode == 1) {
    return(false);
  }
  if (bit(opcode, 7)) {
    opsize = ops_word;
  } else {
    opsize = ops_long;
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  preau();
  pcomma();
  prdreg(opreg2);
  return(true);
}

static bool CMP(char* name)
{
  if (opsize == ops_none) {
    return(false);
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  preau();
  pcomma();
  prdreg(opreg2);
  return(true);
}

static bool CMPA(char* name)
{
  if (bit(opcode, 8)) {
    opsize = ops_long;
  } else {
    opsize = ops_word;
  }
  if (!chk1ea()) {
    return(false);
  }
  props(name);
  prea();
  pcomma();
  prareg(opreg2);
  return(true);
}

static bool CMPM(char* name)
{
  if (opsize == ops_none) return(false);
  props(name);
  prireg(opreg);
  bufchar('+');
  pcomma();
  prireg(opreg2);
  bufchar('+');
  return(true);
}

static bool EXG(char* name)
{
  prop(name);
  if ((opcode & 0770) == 0510) {
    prareg(opreg2);
  } else {
    prdreg(opreg2);
  }
  pcomma();
  if ((opcode & 010) == 010) {
    prareg(opreg);
  } else {
    prdreg(opreg);
  }
  return(true);
}

static bool EXT(char* name)
{
  if (bit(opcode, 6)) {
    opsize = ops_long;
  } else {
    opsize = ops_word;
  }
  props(name);
  prdreg(opreg);
  return(true);
}

static bool JMP(char* name)
{
  if (!chk1ea()) {
    return(false);
  }
  prop(name);
  prea();

  if (updateflag) {
    if ((opmode == 2) && (opreg == moveareg) &&
	(peekserial == (moveaserial + 1)) &&
	a_eq(moveanext, istart)) {
      touch = moveatarget;
    }
    if (touch != nil) {
      genlabel(touch);
    }
  }

  pb_detour = touch;

  if (bit(opcode, 6)) {
    pb_deadend = true;
    delayblank = true;
  }

  return(true);
}

static bool LEA(char* name)
{
  if (!chk1ea()) {
    return(false);
  }
  prop(name);
  prea();
  pcomma();
  prareg(opreg2);
  if (opmode == 7) {
    if (opreg <= 1) {
      moveaserial = peekserial;
      moveatarget = a_copy(touch, moveatarget);
      moveanext = a_copy(pc, moveanext);
      moveareg = opreg2;
    }
  }
  return(true);
}

static bool LINKL(char* name)
{
  pb_length += 4;
  opsize = ops_long;
  props(name);
  prareg(opreg);
  pcomma();
  signum(sextl(getlong()));
  return(true);
}

static bool LINKW(char* name)
{
  pb_length += 2;
  opsize = ops_word;
  props(name);
  prareg(opreg);
  pcomma();
  signum(sextw(getword()));
  return(true);
}

static bool MOVE(char* name)
{
  switch (opcode >> 12) {
    case 1: opsize = ops_byte; break;
    case 2: opsize = ops_long; break;
    case 3: opsize = ops_word; break;
  }

  if (!chk2ea()) {
    return(false);
  }

  props(name);
  preau();
  pcomma();
  opmode = opmod2;
  opreg = opreg2;
  preau();
  return(true);
}

static bool MOVEA(char* name)
{
  switch (opcode >> 12) {
    case 2: opsize = ops_long; break;
    case 3: opsize = ops_word; break;
  }

  if (!chk1ea()) {
    return(false);
  }

  props(name);
  preau();
  pcomma();
  prareg(opreg2);
  if (opmode == 7) {
    if (opreg <= 1) {
      moveaserial = peekserial;
      moveatarget = a_copy(touch, moveatarget);
      moveanext = a_copy(pc, moveanext);
      moveareg = opreg2;
    }
    if (opreg == 4) {
      moveaserial = peekserial;
      moveatarget = a_copy(a_l2a(lastimmdata), moveatarget);
      moveanext = a_copy(pc, moveanext);
      moveareg = opreg2;
    }
  }
  return(true);
}

static bool MOVEC(char* name)
{
  int arg;
  char* regname;

  if (cputype < 1) {
    return(false);
  }
  arg = getword();
  pb_length += 2;

  regname = cregname(arg & 0x0fff);
  if (regname == nil) {
    return(false);
  }

  prop(name);
  if (bit(opcode, 0)) {
    prreg((arg >> 12) & 0x0fff);
    pcomma();
    bufstring(regname);
  } else {
    bufstring(regname);
    pcomma();
    prreg((arg >> 12) & 0x0fff);
  }
  return(true);
}

static bool MOVEM(char* name)
{
  word mask;

  mask = getword();
  pb_length += 2;
  if (!chk1ea()) {
    return(false);
  }
  if ((opcode & 0100) == 0100) {
    opsize = ops_long;
  } else {
    opsize = ops_word;
  }
  props(name);
  if ((opcode & 02000) == 02000) {
    prea();
    pcomma();
    prmask(mask);
  } else {
    prmask(mask);
    pcomma();
    prea();
  }
  return(true);
}

static bool MOVEP(char* name)
{
  pb_length += 2;
  if (bit(opcode, 6)) {
    opsize = ops_long;
  } else {
    opsize = ops_word;
  }
  prop(name);
  if (bit(opcode, 7)) {
    prdreg(opreg2);
    pcomma();
  }
  signum(sextw(getword()));
  prireg(opreg);
  if (!bit(opcode, 7)) {
    pcomma();
    prdreg(opreg2);
  }
  return(true);
}

static bool MOVEQ(char* name)
{
  prop(name);
  piconst(opcode & 0xff);
  pcomma();
  prdreg(opreg2);
  return(true);
}

static bool MOVEU(char* name)
{
  prop(name);
  if (bit(opcode, 3)) {
    prareg(reg_usp);
    pcomma();
  }
  prareg(opreg);
  if (!bit(opcode, 3)) {
    pcomma();
    prareg(reg_usp);
  }
  return(true);
}

static bool PEA(char* name)
{
  if (!chk1ea()) {
    return(false);
  }
  prop(name);
  prea();
  return(true);
}

static bool RTM(char* name)
{
  if (cputype != 2) {
    return(false);
  }
  prop(name);
  prreg(opcode & 0x000f);
  return(true);
}

static bool STOP(char* name)
{
  pb_length += 2;
  prop(name);
  piconst(getword());
  return(true);
}

static bool SWAP(char* name)
{
  prop(name);
  prdreg(opreg);
  return(true);
}

static bool TRAP(char* name)
{
  prop(name);
  piconst(opcode & 0x000f);
  return(true);
}

static bool TRAPcc(char* name)
{
  longword data;

  if (cputype < 2) return(false);

  if (bit(opcode, 2)) {
    prop(name);
  } else {
    if ((opcode & 3) == 3) {
      opsize = ops_long;
      pb_length += 4;
      data = getlong();
    } else {
      opsize = ops_word;
      pb_length += 2;
      data = getword();
    }
    props(name);
    piconst(data);
  }
  return(true);
}

static bool UNLK(char* name)
{
  prop(name);
  prareg(opreg);
  return(true);
}

/***********************************************************************/

/*
** start of the moby tables we need:
*/

typedef bool (handler)(char*);

struct disp {
  word instr;
  word mask;
  handler* work;
  char* name;
};

struct disp itab00[] = {
  { 0000074, 0177777, piccsr, "ori;or" },
  { 0000174, 0177777, piccsr, "ori;or" },
  { 0001074, 0177777, piccsr, "andi;and" },
  { 0001174, 0177777, piccsr, "andi;and" },
  { 0005074, 0177777, piccsr, "eori;eor" },
  { 0005174, 0177777, piccsr, "eori;eor" },
  { 0000000, 0177400, pimm,   "ori;or" },
  { 0001000, 0177400, pimm,   "andi;and" },
  { 0002000, 0177400, pimm,   "subi;sub" },
  { 0003000, 0177400, pimm,   "addi;add" },
  { 0005000, 0177400, pimm,   "eori;eor" },
  { 0006000, 0177400, pimm,   "cmpi;cmp" },
  { 0004000, 0177700, pbit,   "btst" },
  { 0004100, 0177700, pbit,   "bchg" },
  { 0004200, 0177700, pbit,   "bclr" },
  { 0004300, 0177700, pbit,   "bset" },
  { 0000400, 0170700, pbit,   "btst" },
  { 0000500, 0170700, pbit,   "bchg" },
  { 0000600, 0170700, pbit,   "bclr" },
  { 0000700, 0170700, pbit,   "bset" },
  { 0003300, 0177760, RTM,    "rtm" },
  { 0003300, 0177700, CALLM,  "callm" },
  { 0000410, 0170470, MOVEP,  "movep" },
/*
  0000 0xx 011 xxx xxx  CHK2		;68020...
  0000 0xx 011 xxx xxx  CMP2		;68020...
  0000 111 0xx xxx xxx  MOVES		;68010...
  0000 1xx 011 111 100  CAS2		;68020...
  0000 1xx 011 xxx xxx  CAS		;68020...
 */
  { 0, 0, 0, nil },
};

struct disp itab01[] = {
  { 0010000, 0170000, MOVE,   "move;mov" },
  { 0, 0, 0, nil },
};

struct disp itab02[] = {
  { 0020100, 0170700, MOVEA,  "movea;mov" },
  { 0020000, 0170000, MOVE,   "move;mov" },
  { 0, 0, 0, nil },
};

struct disp itab03[] = {
  { 0030100, 0170700, MOVEA,  "movea;mov" },
  { 0030000, 0170000, MOVE,   "move;mov" },
  { 0, 0, 0, nil },
};

struct disp itab04[] = {
  { 0040000, 0177400, pones,  "negx" },
  { 0040300, 0174700, pmccsr, "move;mov" },
  { 0040700, 0170700, LEA,    "lea" },
  { 0041000, 0177400, pones,  "clr" },
  { 0042000, 0177400, pones,  "neg" },
  { 0043000, 0177400, pones,  "not" },
  { 0044010, 0177770, LINKL,  "link" },
  { 0044100, 0177700, PEA,    "pea" },
  { 0044200, 0177670, EXT,    "ext" },
  { 0044200, 0175600, MOVEM,  "movem" },
  { 0044700, 0177770, EXT,    "extb" },
  { 0045000, 0177400, pones,  "tst" },
  { 0047120, 0177770, LINKW,  "link" },
  { 0047140, 0177760, MOVEU,  "move;mov" },
  { 0047200, 0177700, JMP,    "jsr" },
  { 0047300, 0177700, JMP,    "jmp" },
  { 0045372, 0177777, pname,  "bgnd" },	/* CPU32 only instruction. */
  { 0045374, 0177777, pname,  "illegal" },
  { 0045300, 0177700, poneb,  "tas" },
  { 0044000, 0177700, poneb,  "nbcd" },
  { 0040400, 0170500, CHK,    "chk" },
  { 0044100, 0177770, SWAP,   "swap" },
  { 0047130, 0177770, UNLK,   "unlk" },
  { 0047100, 0177760, TRAP,   "trap" },
  { 0047160, 0177777, pname,  "reset" },
  { 0047161, 0177777, pname,  "nop" },
  { 0047162, 0177777, STOP,   "stop" },
  { 0047163, 0177777, pret,   "rte" },
  { 0047164, 0177777, pret,   "rtd"},
  { 0047165, 0177777, pret,   "rts" },
  { 0047166, 0177777, pname,  "trapv" },
  { 0047167, 0177777, pret,   "rtr" },
  { 0047172, 0177776, MOVEC,  "movec" },
  { 0, 0, 0, nil },
/*
  0100 100 001 001 xxx  BKPT		;68020...
  0100 110 000 xxx xxx  MULS.L		;68020...
  0100 110 000 xxx xxx  MULU.L		;68020...
  0100 110 001 xxx xxx  DIVS.L		;68020...
  0100 110 001 xxx xxx  DIVU.L		;68020...
 */
};

struct disp itab05[] = {
  { 0050000, 0170400, pqadd,  "addq;add" },
  { 0050400, 0170400, pqadd,  "subq;sub" },

  { 0050310, 0177770, DBT,    "dbt" },
  { 0050710, 0177770, DBcc,   "dbra" },
  { 0051310, 0177770, DBcc,   "dbhi" },
  { 0051710, 0177770, DBcc,   "dbls" },
  { 0052310, 0177770, DBcc,   "dbcc" },
  { 0052710, 0177770, DBcc,   "dbcs" },
  { 0053310, 0177770, DBcc,   "dbne" },
  { 0053710, 0177770, DBcc,   "dbeq" },
  { 0054310, 0177770, DBcc,   "dbvc" },
  { 0054710, 0177770, DBcc,   "dbvs" },
  { 0055310, 0177770, DBcc,   "dbpl" },
  { 0055710, 0177770, DBcc,   "dbmi" },
  { 0056310, 0177770, DBcc,   "dbge" },
  { 0056710, 0177770, DBcc,   "dblt" },
  { 0057310, 0177770, DBcc,   "dbgt" },
  { 0057710, 0177770, DBcc,   "dble" },

  { 0050370, 0177770, TRAPcc, "trapt" },
  { 0050770, 0177770, TRAPcc, "trapf" },
  { 0051370, 0177770, TRAPcc, "traphi" },
  { 0051770, 0177770, TRAPcc, "trapls" },
  { 0052370, 0177770, TRAPcc, "trapcc" },
  { 0052770, 0177770, TRAPcc, "trapcs" },
  { 0053370, 0177770, TRAPcc, "trapne" },
  { 0053770, 0177770, TRAPcc, "trapeq" },
  { 0054370, 0177770, TRAPcc, "trapvc" },
  { 0054770, 0177770, TRAPcc, "trapvs" },
  { 0055370, 0177770, TRAPcc, "trappl" },
  { 0055770, 0177770, TRAPcc, "trapmi" },
  { 0056370, 0177770, TRAPcc, "trapge" },
  { 0056770, 0177770, TRAPcc, "traplt" },
  { 0057370, 0177770, TRAPcc, "trapgt" },
  { 0057770, 0177770, TRAPcc, "traple" },

  { 0050300, 0177700, poneb,  "st" },
  { 0050700, 0177700, poneb,  "sf" },
  { 0051300, 0177700, poneb,  "shi" },
  { 0051700, 0177700, poneb,  "sls" },
  { 0052300, 0177700, poneb,  "scc" },
  { 0052700, 0177700, poneb,  "scs" },
  { 0053300, 0177700, poneb,  "sne" },
  { 0053700, 0177700, poneb,  "seq" },
  { 0054300, 0177700, poneb,  "svc" },
  { 0054700, 0177700, poneb,  "svs" },
  { 0055300, 0177700, poneb,  "spl" },
  { 0055700, 0177700, poneb,  "smi" },
  { 0056300, 0177700, poneb,  "sge" },
  { 0056700, 0177700, poneb,  "slt" },
  { 0057300, 0177700, poneb,  "sgt" },
  { 0057700, 0177700, poneb,  "sle" },
  { 0, 0, 0, nil },
};

struct disp itab06[] = {
  { 0060000, 0177400, BRA,    "bra" },
  { 0060400, 0177400, Bcc,    "bsr" },
  { 0061000, 0177400, Bcc,    "bhi" },
  { 0061400, 0177400, Bcc,    "bls" },
  { 0062000, 0177400, Bcc,    "bcc" },
  { 0062400, 0177400, Bcc,    "bcs" },
  { 0063000, 0177400, Bcc,    "bne" },
  { 0063400, 0177400, Bcc,    "beq" },
  { 0064000, 0177400, Bcc,    "bvc" },
  { 0064400, 0177400, Bcc,    "bvs" },
  { 0065000, 0177400, Bcc,    "bpl" },
  { 0065400, 0177400, Bcc,    "bmi" },
  { 0066000, 0177400, Bcc,    "bge" },
  { 0066400, 0177400, Bcc,    "blt" },
  { 0067000, 0177400, Bcc,    "bgt" },
  { 0067400, 0177400, Bcc,    "ble" },
  { 0, 0, 0, nil },
};

struct disp itab07[] = {
  { 0070000, 0170400, MOVEQ,  "moveq;mov" },
  { 0, 0, 0, nil },
};

struct disp itab10[] = {
  { 0100400, 0170760, ABCD,   "abcd" },
  { 0100000, 0170000, parith, "or" },
  { 0100300, 0170700, pmldvw, "divu" },
  { 0100700, 0170700, pmldvw, "divs" },
/*
  1000 xxx 101 00x xxx  PACK		;68020...
  1000 xxx 110 00x xxx  UNPK		;68020...
 */
  { 0, 0, 0, nil },
};

struct disp itab11[] = {
  { 0110400, 0170460, ADDX,   "subx" },
  { 0110000, 0170000, parith, "sub" },
  { 0110300, 0170300, paddra, "suba;sub" },
  { 0, 0, 0, nil },
};

struct disp itab12[] = {
  { 0, 0, 0, nil },
};

struct disp itab13[] = {
  { 0130000, 0170400, CMP,    "cmp" },
  { 0130300, 0170300, CMPA,   "cmpa;cmp" },
  { 0130410, 0170470, CMPM,   "cmpm" },
  { 0130400, 0170400, parith, "eor" },
  { 0, 0, 0, nil },
};

struct disp itab14[] = {
  { 0140500, 0170760, EXG,    "exg" },
  { 0140610, 0170770, EXG,    "exg" },
  { 0140400, 0170760, ABCD,   "sbcd" },
  { 0140000, 0170000, parith, "and" },
  { 0140300, 0170700, pmldvw, "mulu" },
  { 0140700, 0170700, pmldvw, "muls" },
  { 0, 0, 0, nil },
};

struct disp itab15[] = {
  { 0150400, 0170460, ADDX,   "addx" },
  { 0150000, 0170000, parith, "add" },
  { 0150300, 0170300, paddra, "adda;add" },
  { 0, 0, 0, nil },
};

struct disp itab16[] = {
  { 0160300, 0177700, ponew,  "asr" },
  { 0160700, 0177700, ponew,  "asl" },
  { 0161300, 0177700, ponew,  "lsr" },
  { 0161700, 0177700, ponew,  "lsl" },
  { 0162300, 0177700, ponew,  "roxr" },
  { 0162700, 0177700, ponew,  "roxl" },
  { 0163300, 0177700, ponew,  "ror" },
  { 0163700, 0177700, ponew,  "rol" },
  { 0160000, 0170430, pshift, "asr" },
  { 0160010, 0170430, pshift, "lsr" },
  { 0160020, 0170430, pshift, "roxr" },
  { 0160030, 0170430, pshift, "ror" },
  { 0160400, 0170430, pshift, "asl" },
  { 0160410, 0170430, pshift, "lsl" },
  { 0160420, 0170430, pshift, "roxl" },
  { 0160430, 0170430, pshift, "rol" },
/*
  1110 100 011 xxx xxx  BFTST		;68020...
  1110 100 111 xxx xxx  BFEXTU		;68020...
  1110 101 011 xxx xxx  BFCHG		;68020...
  1110 101 111 xxx xxx  BFEXTS		;68020...
  1110 110 011 xxx xxx  BFCLR		;68020...
  1110 110 111 xxx xxx  BFFFO		;68020...
  1110 111 011 xxx xxx  BFSET		;68020...
  1110 111 111 xxx xxx  BFINS		;68020...
 */
  { 0, 0, 0, nil },
};

struct disp itab17[] = {
  { 0, 0, 0, nil },
};

static struct disp* itable[16] = {
  itab00, itab01, itab02, itab03, itab04, itab05, itab06, itab07,
  itab10, itab11, itab12, itab13, itab14, itab15, itab16, itab17,
};

/**********************************************************************/

static void dobyte(byte b)
{
  pb_status = st_byte;
  pb_length = 1;
  prop("dc.b;byte");
  number(b);
  if (pb_actual == st_byte) {
    while (getstatus(pc) == st_cont) {
      pcomma();
      number(getbyte());
      pb_length += 1;
    }
  }
  checkblank();
}

static void dochar(byte b)
{
  if (printable(b)) {
    pb_status = st_char;	/* ??? */
    pb_length = 1;
    prop("dc.b;byte");
    bufchar('\'');
    bufchar((char) b);
    bufchar('\'');
    checkblank();
  } else {
    dobyte(b);
  }
}

static void doword(word w)
{
  pb_status = st_word;
  pb_length = 2;
  prop("dc.w;word");
  number(w);
  if (pb_actual == st_word) {
    while (getstatus(pc) == st_cont) {
      pcomma();
      number(getword());
      pb_length += 2;
    }
  }
  checkblank();
}

static void dolong(longword l)
{
  pb_status = st_long;
  pb_length = 4;
  prop("dc.l;long");
  number(l);
  if (pb_actual == st_long) {
    while (getstatus(pc) == st_cont) {
      pcomma();
      number(getlong());
      pb_length += 4;
    }
  }
  checkblank();
}

static void doptr(longword l)
{
  pb_status = st_ptr;
  pb_length = 4;
  prop("dc.l;long");
  if (l_exist(a_l2a(l))) {
    bufstring(l_find(a_l2a(l)));
  } else {
    number(l);
  }
  checkblank();
}

static void scantable(struct disp* table)
{
  int i;

  i = 0;
  while (table[i].mask != 0) {
    if ((opcode & table[i].mask) == table[i].instr) {
      if ((table[i].work)(table[i].name)) {
	pb_status = st_inst;
	return;
      }
    }
    i += 1;
  }
  doword(opcode);
}

static void doinstr(void)
{
  if ((a_a2l(istart) & 1) == 1) {
    dobyte(getbyte());
  } else {
    opcode = getword();
    opreg = opcode & 7;
    opmode = (opcode >> 3) & 7;
    opsize = (opcode >> 6) & 3;
    opmod2 = (opcode >> 6) & 7;
    opreg2 = (opcode >> 9) & 7;

    pb_length = 2;

    scantable(itable[(opcode >> 12) & 0x0f]);
  }
}

static void dotext(void)
{
  int pos;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    dochar(getmemory(istart));
  } else {
    pb_status = st_text;
    prop("dc.b;ascii");
    bufchar('\'');
    for (pos = 0; pos < pb_length; pos += 1) {
      bufchar(line[pos]);
    }
    bufchar('\'');
    checkblank();
  }
}

static void checkunmap(address* a)
{
  if (!mapped(a) && l_exist(a)) {
    bufstring(l_find(a));
    tabspace(8);
    casestring("equ");
    tabdelim();
    hex(a_a2l(a));
    if (c_exist(a)) {
      tabto(32);
      bufstring(cstart());
      bufstring(c_find(a));
    }
    bufnewline();
  }
}

void m68k_spec(address* a, int func)
{
  if (func == SPC_BEGIN) {
    bufstring(";Beginning of program");
    bufblankline();
    foreach(checkunmap);
    bufblankline();
  }

  if (func == SPC_ORG) {
    bufblankline();
    casestring("org");
    tabdelim();
    hex(a_a2l(a));
    bufblankline();
  }

  if (func == SPC_END) {
    bufblankline();
    bufstring(";End of program");
  }

}

void m68k_peek(stcode prefer)
{
  if (d_exist(istart)) {
    bufblankline();
    startline(false);
    bufdescription(istart, cstart());
    bufblankline();
  }

  if ((prefer == st_none) && e_exist(istart)) {
    pb_length = e_length(istart);
    startline(true);
    bufstring(e_find(istart));
  } else {
    switch (pb_status) {
    case st_byte:
      dobyte(getbyte());
      break;
    case st_char:
      dochar(getbyte());
      break;
    case st_word:
      doword(getword());
      break;
    case st_long:
      dolong(getlong());
      break;
    case st_ptr:
      doptr(getlong());
      break;
    case st_text:
      dotext();
      break;
    case st_none:
      /* none means instruction if we can. */
    case st_inst:
      doinstr();
      break;
    default:
      dobyte(getbyte());
      break;
    }
  }

  stdcomment(32, cstart());

  restline();
}

/**********************************************************************/

bool m68k_cchk(byte b)
{
  if (b < 32) return(false);
  if (b >= 127) return(false);
  if (b == '\'') return(false);
  return(true);
}

char* m68k_lcan(char* name)
{
  static char work[10];

  if (unixflag) {
    return(name);
  }
  return(canonicalize(name, work, 8));
}

bool m68k_lchk(char* name)
{
  return(checkstring(name, ".", "0123456789_$-."));
}

void m68k_lgen(address* addr)
{
  genlabel(addr);
}

address* m68k_auto(void)
{
  return(nil);			/* For the time being. */
}

/**********************************************************************/

static void setpf(void)
{
  /* Set up our functions: */
  
  spf_peek(m68k_peek);
  spf_spec(m68k_spec);
  spf_cchk(m68k_cchk);
  spf_lcan(m68k_lcan);
  spf_lchk(m68k_lchk);
  spf_lgen(m68k_lgen);
  spf_auto(m68k_auto);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 6;			/* Bytes per line. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = true;		/* We are big-endian. */
}

void m68000_init(void)
{
  cputype = 0;
  setpf();
}

bool m68000_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Motorola 68000 processors.\n\
");
    return(true);
  }
  return(false);
}

void m68010_init(void)
{
  cputype = 1;
  setpf();
}

bool m68010_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Motorola 68010 processors.\n\
");
    return(true);
  }
  return(false);
}

void m68020_init(void)
{
  cputype = 2;
  setpf();
}

bool m68020_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Motorola 68020 processors.\n\
");
    return(true);
  }
  return(false);
}

void m68030_init(void)
{
  cputype = 3;
  setpf();
}

bool m68030_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Motorola 68030 processors.\n\
");
    return(true);
  }
  return(false);
}

void m68040_init(void)
{
  cputype = 4;
  setpf();
}

bool m68040_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Motorola 68040 processors.\n\
");
    return(true);
  }
  return(false);
}

void cpu32_init(void)
{
  cputype = 32;
  setpf();
}

bool cpu32_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Motorola CPU32 processors.\n\
");
    return(true);
  }
  return(false);
}
