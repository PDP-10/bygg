/*
** This module implements driver code for the Motorla 6805 processor.
*/

#include "disass.h"

evf_init m6805_init;
evf_help m6805_help;

struct entryvector m6805_vector = {
  "6805",			/* Name */
  "Motorola 6805",		/* One-liner. */
  m6805_init,			/* Init routine */
  m6805_help,			/* Help routine */
};

/* configurable variables: */

extern int radix;

/*
** Start of our local variables:
*/

static address* touch;

/************************************************************************/

/* itype values: */

#define unused  0
#define inst    1
#define instb   2
#define instw   3
#define jumpa   4
#define jumpr   5
#define calla   6
#define callr   7
#define ret     8

/* flags: */

/* no flags defined yet. */

/* ---- main table ---- */

static dispblock maindisp[256] = {

/* NOTE THAT THIS IS A COPY OF THE M6800 TABLE AT THE MOMENT */

  { 0x00, 1, inst,  0,  "test" },
  { 0x01, 1, inst,  0,  "nop" },
  { 0x02, 1, inst,  0,  "idiv" },
  { 0x03, 1, inst,  0,  "fdiv" },
  { 0x04, 1, inst,  0,  "lsrd" },
  { 0x05, 1, inst,  0,  "lsld" },
  { 0x06, 1, inst,  0,  "tap" },
  { 0x07, 1, inst,  0,  "tpa" },
  { 0x08, 1, inst,  0,  "inx" },
  { 0x09, 1, inst,  0,  "dex" },
  { 0x0a, 1, inst,  0,  "clv" },
  { 0x0b, 1, inst,  0,  "sev" },
  { 0x0c, 1, inst,  0,  "clc" },
  { 0x0d, 1, inst,  0,  "sec" },
  { 0x0e, 1, inst,  0,  "cli" },
  { 0x0f, 1, inst,  0,  "sei" },
  { 0x10, 1, inst,  0,  "sba" },
  { 0x11, 1, inst,  0,  "cba" },
  { 0x12, 4, inst,  0,  "brset %0 %m %d" }, /* ???? */
  { 0x13, 4, inst,  0,  "brclr %0 %m %d" }, /* ???? */
  { 0x14, 3, inst,  0,  "bset %0 %m" },
  { 0x15, 3, inst,  0,  "bclr %0 %m" },
  { 0x16, 1, inst,  0,  "tab" },
  { 0x17, 1, inst,  0,  "tba" },
  { 0x18, 0, pfx18, 0,  0 },
  { 0x19, 1, inst,  0,  "daa" },
  { 0x1a, 0, pfx1A, 0,  0 },
  { 0x1b, 1, inst,  0,  "aba" },
  { 0x1c, 3, inst,  0,  "bset %x %m" },
  { 0x1d, 3, inst,  0,  "bclr %x %m" },
  { 0x1e, 4, callr, 0,  "brset %x %m %d" },
  { 0x1f, 4, callr, 0,  "brclr %x %m %d" },
  { 0x20, 2, jumpr, 0,  "bra %d" },
  { 0x21, 2, inst,  0,  "brn %d" },
  { 0x22, 2, callr, 0,  "bhi %d" },
  { 0x23, 2, callr, 0,  "bls %d" },
  { 0x24, 2, callr, 0,  "bcc %d" },
  { 0x25, 2, callr, 0,  "bcs %d" },
  { 0x26, 2, callr, 0,  "bne %d" },
  { 0x27, 2, callr, 0,  "beq %d" },
  { 0x28, 2, callr, 0,  "bvc %d" },
  { 0x29, 2, callr, 0,  "bvs %d" },
  { 0x2a, 2, callr, 0,  "bpl %d" },
  { 0x2b, 2, callr, 0,  "bmi %d" },
  { 0x2c, 2, callr, 0,  "bge %d" },
  { 0x2d, 2, callr, 0,  "blt %d" },
  { 0x2e, 2, callr, 0,  "bgt %d" },
  { 0x2f, 2, callr, 0,  "ble %d" },
  { 0x30, 1, inst,  0,  "tsx" },
  { 0x31, 1, inst,  0,  "ins" },
  { 0x32, 1, inst,  0,  "pula" },
  { 0x33, 1, inst,  0,  "pulb" },
  { 0x34, 1, inst,  0,  "des" },
  { 0x35, 1, inst,  0,  "txs" },
  { 0x36, 1, inst,  0,  "psha" },
  { 0x37, 1, inst,  0,  "pshb" },
  { 0x38, 1, inst,  0,  "pulx" },
  { 0x39, 1, ret,   0,  "rts" },
  { 0x3a, 1, inst,  0,  "abx" },
  { 0x3b, 1, ret,   0,  "rti" },
  { 0x3c, 1, inst,  0,  "pshx" },
  { 0x3d, 1, inst,  0,  "mul" },
  { 0x3e, 1, inst,  0,  "wai" },
  { 0x3f, 1, inst,  0,  "swi" },
  { 0x40, 1, inst,  0,  "nega" },
  { 0x41, 0, unused, 0, 0 },
  { 0x42, 0, unused, 0, 0 },
  { 0x43, 1, inst,  0,  "coma" },
  { 0x44, 1, inst,  0,  "lsra" },
  { 0x45, 0, unused, 0, 0 },
  { 0x46, 1, inst,  0,  "rora" },
  { 0x47, 1, inst,  0,  "asra" },
  { 0x48, 1, inst,  0,  "lsla" },
  { 0x49, 1, inst,  0,  "rola" },
  { 0x4a, 1, inst,  0,  "deca" },
  { 0x4b, 0, unused, 0, 0 },
  { 0x4c, 1, inst,  0,  "inca" },
  { 0x4d, 1, inst,  0,  "tsta" },
  { 0x4e, 0, unused, 0, 0 },
  { 0x4f, 1, inst,  0,  "clra" },
  { 0x50, 1, inst,  0,  "negb" },
  { 0x51, 0, unused, 0, 0 },
  { 0x52, 0, unused, 0, 0 },
  { 0x53, 1, inst,  0,  "comb" },
  { 0x54, 1, inst,  0,  "lsrb" },
  { 0x55, 0, unused, 0, 0 },
  { 0x56, 1, inst,  0,  "rorb" },
  { 0x57, 1, inst,  0,  "asrb" },
  { 0x58, 1, inst,  0,  "lslb" },
  { 0x59, 1, inst,  0,  "rolb" },
  { 0x5a, 1, inst,  0,  "decb" },
  { 0x5b, 0, unused, 0, 0 },
  { 0x5c, 1, inst,  0,  "incb" },
  { 0x5d, 1, inst,  0,  "tstb" },
  { 0x5e, 0, unused, 0, 0 },
  { 0x5f, 1, inst,  0,  "clrb" },
  { 0x60, 2, inst,  0,  "neg %x" },
  { 0x61, 0, unused, 0, 0 },
  { 0x62, 0, unused, 0, 0 },
  { 0x63, 2, inst,  0,  "com %x" },
  { 0x64, 2, inst,  0,  "lsr %x" },
  { 0x65, 0, unused, 0, 0 },
  { 0x66, 2, inst,  0,  "ror %x" },
  { 0x67, 2, inst,  0,  "asr %x" },
  { 0x68, 2, inst,  0,  "lsl %x" },
  { 0x69, 2, inst,  0,  "rol %x" },
  { 0x6a, 2, inst,  0,  "dec %x" },
  { 0x6b, 0, unused, 0, 0 },
  { 0x6c, 2, inst,  0,  "inc %x" },
  { 0x6d, 2, inst,  0,  "tst %x" },
  { 0x6e, 2, ret,   0,  "jmp %x" },
  { 0x6f, 2, inst,  0,  "clr %x" },
  { 0x70, 3, instb, 0,  "neg %a" },
  { 0x71, 0, unused, 0, 0 },
  { 0x72, 0, unused, 0, 0 },
  { 0x73, 3, instb, 0,  "com %a" },
  { 0x74, 3, instb, 0,  "lsr %a" },
  { 0x75, 0, unused, 0, 0 },
  { 0x76, 3, instb, 0,  "ror %a" },
  { 0x77, 3, instb, 0,  "asr %a" },
  { 0x78, 3, instb, 0,  "lsl %a" },
  { 0x79, 3, instb, 0,  "rol %a" },
  { 0x7a, 3, instb, 0,  "dec %a" },
  { 0x7b, 0, unused, 0, 0 },
  { 0x7c, 3, instb, 0,  "inc %a" },
  { 0x7d, 3, instb, 0,  "tst %a" },
  { 0x7e, 3, jumpa, 0,  "jmp %a" },
  { 0x7f, 3, instb, 0,  "clr %a" },
  { 0x80, 2, inst,  0,  "suba %1" },
  { 0x81, 2, inst,  0,  "cmpa %1" },
  { 0x82, 2, inst,  0,  "sbca %1" },
  { 0x83, 3, inst,  0,  "subd %2" },
  { 0x84, 2, inst,  0,  "anda %1" },
  { 0x85, 2, inst,  0,  "bita %1" },
  { 0x86, 2, inst,  0,  "ldaa %1" },
  { 0x87, 0, unused, 0, 0 },
  { 0x88, 2, inst,  0,  "eora %1" },
  { 0x89, 2, inst,  0,  "adca %1" },
  { 0x8a, 2, inst,  0,  "oraa %1" },
  { 0x8b, 2, inst,  0,  "adda %1" },
  { 0x8c, 3, inst,  0,  "cpx %2" },
  { 0x8d, 2, callr, 0,  "bsr %d" },
  { 0x8e, 3, inst,  0,  "lds %2" },
  { 0x8f, 1, inst,  0,  "xgdx" },
  { 0x90, 2, instb, 0,  "suba %0" },
  { 0x91, 2, instb, 0,  "cmpa %0" },
  { 0x92, 2, instb, 0,  "sbca %0" },
  { 0x93, 2, instw, 0,  "subd %0" },
  { 0x94, 2, instb, 0,  "anda %0" },
  { 0x95, 2, instb, 0,  "bita %0" },
  { 0x96, 2, instb, 0,  "ldaa %0" },
  { 0x97, 2, instb, 0,  "staa %0" },
  { 0x98, 2, instb, 0,  "eora %0" },
  { 0x99, 2, instb, 0,  "adca %0" },
  { 0x9a, 2, instb, 0,  "oraa %0" },
  { 0x9b, 2, instb, 0,  "adda %0" },
  { 0x9c, 2, instw, 0,  "cpx %0" },
  { 0x9d, 2, calla, 0,  "jsr %0" },
  { 0x9e, 2, instw, 0,  "lds %0" },
  { 0x9f, 2, instw, 0,  "sts %0" },
  { 0xa0, 2, inst, 0,  "suba %x" },
  { 0xa1, 2, inst, 0,  "cmpa %x" },
  { 0xa2, 2, inst, 0,  "sbca %x" },
  { 0xa3, 2, inst, 0,  "subd %x" },
  { 0xa4, 2, inst, 0,  "anda %x" },
  { 0xa5, 2, inst, 0,  "bita %x" },
  { 0xa6, 2, inst, 0,  "ldaa %x" },
  { 0xa7, 2, inst, 0,  "staa %x" },
  { 0xa8, 2, inst, 0,  "eora %x" },
  { 0xa9, 2, inst, 0,  "adca %x" },
  { 0xaa, 2, inst, 0,  "oraa %x" },
  { 0xab, 2, inst, 0,  "adda %x" },
  { 0xac, 2, inst, 0,  "cpx %x" },
  { 0xad, 2, inst, 0,  "jsr %x" },
  { 0xae, 2, inst, 0,  "lds %x" },
  { 0xaf, 2, inst, 0,  "sts %x" },
  { 0xb0, 3, instb, 0,  "suba %a" },
  { 0xb1, 3, instb, 0,  "cmpa %a" },
  { 0xb2, 3, instb, 0,  "sbca %a" },
  { 0xb3, 3, instw, 0,  "subd %a" },
  { 0xb4, 3, instb, 0,  "anda %a" },
  { 0xb5, 3, instb, 0,  "bita %a" },
  { 0xb6, 3, instb, 0,  "ldaa %a" },
  { 0xb7, 3, instb, 0,  "staa %a" },
  { 0xb8, 3, instb, 0,  "eora %a" },
  { 0xb9, 3, instb, 0,  "adca %a" },
  { 0xba, 3, instb, 0,  "oraa %a" },
  { 0xbb, 3, instb, 0,  "adda %a" },
  { 0xbc, 3, instw, 0,  "cpx %a" },
  { 0xbd, 3, calla, 0,  "jsr %a" },
  { 0xbe, 3, instw, 0,  "lds %a" },
  { 0xbf, 3, instw, 0,  "sts %a" },
  { 0xc0, 2, inst,  0,  "subb %1" },
  { 0xc1, 2, inst,  0,  "cmpb %1" },
  { 0xc2, 2, inst,  0,  "sbcb %1" },
  { 0xc3, 3, inst,  0,  "addd %2" },
  { 0xc4, 2, inst,  0,  "andb %1" },
  { 0xc5, 2, inst,  0,  "bitb %1" },
  { 0xc6, 2, inst,  0,  "ldab %1" },
  { 0xc7, 0, unused, 0, 0 },
  { 0xc8, 2, inst,  0,  "eorb %1" },
  { 0xc9, 2, inst,  0,  "adcb %1" },
  { 0xca, 2, inst,  0,  "orab %1" },
  { 0xcb, 2, inst,  0,  "addb %1" },
  { 0xcc, 3, inst,  0,  "ldd %2" },
  { 0xcd, 0, pfxCD, 0,  0 },
  { 0xce, 3, inst,  0,  "ldx %2" },
  { 0xcf, 1, inst,  0,  "stop" },
  { 0xd0, 2, instb, 0,  "subb %0" },
  { 0xd1, 2, instb, 0,  "cmpb %0" },
  { 0xd2, 2, instb, 0,  "sbcb %0" },
  { 0xd3, 2, instw, 0,  "addd %0" },
  { 0xd4, 2, instb, 0,  "andb %0" },
  { 0xd5, 2, instb, 0,  "bitb %0" },
  { 0xd6, 2, instb, 0,  "ldab %0" },
  { 0xd7, 2, instb, 0,  "stab %0" },
  { 0xd8, 2, instb, 0,  "eorb %0" },
  { 0xd9, 2, instb, 0,  "adcb %0" },
  { 0xda, 2, instb, 0,  "orab %0" },
  { 0xdb, 2, instb, 0,  "addb %0" },
  { 0xdc, 2, instw, 0,  "ldd %0" },
  { 0xdd, 2, instw, 0,  "std %0" },
  { 0xde, 2, instw, 0,  "ldx %0" },
  { 0xdf, 2, instw, 0,  "stx %0" },
  { 0xe0, 2, inst,  0,  "subb %x" },
  { 0xe1, 2, inst,  0,  "cmpb %x" },
  { 0xe2, 2, inst,  0,  "sbcb %x" },
  { 0xe3, 2, inst,  0,  "addd %x" },
  { 0xe4, 2, inst,  0,  "andb %x" },
  { 0xe5, 2, inst,  0,  "bitb %x" },
  { 0xe6, 2, inst,  0,  "ldab %x" },
  { 0xe7, 2, inst,  0,  "stab %x" },
  { 0xe8, 2, inst,  0,  "eorb %x" },
  { 0xe9, 2, inst,  0,  "adcb %x" },
  { 0xea, 2, inst,  0,  "orab %x" },
  { 0xeb, 2, inst,  0,  "addb %x" },
  { 0xec, 2, inst,  0,  "ldd %x" },
  { 0xed, 2, inst,  0,  "std %x" },
  { 0xee, 2, inst,  0,  "ldx %x" },
  { 0xef, 2, inst,  0,  "stx %x" },
  { 0xf0, 3, instb, 0,  "subb %a" },
  { 0xf1, 3, instb, 0,  "cmpb %a" },
  { 0xf2, 3, instb, 0,  "sbcb %a" },
  { 0xf3, 3, instw, 0,  "addd %a" },
  { 0xf4, 3, instb, 0,  "andb %a" },
  { 0xf5, 3, instb, 0,  "bitb %a" },
  { 0xf6, 3, instb, 0,  "ldab %a" },
  { 0xf7, 3, instb, 0,  "stab %a" },
  { 0xf8, 3, instb, 0,  "eorb %a" },
  { 0xf9, 3, instb, 0,  "adcb %a" },
  { 0xfa, 3, instb, 0,  "orab %a" },
  { 0xfb, 3, instb, 0,  "addb %a" },
  { 0xfc, 3, instw, 0,  "ldd %a" },
  { 0xfd, 3, instw, 0,  "std %a" },
  { 0xfe, 3, instw, 0,  "ldx %a" },
  { 0xff, 3, instw, 0,  "stx %a" },
};

/************************************************************************/

static void hex(word w)
{
  if (w > 9) bufchar('$');
  bufhex(w, 1);
}

/*
** This routine will generate a label on the specified address, unless
** one already exist.
*/

static void genlabel(word w)
{
  char work[10];
  sprintf(work, "l.%04x", w);
  l_insert(a_l2a(w), work);
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
  hex(b);
  if (pb_actual == st_byte) {
    while (getstatus(pc) == st_cont) {
      bufstring(", ");
      hex(getbyte());
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
    bufchar('"');
    if (b == '"') {
      bufchar((char) b);
    }
    bufchar((char) b);
    bufchar('"');
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
  hex(w);
  if (pb_actual == st_word) {
    while (getstatus(pc) == st_cont) {
      bufstring(", ");
      hex(getword());
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

static void doptr(word w)
{
  address* a;

  pb_length = 2;
  a = a_l2a(w);
  if (l_exist(a)) {
    startline(true);
    casestring(".word");
    tabdelim();
    bufstring(l_find(a));
    checkblank();
  } else {
    doword(w);
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
    casestring("defm");
    spacedelim();
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

/*
** doinstr() is the main workhorse.  This is the place you will
** have to write some serious code.
*/

static void refaddr(word w)
{
  touch = a_l2a(w);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    hex(w);
    if (updateflag) {
      genlabel(w);
    }
  }
}

static word getdisp(void)
{
  byte b;
  b = getbyte();
  return(a_a2l(pc) + sextb(b));
}

static void refdisp(word w)
{
  int i;

  touch = a_l2a(w);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    if (updateflag) {
      genlabel(w);
    }
    i = w + 2 - a_a2l(pc);
    bufchar('.');
    if (i < 0) {
      bufchar('-'); hex(-i);
    }
    if (i > 0) {
      bufchar('+'); hex(i);
    }
  }
}

static void iname(char c)
{
  hex(getbyte());
  bufchar(',');
  casechar(c);
}

static void copytext(char* expand)
{
  char c;

  while ((c = *(expand++)) != (char) 0) {
    if (c == '%') {
      c = *(expand++);
      switch (c) {
	case '0':		/* Zero-page address */
	  refaddr(getbyte());
	  break;
	case 'm':		/* Bit mask (?) */
	case '1':		/* One byte of data */
	  bufchar('#');
	  hex(getbyte());
	  break;
	case '2':		/* One word of data */
	  bufchar('#');
	  hex(getword());
	  break;
	case 'a':		/* Absolute address */
	  refaddr(getword());
	  break;
	case 'd':		/* Relative address */
	  refdisp(getdisp());
	  break;
	case 'x':		/* Index X */
	  iname('x');
	  break;
	case 'y':		/* Index Y */
	  iname('y');
	  break;
      }
    } else if (c == ' ') {
      spacedelim();
    } else {
      casechar(c);
    }
  }
}

static void doinstr(void)
{
  byte opcode;
  dispblock* disp;

  opcode = getbyte();
  disp = &maindisp[opcode];
  
  if (disp->itype == unused) {
    dobyte(opcode);
    return;
  }

  pb_length = disp->length;

  startline(true);		/* Here we know length.  Start off line. */
  copytext(disp->expand);	/* Copy expansion, expanding labels etc. */

  switch (disp->itype) {
  case inst:
    break;
  case instb:
    suggest(touch, st_byte, 1);
    break;
  case instw:
    suggest(touch, st_word, 2);
    break;
  case jumpa:
  case jumpr:
    pb_detour = touch;
    pb_deadend = true;
    delayblank = true;
    break;
  case calla:
  case callr:
    pb_detour = touch;
    break;
  case ret:
    pb_deadend = true;
    delayblank = true;
    break;
  }
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
    tabspace(8);
    casestring("equ");
    spacedelim();
    hex(a_a2l(a));
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

void m6805_spec(address* a, int func)
{
  if (func == SPC_BEGIN) {
    bufstring(";Beginning of program");
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
    bufstring(";End of program");
  }
}

/*
** the main entry is the peek routine.  This should need a minimum of work.
*/

void m6805_peek(stcode prefer)
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
** This routine returns a canonical representation of a label, used for
** looking them up by name in the database.  (Labels are stored and used
** exactly the same way as you type them in).  Labels in all known m6805
** assemblers have eight significant characters, and upper/lower case is
** considered equal.  We have a standard helper routine that copies and
** unifies (to lower) the case of a label to a work buffer, cutting off
** after a given number of bytes.  If you change this, just make sure
** that the buffer is large enough for what you will put into it, and
** don't forget to count the terminating null byte!
*/

char* m6805_lcan(char* name)
{
  static char work[10];

  return(canonicalize(name, work, 6));
}

/*
** This routine is used to check that a user-specified label string con-
** forms to the syntax for labels for the selected assembler.  Labels can
** have (in all known assemblers) letters and "." in the first position,
** and letters, "." and digits in all but the first.  Check this with a 
** standard helper routine.
*/

bool m6805_lchk(char* name)
{
  return(checkstring(name, ".", "0123456789."));
}

/*
** This routine is used to generate labels at specified addresses, from
** the command "SET LABEL <address>", if there is no label given.  In that
** case we make one up.
*/

void m6805_lgen(address* addr)
{
  genlabel(a_a2w(addr));
}

/*
** This routine will be called when we select this processor.  It is then
** our job to set up whatever we need in our environment, like telling
** the support routines if we are big- or little-endian.
*/

void m6805_init(void)
{
  /* Set up our functions: */
  
  spf_peek(m6805_peek);
  spf_spec(m6805_spec);
  spf_lcan(m6805_lcan);
  spf_lchk(m6805_lchk);
  spf_lgen(m6805_lgen);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. *
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 16;		/* Number of address bits. */
  pv_bigendian = true;		/* We are big-endian. */
}

/*
** This routine returns a help string for this processor, so that the
** user can ask us what the **** we are.  We should give information on
** what the different assemblers mean and all that jazz.
*/

bool m6805_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help text for Motorola 6805 processor.\n\
");
    return(true);
  }
  return(false);
}
