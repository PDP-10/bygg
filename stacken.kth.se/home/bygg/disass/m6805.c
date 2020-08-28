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

/*
** Start of our local variables:
*/

static address* touch;

/************************************************************************/

/* itype values: */

#define unused  0
#define inst    1
#define instb   2
#define jrst    4
#define pushj   5
#define popj    6

/* ---- main table ---- */

static dispblock maindisp[256] = {
  { 0x00, 3, pushj,  0, "brset0 %0,%d" },
  { 0x01, 3, pushj,  0, "brclr0 %0,%d" },
  { 0x02, 3, pushj,  0, "brset1 %0,%d" },
  { 0x03, 3, pushj,  0, "brclr1 %0,%d" },
  { 0x04, 3, pushj,  0, "brset2 %0,%d" },
  { 0x05, 3, pushj,  0, "brclr2 %0,%d" },
  { 0x06, 3, pushj,  0, "brset3 %0,%d" },
  { 0x07, 3, pushj,  0, "brclr3 %0,%d" },
  { 0x08, 3, pushj,  0, "brset4 %0,%d" },
  { 0x09, 3, pushj,  0, "brclr4 %0,%d" },
  { 0x0a, 3, pushj,  0, "brset5 %0,%d" },
  { 0x0b, 3, pushj,  0, "brclr5 %0,%d" },
  { 0x0c, 3, pushj,  0, "brset6 %0,%d" },
  { 0x0d, 3, pushj,  0, "brclr6 %0,%d" },
  { 0x0e, 3, pushj,  0, "brset7 %0,%d" },
  { 0x0f, 3, pushj,  0, "brclr7 %0,%d" },

  { 0x10, 2, inst,   0, "bset0 %0" },
  { 0x11, 2, inst,   0, "bclr0 %0" },
  { 0x12, 2, inst,   0, "bset1 %0" },
  { 0x13, 2, inst,   0, "bclr1 %0" },
  { 0x14, 2, inst,   0, "bset2 %0" },
  { 0x15, 2, inst,   0, "bclr2 %0" },
  { 0x16, 2, inst,   0, "bset3 %0" },
  { 0x17, 2, inst,   0, "bclr3 %0" },
  { 0x18, 2, inst,   0, "bset4 %0" },
  { 0x19, 2, inst,   0, "bclr4 %0" },
  { 0x1a, 2, inst,   0, "bset5 %0" },
  { 0x1b, 2, inst,   0, "bclr5 %0" },
  { 0x1c, 2, inst,   0, "bset6 %0" },
  { 0x1d, 2, inst,   0, "bclr6 %0" },
  { 0x1e, 2, inst,   0, "bset7 %0" },
  { 0x1f, 2, inst,   0, "bclr7 %0" },

  { 0x20, 2, jrst,   0, "bra %d" },
  { 0x21, 2, inst,   0, "brn %d" },
  { 0x22, 2, pushj,  0, "bhi %d" },
  { 0x23, 2, pushj,  0, "bls %d" },
  { 0x24, 2, pushj,  0, "bcc %d" },
  { 0x25, 2, pushj,  0, "bcs %d" },
  { 0x26, 2, pushj,  0, "bne %d" },
  { 0x27, 2, pushj,  0, "beq %d" },
  { 0x28, 2, pushj,  0, "bhcc %d" },
  { 0x29, 2, pushj,  0, "bhcs %d" },
  { 0x2a, 2, pushj,  0, "bpl %d" },
  { 0x2b, 2, pushj,  0, "bmi %d" },
  { 0x2c, 2, pushj,  0, "bmc %d" },
  { 0x2d, 2, pushj,  0, "bms %d" },
  { 0x2e, 2, pushj,  0, "bil %d" },
  { 0x2f, 2, pushj,  0, "bih %d" },

  { 0x30, 2, inst,   0, "neg %0" },
  { 0x31, 0, unused, 0, 0 },
  { 0x32, 0, unused, 0, 0 },
  { 0x33, 2, inst,   0, "com %0" },
  { 0x34, 2, inst,   0, "lsr %0" },
  { 0x35, 0, unused, 0, 0 },
  { 0x36, 2, inst,   0, "ror %0" },
  { 0x37, 2, inst,   0, "asr %0" },
  { 0x38, 2, inst,   0, "lsl %0" },
  { 0x39, 2, inst,   0, "rol %0" },
  { 0x3a, 2, inst,   0, "dec %0" },
  { 0x3b, 0, unused, 0, 0 },
  { 0x3c, 2, inst,   0, "inc %0" },
  { 0x3d, 2, inst,   0, "tst %0" },
  { 0x3e, 0, unused, 0, 0 },
  { 0x3f, 2, inst,   0, "clr %0" },

  { 0x40, 1, inst,   0, "neg a" },
  { 0x41, 0, unused, 0, 0 },
  { 0x42, 0, unused, 0, 0 },
  { 0x43, 1, inst,   0, "com a" },
  { 0x44, 1, inst,   0, "lsr a" },
  { 0x45, 0, unused, 0, 0 },
  { 0x46, 1, inst,   0, "ror a" },
  { 0x47, 1, inst,   0, "asr a" },
  { 0x48, 1, inst,   0, "lsl a" },
  { 0x49, 1, inst,   0, "rol a" },
  { 0x4a, 1, inst,   0, "dec a" },
  { 0x4b, 0, unused, 0, 0 },
  { 0x4c, 1, inst,   0, "inc a" },
  { 0x4d, 1, inst,   0, "tst a" },
  { 0x4e, 0, unused, 0, 0 },
  { 0x4f, 1, inst,   0, "clr a" },

  { 0x50, 1, inst,   0, "neg x" },
  { 0x51, 0, unused, 0, 0 },
  { 0x52, 0, unused, 0, 0 },
  { 0x53, 1, inst,   0, "com x" },
  { 0x54, 1, inst,   0, "lsr x" },
  { 0x55, 0, unused, 0, 0 },
  { 0x56, 1, inst,   0, "ror x" },
  { 0x57, 1, inst,   0, "asr x" },
  { 0x58, 1, inst,   0, "lsl x" },
  { 0x59, 1, inst,   0, "rol x" },
  { 0x5a, 1, inst,   0, "dec x" },
  { 0x5b, 0, unused, 0, 0 },
  { 0x5c, 1, inst,   0, "inc x" },
  { 0x5d, 1, inst,   0, "tst x" },
  { 0x5e, 0, unused, 0, 0 },
  { 0x5f, 1, inst,   0, "clr x" },

  { 0x60, 2, inst,   0, "neg %1(x)" },
  { 0x61, 0, unused, 0, 0 },
  { 0x62, 0, unused, 0, 0 },
  { 0x63, 2, inst,   0, "com %1(x)" },
  { 0x64, 2, inst,   0, "lsr %1(x)" },
  { 0x65, 0, unused, 0, 0 },
  { 0x66, 2, inst,   0, "ror %1(x)" },
  { 0x67, 2, inst,   0, "asr %1(x)" },
  { 0x68, 2, inst,   0, "lsl %1(x)" },
  { 0x69, 2, inst,   0, "rol %1(x)" },
  { 0x6a, 2, inst,   0, "dec %1(x)" },
  { 0x6b, 0, unused, 0, 0 },
  { 0x6c, 2, inst,   0, "inc %1(x)" },
  { 0x6d, 2, inst,   0, "tst %1(x)" },
  { 0x6e, 0, unused, 0, 0 },
  { 0x6f, 2, inst,   0, "clr %1(x)" },

  { 0x70, 1, inst,   0, "neg (x)" },
  { 0x71, 0, unused, 0, 0 },
  { 0x72, 0, unused, 0, 0 },
  { 0x73, 1, inst,   0, "com (x)" },
  { 0x74, 1, inst,   0, "lsr (x)" },
  { 0x75, 0, unused, 0, 0 },
  { 0x76, 1, inst,   0, "ror (x)" },
  { 0x77, 1, inst,   0, "asr (x)" },
  { 0x78, 1, inst,   0, "lsl (x)" },
  { 0x79, 1, inst,   0, "rol (x)" },
  { 0x7a, 1, inst,   0, "dec (x)" },
  { 0x7b, 0, unused, 0, 0 },
  { 0x7c, 1, inst,   0, "inc (x)" },
  { 0x7d, 1, inst,   0, "tst (x)" },
  { 0x7e, 0, unused, 0, 0 },
  { 0x7f, 1, inst,   0, "clr (x)" },

  { 0x80, 1, inst,   0, "rti" },
  { 0x81, 1, inst,   0, "rts" },
  { 0x82, 0, unused, 0, 0 },
  { 0x83, 1, inst,   0, "swi" },
  { 0x84, 0, unused, 0, 0 },
  { 0x85, 0, unused, 0, 0 },
  { 0x86, 0, unused, 0, 0 },
  { 0x87, 0, unused, 0, 0 },
  { 0x88, 0, unused, 0, 0 },
  { 0x89, 0, unused, 0, 0 },
  { 0x8a, 0, unused, 0, 0 },
  { 0x8b, 0, unused, 0, 0 },
  { 0x8c, 0, unused, 0, 0 },
  { 0x8d, 0, unused, 0, 0 },
  { 0x8e, 1, inst,   0, "stop" },
  { 0x8f, 1, inst,   0, "wait" },

  { 0x90, 0, unused, 0, 0 },
  { 0x91, 0, unused, 0, 0 },
  { 0x92, 0, unused, 0, 0 },
  { 0x93, 0, unused, 0, 0 },
  { 0x94, 0, unused, 0, 0 },
  { 0x95, 0, unused, 0, 0 },
  { 0x96, 0, unused, 0, 0 },
  { 0x97, 1, inst,   0, "tax" },
  { 0x98, 1, inst,   0, "clc" },
  { 0x99, 1, inst,   0, "sec" },
  { 0x9a, 1, inst,   0, "cli" },
  { 0x9b, 1, inst,   0, "sei" },
  { 0x9c, 1, inst,   0, "rsp" },
  { 0x9d, 1, inst,   0, "nop" },
  { 0x9e, 0, unused, 0, 0 },
  { 0x9f, 1, inst,   0, "txa" },

  { 0xa0, 2, inst,   0, "sub %1" },
  { 0xa1, 2, inst,   0, "cmp %1" },
  { 0xa2, 2, inst,   0, "sbc %1" },
  { 0xa3, 2, inst,   0, "cpx %1" },
  { 0xa4, 2, inst,   0, "and %1" },
  { 0xa5, 2, inst,   0, "bit %1" },
  { 0xa6, 2, inst,   0, "lda %1" },
  { 0xa7, 0, unused, 0, 0 },
  { 0xa8, 2, inst,   0, "eor %1" },
  { 0xa9, 2, inst,   0, "adc %1" },
  { 0xaa, 2, inst,   0, "ora %1" },
  { 0xab, 2, inst,   0, "add %1" },
  { 0xac, 0, unused, 0, 0 },
  { 0xad, 2, pushj,  0, "bsr %1" },
  { 0xae, 2, inst,   0, "ldx %1" },
  { 0xaf, 0, unused, 0, 0 },

  { 0xb0, 2, instb,  0, "sub %0" },
  { 0xb1, 2, instb,  0, "cmp %0" },
  { 0xb2, 2, instb,  0, "sbc %0" },
  { 0xb3, 2, instb,  0, "cpx %0" },
  { 0xb4, 2, instb,  0, "and %0" },
  { 0xb5, 2, instb,  0, "bit %0" },
  { 0xb6, 2, instb,  0, "lda %0" },
  { 0xb7, 2, instb,  0, "sta %0" },
  { 0xb8, 2, instb,  0, "eor %0" },
  { 0xb9, 2, instb,  0, "adc %0" },
  { 0xba, 2, instb,  0, "ora %0" },
  { 0xbb, 2, instb,  0, "add %0" },
  { 0xbc, 2, jrst,   0, "jmp %0" },
  { 0xbd, 2, pushj,  0, "jsr %0" },
  { 0xbe, 2, instb,  0, "ldx %0" },
  { 0xbf, 2, instb,  0, "stx %0" },

  { 0xc0, 3, instb,  0, "sub %a" },
  { 0xc1, 3, instb,  0, "cmp %a" },
  { 0xc2, 3, instb,  0, "sbc %a" },
  { 0xc3, 3, instb,  0, "cpx %a" },
  { 0xc4, 3, instb,  0, "and %a" },
  { 0xc5, 3, instb,  0, "bit %a" },
  { 0xc6, 3, instb,  0, "lda %a" },
  { 0xc7, 3, instb,  0, "sta %a" },
  { 0xc8, 3, instb,  0, "eor %a" },
  { 0xc9, 3, instb,  0, "adc %a" },
  { 0xca, 3, instb,  0, "ora %a" },
  { 0xcb, 3, instb,  0, "add %a" },
  { 0xcc, 3, jrst,   0, "jmp %a" },
  { 0xcd, 3, pushj,  0, "jsr %a" },
  { 0xce, 3, instb,  0, "ldx %a" },
  { 0xcf, 3, instb,  0, "stx %a" },

  { 0xd0, 3, inst,   0, "sub %2(x)" },
  { 0xd1, 3, inst,   0, "cmp %2(x)" },
  { 0xd2, 3, inst,   0, "sbc %2(x)" },
  { 0xd3, 3, inst,   0, "cpx %2(x)" },
  { 0xd4, 3, inst,   0, "and %2(x)" },
  { 0xd5, 3, inst,   0, "bit %2(x)" },
  { 0xd6, 3, inst,   0, "lda %2(x)" },
  { 0xd7, 3, inst,   0, "sta %2(x)" },
  { 0xd8, 3, inst,   0, "eor %2(x)" },
  { 0xd9, 3, inst,   0, "adc %2(x)" },
  { 0xda, 3, inst,   0, "ora %2(x)" },
  { 0xdb, 3, inst,   0, "add %2(x)" },
  { 0xdc, 3, jrst,   0, "jmp %2(x)" },
  { 0xdd, 3, pushj,  0, "jsr %2(x)" },
  { 0xde, 3, inst,   0, "ldx %2(x)" },
  { 0xdf, 3, inst,   0, "stx %2(x)" },

  { 0xe0, 2, inst,   0, "sub %1(x)" },
  { 0xe1, 2, inst,   0, "cmp %1(x)" },
  { 0xe2, 2, inst,   0, "sbc %1(x)" },
  { 0xe3, 2, inst,   0, "cpx %1(x)" },
  { 0xe4, 2, inst,   0, "and %1(x)" },
  { 0xe5, 2, inst,   0, "bit %1(x)" },
  { 0xe6, 2, inst,   0, "lda %1(x)" },
  { 0xe7, 2, inst,   0, "sta %1(x)" },
  { 0xe8, 2, inst,   0, "eor %1(x)" },
  { 0xe9, 2, inst,   0, "adc %1(x)" },
  { 0xea, 2, inst,   0, "ora %1(x)" },
  { 0xeb, 2, inst,   0, "add %1(x)" },
  { 0xec, 2, jrst,   0, "jmp %1(x)" },
  { 0xed, 2, pushj,  0, "jsr %1(x)" },
  { 0xee, 2, inst,   0, "ldx %1(x)" },
  { 0xef, 2, inst,   0, "stx %1(x)" },

  { 0xf0, 1, inst,   0, "sub (x)" },
  { 0xf1, 1, inst,   0, "cmp (x)" },
  { 0xf2, 1, inst,   0, "sbc (x)" },
  { 0xf3, 1, inst,   0, "cpx (x)" },
  { 0xf4, 1, inst,   0, "and (x)" },
  { 0xf5, 1, inst,   0, "bit (x)" },
  { 0xf6, 1, inst,   0, "lda (x)" },
  { 0xf7, 1, inst,   0, "sta (x)" },
  { 0xf8, 1, inst,   0, "eor (x)" },
  { 0xf9, 1, inst,   0, "adc (x)" },
  { 0xfa, 1, inst,   0, "ora (x)" },
  { 0xfb, 1, inst,   0, "add (x)" },
  { 0xfc, 1, jrst,   0, "jmp (x)" },
  { 0xfd, 1, pushj,  0, "jsr (x)" },
  { 0xfe, 1, inst,   0, "ldx (x)" },
  { 0xff, 1, inst,   0, "stx (x)" },
};

/************************************************************************/

/*
** radix mods:
**   $ - hex
**   % - binary
**   @ - octal
*/

static void number(word w)
{
  switch (radix) {
  case 2:
    if (w > 1)
      bufchar('%');
    bufbinary(w, 1);
    break;
  case 8:
    if (w > 7)
      bufchar('@');
    bufoctal(w, 1);
    break;
  case 10:
    bufdecimal(w, 1);
    break;
  case 16:
  default:
    if (w > 9)
      bufchar('$');
    bufhex(w, 1);
    break;
  }
}

/*
** This routine will generate a label on the specified address, unless
** one already exist.
*/

static void genlabel(word w)
{
  char work[10];
  sprintf(work, "l.%04" PRIxw, w);
  l_insert(a_l2a(w), work);
}

/*
** dobyte() will output the current item as a byte of data.
*/

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

static void m6805_dobyte(void)
{
  defb(getbyte());
}

/*
** dochar() will try to output its argument as a character.  If it can't,
** it will default to dobyte().
*/

static void defc(byte b)
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
    defb(b);
  }
}

static void m6805_dochar(void)
{
  defc(getbyte());
}

/*
** doword() will output the current item as a word (16 bits) of data.
*/

static void m6805_doword(void)
{
  word w;

  w = getword();
  pb_length = 2;
  startline(true);
  casestring(".word");
  tabdelim();
  number(w);
  checkblank();
}

/*
** doptr() tries to output its argument as a pointer, i.e. if there is
** a label that corresponds to the argument, it will be used, otherwise
** we will setter for a numeric value.
*/

static void m6805_doptr(void)
{
  word w;
  address* a;

  pb_length = 2;
  w = getword();
  a = a_l2a(w);
  
  startline(true);
  casestring(".word");
  tabdelim();
  reference(a);
  if (l_exist(a)) {
    bufstring(l_find(a));
  } else {
    number(w);
  }
  endref();
  checkblank();
}

/*
** dotext() will try to decode a text constant.
*/

static void m6805_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    defc(getmemory(istart));
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
  reference(touch);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    number(w);
    if (updateflag) {
      genlabel(w);
    }
  }
  endref();
}

static void refdisp(byte b)
{
  int i;
  word w;

  w = a_a2l(pc) + sextb(b);

  if (babsflag) {
    refaddr(w);
    return;
  }

  touch = a_l2a(w);
  reference(touch);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    if (updateflag) {
      genlabel(w);
    }
    i = w  - a_a2l(istart);
    bufchar('.');
    if (i < 0) {
      bufchar('-'); number(-i);
    }
    if (i > 0) {
      bufchar('+'); number(i);
    }
  }
  endref();
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
	case '1':		/* One byte of data */
	  bufchar('#');
	  number(getbyte());
	  break;
	case '2':		/* One word of data */
	  bufchar('#');
	  number(getword());
	  break;
	case 'a':		/* Absolute address */
	  refaddr(getword());
	  break;
	case 'd':		/* Relative address */
	  refdisp(getbyte());
	  break;
      }
    } else if (c == ' ') {
      spacedelim();
    } else {
      casechar(c);
    }
  }
}

static void m6805_doinstr(void)
{
  byte opcode;
  dispblock* disp;

  opcode = getbyte();
  disp = &maindisp[opcode];
  
  if (disp->itype == unused) {
    defb(opcode);
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
  case jrst:
    pb_detour = touch;
    pb_deadend = true;
    delayblank = true;
    break;
  case pushj:
    pb_detour = touch;
    break;
  case popj:
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
    tabto(8);
    casestring("equ");
    spacedelim();
    number(a_a2l(a));
    if (c_exist(a)) {
      tabto(32);
      bufchar(';');
      bufstring(c_find(a));
    }
    bufnewline();
  }
}

void m6805_begin(void)
{
  bufstring(";Beginning of program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

void m6805_org(address* a)
{
  bufblankline();
  bufstring(".=");
  bufstring(a_a2str(a));
  bufblankline();
}

void m6805_end(void)
{
  bufblankline();
  bufstring(";End of program");
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

  return canonicalize(name, work, 6);
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
  return checkstring(name, ".", "0123456789.");
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
  
  spf_lcan(m6805_lcan);
  spf_lchk(m6805_lchk);
  spf_lgen(m6805_lgen);

  /* set up our object handlers: */
  
  spf_dodef(m6805_dobyte);

  spf_doobj(st_inst, m6805_doinstr);
  spf_doobj(st_byte, m6805_dobyte);
  spf_doobj(st_word, m6805_doword);
  spf_doobj(st_ptr,  m6805_doptr);
  spf_doobj(st_char, m6805_dochar);
  spf_doobj(st_text, m6805_dotext);

  spf_begin(m6805_begin);
  spf_end(m6805_end);
  spf_org(m6805_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 12;		/* Number of address bits. */
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
    return true;
  }
  return false;
}
