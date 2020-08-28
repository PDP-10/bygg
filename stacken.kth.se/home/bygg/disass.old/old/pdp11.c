/*
** This module implements driver code for the DEC PDP-11 processor.
*/

#include "disass.h"

evf_init pdp11_init;
evf_help pdp11_help;

struct entryvector pdp11_vector = {
  "pdp11",			/* Name. */
  "DEC PDP11",			/* One-liner. */
  pdp11_init,			/* Init routine. */
  pdp11_help,			/* Help routine. */
};

/*
** Global variables, from module common:
*/

extern char statuschar;

extern bool delayblank;

/*
** Start of our local variables:
*/

static address* touch;		/* Address we read/write, if known. */

static word opcode;		/* Opcode we are decoding. */

static word dstreg, dstmod;	/* Destination mode/reg. */
static word srcreg, srcmod;	/* Source mode/reg. */

static int residue;		/* Number of bytes of "data" we did not */
				/* fit on the first line. */

/************************************************************************/

/*
** octal() prints a number in base 8.  Do I really have to tell you that?
*/

static void octal(longword l)
{
  bufoctal(l, 1);
}

/*
** number() prints out a number, according to the selected radix.  If the
** selected radix is something we don't support, use the default, i.e. octal.
*/

static void number(longword l)
{
  octal(l);
}

/*
** When we have decided upon the length of the current object, we call
** startline() to start the line with whatever is needed.
*/

/*
** Format of the "data" portion of the lines:
**
** "F aaaaaa: dddddd  ", length = 18 chars.
*/

static void startline(bool nonempty)
{
  byte lsb, msb;

  if (listformat) {
    if (nonempty) {
      bufchar(statuschar);
      bufchar(' ');
      bufoctal(a_a2w(istart), 6);
      bufstring(": ");
      if (pb_length == 1) {
	bufoctal(getmemory(istart), 6);
      } else {
	lsb = getmemory(istart);
	msb = getmemory(a_next(istart));
	bufoctal((msb << 8) + lsb, 6);
      }
      residue = pb_length - 2;
    }
    tabto(18);
    bufmark();
  }

  if (nonempty) {
    stdlabel();
  }
}

/*
** restline() will do whatever startline() cared not to do at his great
** moment in time.
*/

static void restline(void)
{
  if (residue > 0) {
    setpc(istart);		/* Restart from beginning. */
    (void) getword();		/* Skip first word. */
  }
  while (residue > 0) {
    delayblank = false;
    bufnewline();
    bufstring("  ");
    bufoctal(a_a2w(pc), 6);
    bufstring(": ");
    if (residue == 1) {
      bufoctal(getbyte(), 6);
    } else {
      bufoctal(getword(), 6);
    }
    residue -= 2;
  }
}

/*
** This routine will generate a label on the specified address, unless
** one already exist.  The generated label will have the format Lnnnnn
** or Unnnnn, with the letter U/L for the upper/lower 32K, and nnnnn =
** the lower 15 bits of the address, in octal.
*/

static void genlabel(address* addr)
{
  char work[10];
  word w;

  if (!l_exist(addr)) {
    w = a_a2w(addr);
    if (w > 077777) {
      sprintf(work, "U%05o", w & 077777);
    } else {
      sprintf(work, "L%05o", w);
    }
    l_insert(addr, work);
  }
}

/*
** printable() is a helper routine used when decoding texts and chars.
*/

static bool printable(byte b)
{
  if (b < 32) return(false);
  if (b >= 127) return(false);
  return(true);
}

/*
** dobyte() will output the current item as a byte of data.
*/

static void dobyte(byte b)
{
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  if (pb_actual == st_byte) {
    while (getstatus(pc) == st_cont) {
      bufstring(", ");
      number(getbyte());
      pb_length += 1;
    }
  }
  checkblank();
}

/*
** dochar() will try to output its argument as a character.  If it can't,
** it will default to dobyte().
*/

static void dochar(byte b)
{
  if (printable(b)) {
    pb_length = 1;
    startline(true);
    casestring(".byte");
    tabdelim();
    bufchar('\'');
    if (b == '\'') {
      bufchar((char) b);
    }
    bufchar((char) b);
    bufchar('\'');
    checkblank();
  } else {
    dobyte(b);
  }
}

/*
** doword() will output the current item as a word (16 bits) of data.
*/

static void doword(word w)
{
  pb_length = 2;
  startline(true);
  casestring(".word");
  tabdelim();
  number(w);
  if (pb_actual == st_word) {
    while (getstatus(pc) == st_cont) {
      bufstring(", ");
      number(getword());
      pb_length += 2;
    }
  }
  checkblank();
}

/*
** doptr() tries to output its argument as a pointer, i.e. if there is
** a label that corresponds to the argument, it will be used, otherwise
** we will setter for a numeric value.
*/

static void doptr(longword l)
{
  address* a;

  pb_length = 2;
  a = a_l2a(l);
  if (l_exist(a)) {
    startline(true);
    casestring(".word");
    tabdelim();
    bufstring(l_find(a));
    checkblank();
  } else {
    doword(l);
  }
}

/*
** dotext() will try to decode a text constant.
*/

static void dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    dochar(getmemory(istart));
  } else {
    startline(true);
    casestring(".ascii");
    tabdelim();
    bufchar('\'');
    for (pos = 0; pos < pb_length; pos += 1) {
      c = line[pos];
      if (c == '\'') {
	bufchar(c);
      }
      bufchar(c);
    }
    bufchar('\'');
  }
}

/* Start of instruction decoding routines: */

static void l1ea(void)
{
  if (dstmod >= 6) {
    pb_length += 2;
    return;
  }
  if ((dstreg == 7) && ((dstmod == 2) || (dstmod == 3))) {
    pb_length += 2;
  }
}

static void l2ea(void)
{
  l1ea();
  if (srcmod >= 6) {
    pb_length += 2;
  }
  if ((srcreg == 7) && ((srcmod == 2) || (srcmod == 3))) {
    pb_length += 2;
  }
}

static void prreg(int reg)
{
  static char* regname[8] = { "r0", "r1", "r2", "r3", "r4", "r5", "sp", "pc" };

  casestring(regname[reg & 7]);
}

static void pireg(int reg)
{
  bufchar('(');
  prreg(reg);
  bufchar(')');
}

static void pcomma(void)
{
  argdelim(", ");
}

static void piconst(word w)
{
  bufchar('#');
  number(w);
}

static void pea37(void)
{
  word w;

  bufchar('@');

  w = getword();
  touch = a_l2a(w);
  if (l_exist(touch)) {
    bufchar('#');
    bufstring(l_find(touch));
  } else {
    piconst(w);
  }
}

static void pea67(void)
{
  word w;

  w = getword();
  touch = a_l2a((w + a_a2w(pc)) & 0177777);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    number(w);
    pireg(7);
  }
}

static void pea77(void)
{
  bufchar('@');
  pea67();
  if (!c_exist(istart)) {
    tabto(32);
    bufchar(';');
    number(a_a2w(touch));
  }
  touch = nil;
}

static void peaddr(int mode, int reg)
{
  touch = nil;
  if (reg == 7) {
    switch (mode) {
      case 2: piconst(getword()); return;
      case 3: pea37(); return;
      case 6: pea67(); return;
      case 7: pea77(); return;
    }
  }
  switch (mode) {
    case 7: bufchar('@');
    case 6: number(getword()); pireg(reg); return;
    case 5: bufchar('@');
    case 4: bufchar('-'); pireg(reg); return;
    case 3: bufchar('@');
    case 2: pireg(reg); bufchar('+'); return;
    case 1: pireg(reg); return;
    case 0: prreg(reg); return;
  }
}
 
static void prsrc(stcode status)
{
  peaddr(srcmod, srcreg);
  if (touch != nil) {
    if (status == st_byte) suggest(touch, st_byte, 1);
    if (status == st_word) suggest(touch, st_word, 2);
  }
}

static void prdst(stcode status)
{
  peaddr(dstmod, dstreg);
  if (touch != nil) {
    if (status == st_byte) suggest(touch, st_byte, 1);
    if (status == st_word) suggest(touch, st_word, 2);
  }
}

static void prjmp(void)
{
  peaddr(dstmod, dstreg);
  if (touch != nil) {
    if (updateflag) {
      genlabel(touch);
    }
    pb_detour = touch;
  }
}

static void pbaddr(int offset)
{
  offset = (offset * 2) + 2;
  touch = a_offset(istart, offset);
  if (updateflag) {
    genlabel(touch);
  }
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    bufchar('.');
    if (offset > 0) {
      bufchar('+'); number(offset);
    }
    if (offset < 0) {
      bufchar('-'); number(-offset);
    }
  }
  pb_detour = touch;
}

/* instruction handlers: */

static void pname(char* name)
{
  startline(true);
  casestring(name);
}

static void pret(char* name)
{
  pname(name);
  pb_deadend = true;
  delayblank = true;
}

static void pone(char* name, stcode status)
{
  l1ea();
  pname(name);
  tabdelim();
  prdst(status);
}

static void ptwo(char* name, stcode status)
{
  l2ea();
  pname(name);
  tabdelim();
  prsrc(status);
  pcomma();
  prdst(status);
}

static void poneb(char* name)
{
  pone(name, st_byte);
}

static void ponew(char* name)
{
  pone(name, st_word);
}

static void ptwob(char* name)
{
  ptwo(name, st_byte);
}

static void ptwow(char* name)
{
  ptwo(name, st_word);
}

static void pcbra(char* name)
{
  pname(name);
  tabdelim();
  pbaddr(sextb(opcode & 0377));
}

static void pubra(char* name)
{
  pcbra(name);
  pb_deadend = true;
  delayblank = true;
}

static void preis(char* name)
{
  if ((srcreg & 1) == 1) {
    pone(name, st_word);
  } else {
    pone(name, st_long);
  }
  pcomma();
  prreg(srcreg);
}

static void prfis(char* name)
{
  pname(name);
  tabdelim();
  prreg(dstreg);
}

static void EMT(char* name)
{
  pname(name);
  tabdelim();
  number(opcode & 0377);
}

static void JMP(char* name)
{
  l1ea();
  pname(name);
  tabdelim();
  prjmp();
  pb_deadend = true;
  delayblank = true;
}

static void JSR(char* name)
{
  l1ea();
  pname(name);
  tabdelim();
  prreg(srcreg);
  pcomma();
  prjmp();
}

static void MARK(char* name)
{
  pname(name);
  tabdelim();
  number(opcode & 077);
}

static void RTS(char* name)
{
  pname(name);
  tabdelim();
  prreg(dstreg);
  pb_deadend = true;
}

static void SOB(char* name)
{
  pname(name);
  tabdelim();
  prreg(srcreg);
  pcomma();
  pbaddr(-(opcode & 077));
}

static void TRAP(char* name)
{
  pname(name);
  tabdelim();
  number(opcode & 0377);
}

static void XOR(char* name)
{
  l1ea();
  pname(name);
  tabdelim();
  prreg(srcreg);
  pcomma();
  prdst(st_word);
}

/*
** start of the moby tables we need:
*/

typedef void (handler)(char*);

struct disp {
  word instr;
  word mask;
  handler* work;
  char* name;
};

static struct disp itable[] = {
  { 0000000, 0177777, pname, "halt" },
  { 0000001, 0177777, pname, "wait" },
  { 0000002, 0177777, pret,  "rti" },
  { 0000003, 0177777, pname, "bpt" },
  { 0000004, 0177777, pname, "iot" },
  { 0000005, 0177777, pname, "reset" },
  { 0000006, 0177777, pret,  "rtt" },
  { 0000100, 0177700, JMP,   "jmp" },
  { 0000200, 0177770, RTS,   "rts" },
  { 0000240, 0177777, pname, "nop" },
  { 0000241, 0177777, pname, "clc" },
  { 0000242, 0177777, pname, "clv" },
  { 0000244, 0177777, pname, "clz" },
  { 0000250, 0177777, pname, "cln" },
  { 0000257, 0177777, pname, "ccc" },
  { 0000261, 0177777, pname, "sec" },
  { 0000262, 0177777, pname, "sev" },
  { 0000264, 0177777, pname, "sez" },
  { 0000270, 0177777, pname, "sen" },
  { 0000277, 0177777, pname, "scc" },
  { 0000300, 0177700, ponew, "swab" },
  { 0000400, 0177400, pubra, "br" },
  { 0001000, 0177400, pcbra, "bne" },
  { 0001400, 0177400, pcbra, "beq" },
  { 0002000, 0177400, pcbra, "bge" },
  { 0002400, 0177400, pcbra, "blt" },
  { 0003000, 0177400, pcbra, "bgt" },
  { 0003400, 0177400, pcbra, "ble" },
  { 0004000, 0177000, JSR,   "jsr" },
  { 0005000, 0177700, ponew, "clr" },
  { 0005100, 0177700, ponew, "com" },
  { 0005200, 0177700, ponew, "inc" },
  { 0005300, 0177700, ponew, "dec" },
  { 0005400, 0177700, ponew, "neg" },
  { 0005500, 0177700, ponew, "adc" },
  { 0005600, 0177700, ponew, "sbc" },
  { 0005700, 0177700, ponew, "tst" },
  { 0006000, 0177700, ponew, "ror" },
  { 0006100, 0177700, ponew, "rol" },
  { 0006200, 0177700, ponew, "asr" },
  { 0006300, 0177700, ponew, "asl" },
  { 0006400, 0177700, MARK,  "mark" },
  { 0006700, 0177700, ponew, "sxt" },
  { 0010000, 0170000, ptwow, "mov" },
  { 0020000, 0170000, ptwow, "cmp" },
  { 0030000, 0170000, ptwow, "bit" },
  { 0040000, 0170000, ptwow, "bic" },
  { 0050000, 0170000, ptwow, "bis" },
  { 0060000, 0170000, ptwow, "add" },
  { 0070000, 0177000, preis, "mul" },
  { 0071000, 0177000, preis, "div" },
  { 0072000, 0177000, preis, "ash" },
  { 0073000, 0177000, preis, "ashc" },
  { 0074000, 0177000, XOR,   "xor" },
  { 0075000, 0177770, prfis, "fadd" },
  { 0075010, 0177770, prfis, "fsub" },
  { 0075020, 0177770, prfis, "fmul" },
  { 0075030, 0177770, prfis, "fdiv" },
  { 0077000, 0177000, SOB,   "sob" },
  { 0100000, 0177400, pcbra, "bpl" },
  { 0100400, 0177400, pcbra, "bmi" },
  { 0101000, 0177400, pcbra, "bhi" },
  { 0101400, 0177400, pcbra, "blos" },
  { 0102000, 0177400, pcbra, "bvc" },
  { 0102400, 0177400, pcbra, "bvs" },
  { 0103000, 0177400, pcbra, "bcc" },
  { 0103400, 0177400, pcbra, "bcs" },
  { 0104000, 0177400, EMT,   "emt" },
  { 0104100, 0177400, TRAP,  "trap" },
  { 0105000, 0177700, poneb, "clrb" },
  { 0105100, 0177700, poneb, "comb" },
  { 0105200, 0177700, poneb, "incb" },
  { 0105300, 0177700, poneb, "decb" },
  { 0105400, 0177700, poneb, "negb" },
  { 0105500, 0177700, poneb, "adcb" },
  { 0105600, 0177700, poneb, "sbcb" },
  { 0105700, 0177700, poneb, "tstb" },
  { 0106000, 0177700, poneb, "rorb" },
  { 0106100, 0177700, poneb, "rolb" },
  { 0106200, 0177700, poneb, "asrb" },
  { 0106300, 0177700, poneb, "aslb" },
  { 0106400, 0177700, poneb, "mtps" },
  { 0106700, 0177700, poneb, "mfps" },
  { 0110000, 0170000, ptwob, "movb" },
  { 0120000, 0170000, ptwob, "cmpb" },
  { 0130000, 0170000, ptwob, "bitb" },
  { 0140000, 0170000, ptwob, "bicb" },
  { 0150000, 0170000, ptwob, "bisb" },
  { 0160000, 0170000, ptwob, "sub" },

  { 0, 0, 0, nil },
};

/*
** doinstr() is the main workhorse that does instruction decoding.
*/

static void doinstr(void)
{
  int i;

  opcode = getword();
  dstreg = opcode & 7;
  dstmod = (opcode >> 3) & 7;
  srcreg = (opcode >> 6) & 7;
  srcmod = (opcode >> 9) & 7;

  pb_length = 2;

  i = 0;
  while (itable[i].mask != 0) {
    if ((opcode & itable[i].mask) == itable[i].instr) {
      (itable[i].work)(itable[i].name);
      pb_status = st_inst;
      return;
    }
    i += 1;
  }
  doword(opcode);
}

/*
** This routine will be called once for each address where there is any
** data like labels, comments, ... defined.  This takes place when we
** generate the beginning of the program, and we use it to output all
** symbols (labels) that corresponds to unmapped memory.
*/

static void checkunmap(address* a)
{
  if (!mapped(a) && l_exist(a)) {
    bufstring(l_find(a));
    bufstring("==");
    number(a_a2w(a));
    if (c_exist(a)) {
      tabto(32);
      bufchar(';');
      bufstring(c_find(a));
    }
    bufnewline();
  }
}

/*
** This routine handles things like generating the beginning and end of
** the output, as well as "moving" between different segments of memory.
*/

void pdp11_spec(address* a, int func)
{
  if (func == SPC_BEGIN) {
    tabto(8);
    casestring(".TITLE");
    tabdelim();
    casestring(".MAIN");
    bufblankline();
    foreach(checkunmap);
    bufblankline();
  }

  if (func == SPC_ORG) {
    bufblankline();
    bufstring(".=");
    bufstring(a_a2str(a));
    bufblankline();
  }

  if (func == SPC_END) {
    bufblankline();
    tabto(8);
    casestring(".END");
  }
}

/*
** the main entry is the peek routine.  This should need a minimum of work.
*/

void pdp11_peek(stcode prefer)
{
  /* We wait with expanding data, labels etc. until we know length... */

  if (d_exist(istart)) {
    bufblankline();
    startline(false);
    bufdescription(istart, ";");
    bufblankline();
  }

  if ((prefer == st_none) && e_exist(istart)) {
    pb_length = e_length(istart);
    startline(true);
    bufstring(e_find(istart));
  } else {
    switch (pb_status) {
    case st_none:
      /* st_none is handled as instruction: */
    case st_inst:
      doinstr();
      break;
    case st_ptr:
      doptr(getword());
      break;
    case st_word:
      doword(getword());
      break;
    case st_char:
      dochar(getbyte());
      break;
    case st_text:
      dotext();
      break;
    case st_byte:
      dobyte(getbyte());
      break;
    default:
      dobyte(getbyte());
      break;
    }
  }

  stdcomment(32, ";");

  restline();
}

/************************************************************************/

/*
** Address to string translator:
*/

char* pdp11_a2s(address* a)
{
  static char work[10];

  sprintf(work, "%o", a_a2w(a));
  return(work);
}

/*
** String to address parser:
*/

address* pdp11_s2a(char* p)
{
  word w;
  char c;

  w = 0;

  while ((c = *p++) != (char) 0) {
    if ((c < '0') || (c > '7')) {
      return(nil);
    }
    w = (w << 3) + (c - '0');
  }
  return(a_l2a(w & 0177777));
}

/*
** Canonicalize a label:
*/

char* pdp11_lcan(char* name)
{
  static char work[10];

  return(canonicalize(name, work, 6));
}

/*
** Check label for valid syntax:
*/

bool pdp11_lchk(char* name)
{
  return(checkstring(name, ".$", "0123456789%.$"));
}

/*
** Autogenerate a label at specified address:
*/

void pdp11_lgen(address* addr)
{
  genlabel(addr);
}

/*
** Return list of starting addresses:
*/

address* pdp11_auto(void)
{
  return(nil);
}

/*
** Init routine, tell the world that we are little-endian and set up
** callback functions.
*/

void pdp11_init(void)
{
  /* Set up our functions: */

  spf_peek(pdp11_peek);
  spf_spec(pdp11_spec);
  spf_a2s(pdp11_a2s);
  spf_s2a(pdp11_s2a);
  spf_lcan(pdp11_lcan);
  spf_lchk(pdp11_lchk);
  spf_lgen(pdp11_lgen);
  spf_auto(pdp11_auto);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_abits = 16;		/* Number of address bits. */
  pv_bigendian = false;		/* We are little-endian. */
}

/*
** Help handler:
*/

bool pdp11_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help text for PDP11 processors.\n\
");
    return(true);
  }
  return(false);
}
