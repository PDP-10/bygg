/*
** This module implements driver code for the Motorla 6809 processor.
*/

#include "disass.h"

evf_init m6809_init;
evf_help m6809_help;

struct entryvector m6809_vector = {
  "6809",			/* Name */
  "Motorola 6809",		/* One-liner. */
  m6809_init,			/* Init routine */
  m6809_help,			/* Help routine */
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
#define jrst    4
#define pushj   5
#define popj    6

#define pfx10  10
#define pfx11  11

/* flags: */

#define IX    0x01		/* Indexed instruction. */
#define LB    0x02		/* Long (16-bit) branch. */

/*
** special values in expansion strings:
** %0 -- Short direct address, "zero page", one byte.
** %1 -- One byte of immediate data.
** %2 -- One word of immediate data.
** %a -- Address (two bytes) of something.
** %d -- Displacement, one (or two, if LB is set) byte(s).
** %i -- Index format, at least one byte of data.  See IX flag.
** %r -- Register mask
*/

/* ---- main table ---- */

static dispblock maindisp[256] = {
  { 0x00, 2, inst,   0,  "neg %0" },
  { 0x01, 0, unused, 0, 0 },
  { 0x02, 0, unused, 0, 0 },
  { 0x03, 2, inst,   0,  "com %0" },
  { 0x04, 2, inst,   0,  "lsr %0" },
  { 0x05, 0, unused, 0,  0 },
  { 0x06, 2, inst,   0,  "ror %0" },
  { 0x07, 2, inst,   0,  "asr %0" },
  { 0x08, 2, inst,   0,  "lsl %0" },
  { 0x09, 2, inst,   0,  "rol %0" },
  { 0x0a, 2, inst,   0,  "dec %0" },
  { 0x0b, 0, unused, 0,  0 },
  { 0x0c, 2, inst,   0,  "inc %0" },
  { 0x0d, 2, inst,   0,  "tst %0" },
  { 0x0e, 2, popj,   0,  "jmp %0" },
  { 0x0f, 2, inst,   0,  "clr %0" },
  { 0x10, 0, pfx10,  0,  0 },
  { 0x11, 0, pfx11,  0,  0 },
  { 0x12, 1, inst,   0,  "nop" },
  { 0x13, 1, inst,   0,  "sync" },
  { 0x14, 0, unused, 0,  0 },
  { 0x15, 0, unused, 0,  0 },
  { 0x16, 3, jrst,   LB, "lbra %d" },
  { 0x17, 3, pushj,  LB, "lbsr %d" },
  { 0x18, 0, unused, 0,  0 },
  { 0x19, 1, inst,   0,  "daa" },
  { 0x1a, 2, inst,   0,  "orcc %1" },
  { 0x1b, 0, unused, 0,  0 },
  { 0x1c, 2, inst,   0,  "andcc %1" },
  { 0x1d, 1, inst,   0,  "sex" },
  { 0x1e, 2, inst,   0,  "exg %r" },
  { 0x1f, 2, inst,   0,  "tfr %r" },
  { 0x20, 2, jrst,   0,  "bra %d" },
  { 0x21, 2, inst,   0,  "brn %d" },
  { 0x22, 2, pushj,  0,  "bhi %d" },
  { 0x23, 2, pushj,  0,  "bls %d" },
  { 0x24, 2, pushj,  0,  "bhs %d" },
  { 0x25, 2, pushj,  0,  "blo %d" },
  { 0x26, 2, pushj,  0,  "bne %d" },
  { 0x27, 2, pushj,  0,  "beq %d" },
  { 0x28, 2, pushj,  0,  "bvc %d" },
  { 0x29, 2, pushj,  0,  "bvs %d" },
  { 0x2a, 2, pushj,  0,  "bpl %d" },
  { 0x2b, 2, pushj,  0,  "bmi %d" },
  { 0x2c, 2, pushj,  0,  "bge %d" },
  { 0x2d, 2, pushj,  0,  "blt %d" },
  { 0x2e, 2, pushj,  0,  "bgt %d" },
  { 0x2f, 2, pushj,  0,  "ble %d" },
  { 0x30, 2, inst,   IX, "leax %i" },
  { 0x31, 2, inst,   IX, "leay %i" },
  { 0x32, 2, inst,   IX, "leas %i" },
  { 0x33, 2, inst,   IX, "leau %i" },
  { 0x34, 2, inst,   0,  "pshs %1" },
  { 0x35, 2, inst,   0,  "puls %1" },
  { 0x36, 2, inst,   0,  "pshu %1" },
  { 0x37, 2, inst,   0,  "pulu %1" },
  { 0x38, 0, unused, 0,  0 },
  { 0x39, 1, popj,   0,  "rts" },
  { 0x3a, 1, inst,   0,  "abx" },
  { 0x3b, 1, popj,   0,  "rti" },
  { 0x3c, 2, inst,   0,  "cwai %1" },
  { 0x3d, 1, inst,   0,  "mul" },
  { 0x3e, 0, unused, 0,  0 },
  { 0x3f, 1, inst,   0,  "swi" },
  { 0x40, 1, inst,   0,  "nega" },
  { 0x41, 0, unused, 0,  0 },
  { 0x42, 0, unused, 0,  0 },
  { 0x43, 1, inst,   0,  "coma" },
  { 0x44, 1, inst,   0,  "lsra" },
  { 0x45, 0, unused, 0,  0 },
  { 0x46, 1, inst,   0,  "rora" },
  { 0x47, 1, inst,   0,  "asra" },
  { 0x48, 1, inst,   0,  "lsla" },
  { 0x49, 1, inst,   0,  "rola" },
  { 0x4a, 1, inst,   0,  "deca" },
  { 0x4b, 0, unused, 0,  0 },
  { 0x4c, 1, inst,   0,  "inca" },
  { 0x4d, 1, inst,   0,  "tsta" },
  { 0x4e, 0, unused, 0,  0 },
  { 0x4f, 1, inst,   0,  "clra" },
  { 0x50, 1, inst,   0,  "negb" },
  { 0x51, 0, unused, 0,  0 },
  { 0x52, 0, unused, 0,  0 },
  { 0x53, 1, inst,   0,  "comb" },
  { 0x54, 1, inst,   0,  "lsrb" },
  { 0x55, 0, unused, 0,  0 },
  { 0x56, 1, inst,   0,  "rorb" },
  { 0x57, 1, inst,   0,  "asrb" },
  { 0x58, 1, inst,   0,  "lslb" },
  { 0x59, 1, inst,   0,  "rolb" },
  { 0x5a, 1, inst,   0,  "decb" },
  { 0x5b, 0, unused, 0,  0 },
  { 0x5c, 1, inst,   0,  "incb" },
  { 0x5d, 1, inst,   0,  "tstb" },
  { 0x5e, 0, unused, 0,  0 },
  { 0x5f, 1, inst,   0,  "clrb" },
  { 0x60, 2, instb,  IX, "neg %i" },
  { 0x61, 0, unused, 0,  0 },
  { 0x62, 0, unused, 0,  0 },
  { 0x63, 2, instb,  IX, "com %i" },
  { 0x64, 2, instb,  IX, "lsr %i" },
  { 0x65, 0, unused, 0,  0 },
  { 0x66, 2, instb,  IX, "ror %i" },
  { 0x67, 2, instb,  IX, "asr %i" },
  { 0x68, 2, instb,  IX, "lsl %i" },
  { 0x69, 2, instb,  IX, "rol %i" },
  { 0x6a, 2, instb,  IX, "dec %i" },
  { 0x6b, 0, unused, 0,  0 },
  { 0x6c, 2, instb,  IX, "inc %i" },
  { 0x6d, 2, instb,  IX, "tst %i" },
  { 0x6e, 2, jrst,   IX, "jmp %i" }, /* ????? */
  { 0x6f, 2, instb,  IX, "clr %i" },
  { 0x70, 3, instb,  0,  "neg %a" },
  { 0x71, 0, unused, 0,  0 },
  { 0x72, 0, unused, 0,  0 },
  { 0x73, 3, instb,  0,  "com %a" },
  { 0x74, 3, instb,  0,  "lsr %a" },
  { 0x75, 0, unused, 0,  0 },
  { 0x76, 3, instb,  0,  "ror %a" },
  { 0x77, 3, instb,  0,  "asr %a" },
  { 0x78, 3, instb,  0,  "lsl %a" },
  { 0x79, 3, instb,  0,  "rol %a" },
  { 0x7a, 3, instb,  0,  "dec %a" },
  { 0x7b, 0, unused, 0,  0 },
  { 0x7c, 3, instb,  0,  "inc %a" },
  { 0x7d, 3, instb,  0,  "tst %a" },
  { 0x7e, 3, jrst,   0,  "jmp %a" },
  { 0x7f, 3, instb,  0,  "clr %a" },
  { 0x80, 2, inst,   0,  "suba %1" },
  { 0x81, 2, inst,   0,  "cmpa %1" },
  { 0x82, 2, inst,   0,  "sbca %1" },
  { 0x83, 3, inst,   0,  "subd %2" },
  { 0x84, 2, inst,   0,  "anda %1" },
  { 0x85, 2, inst,   0,  "bita %1" },
  { 0x86, 2, inst,   0,  "lda %1" },
  { 0x87, 0, unused, 0,  0 },
  { 0x88, 2, inst,   0,  "eora %1" },
  { 0x89, 2, inst,   0,  "adca %1" },
  { 0x8a, 2, inst,   0,  "ora %1" },
  { 0x8b, 2, inst,   0,  "adda %1" },
  { 0x8c, 3, inst,   0,  "cmpx %2" },
  { 0x8d, 2, pushj,  0,  "bsr %d" },
  { 0x8e, 3, inst,   0,  "ldx %2" },
  { 0x8f, 0, unused, 0,  0 },
  { 0x90, 2, instb,  0,  "suba %0" },
  { 0x91, 2, instb,  0,  "cmpa %0" },
  { 0x92, 2, instb,  0,  "sbca %0" },
  { 0x93, 2, instw,  0,  "subd %0" },
  { 0x94, 2, instb,  0,  "anda %0" },
  { 0x95, 2, instb,  0,  "bita %0" },
  { 0x96, 2, instb,  0,  "lda %0" },
  { 0x97, 2, instb,  0,  "sta %0" },
  { 0x98, 2, instb,  0,  "eora %0" },
  { 0x99, 2, instb,  0,  "adca %0" },
  { 0x9a, 2, instb,  0,  "ora %0" },
  { 0x9b, 2, instb,  0,  "adda %0" },
  { 0x9c, 2, instw,  0,  "cmpx %0" },
  { 0x9d, 2, pushj,  0,  "jsr %0" },
  { 0x9e, 2, instw,  0,  "ldx %0" },
  { 0x9f, 2, instw,  0,  "stx %0" },
  { 0xa0, 2, instb,  IX, "suba %i" },
  { 0xa1, 2, instb,  IX, "cmpa %i" },
  { 0xa2, 2, instb,  IX, "sbca %i" },
  { 0xa3, 2, instw,  IX, "subd %i" },
  { 0xa4, 2, instb,  IX, "anda %i" },
  { 0xa5, 2, instb,  IX, "bita %i" },
  { 0xa6, 2, instb,  IX, "lda %i" },
  { 0xa7, 2, instb,  IX, "sta %i" },
  { 0xa8, 2, instb,  IX, "eora %i" },
  { 0xa9, 2, instb,  IX, "adca %i" },
  { 0xaa, 2, instb,  IX, "ora %i" },
  { 0xab, 2, instb,  IX, "adda %i" },
  { 0xac, 2, instw,  IX, "cmpx %i" },
  { 0xad, 2, pushj,  IX, "jsr %i" },
  { 0xae, 2, instw,  IX, "ldx %i" },
  { 0xaf, 2, instw,  IX, "stx %i" },
  { 0xb0, 3, instb,  0,  "suba %a" },
  { 0xb1, 3, instb,  0,  "cmpa %a" },
  { 0xb2, 3, instb,  0,  "sbca %a" },
  { 0xb3, 3, instw,  0,  "subd %a" },
  { 0xb4, 3, instb,  0,  "anda %a" },
  { 0xb5, 3, instb,  0,  "bita %a" },
  { 0xb6, 3, instb,  0,  "lda %a" },
  { 0xb7, 3, instb,  0,  "sta %a" },
  { 0xb8, 3, instb,  0,  "eora %a" },
  { 0xb9, 3, instb,  0,  "adca %a" },
  { 0xba, 3, instb,  0,  "ora %a" },
  { 0xbb, 3, instb,  0,  "adda %a" },
  { 0xbc, 3, instw,  0,  "cmpx %a" },
  { 0xbd, 3, pushj,  0,  "jsr %a" },
  { 0xbe, 3, instw,  0,  "ldx %a" },
  { 0xbf, 3, instw,  0,  "stx %a" },
  { 0xc0, 2, inst,   0,  "subb %1" },
  { 0xc1, 2, inst,   0,  "cmpb %1" },
  { 0xc2, 2, inst,   0,  "sbcb %1" },
  { 0xc3, 3, inst,   0,  "addd %2" },
  { 0xc4, 2, inst,   0,  "andb %1" },
  { 0xc5, 2, inst,   0,  "bitb %1" },
  { 0xc6, 2, inst,   0,  "ldb %1" },
  { 0xc7, 0, unused, 0,  0 },
  { 0xc8, 2, inst,   0,  "eorb %1" },
  { 0xc9, 2, inst,   0,  "adcb %1" },
  { 0xca, 2, inst,   0,  "orb %1" },
  { 0xcb, 2, inst,   0,  "addb %1" },
  { 0xcc, 3, inst,   0,  "ldd %2" },
  { 0xcd, 2, unused, 0,  0 },
  { 0xce, 3, inst,   0,  "ldu %2" },
  { 0xcf, 0, unused, 0,  0 },
  { 0xd0, 2, instb,  0,  "subb %0" },
  { 0xd1, 2, instb,  0,  "cmpb %0" },
  { 0xd2, 2, instb,  0,  "sbcb %0" },
  { 0xd3, 2, instw,  0,  "addd %0" },
  { 0xd4, 2, instb,  0,  "andb %0" },
  { 0xd5, 2, instb,  0,  "bitb %0" },
  { 0xd6, 2, instb,  0,  "ldb %0" },
  { 0xd7, 2, instb,  0,  "stb %0" },
  { 0xd8, 2, instb,  0,  "eorb %0" },
  { 0xd9, 2, instb,  0,  "adcb %0" },
  { 0xda, 2, instb,  0,  "orb %0" },
  { 0xdb, 2, instb,  0,  "addb %0" },
  { 0xdc, 2, instw,  0,  "ldd %0" },
  { 0xdd, 2, instw,  0,  "std %0" },
  { 0xde, 2, instw,  0,  "ldu %0" },
  { 0xdf, 2, instw,  0,  "stu %0" },
  { 0xe0, 2, instb,  IX, "subb %i" },
  { 0xe1, 2, instb,  IX, "cmpb %i" },
  { 0xe2, 2, instb,  IX, "sbcb %i" },
  { 0xe3, 2, instw,  IX, "addd %i" },
  { 0xe4, 2, instb,  IX, "andb %i" },
  { 0xe5, 2, instb,  IX, "bitb %i" },
  { 0xe6, 2, instb,  IX, "ldb %i" },
  { 0xe7, 2, instb,  IX, "stb %i" },
  { 0xe8, 2, instb,  IX, "eorb %i" },
  { 0xe9, 2, instb,  IX, "adcb %i" },
  { 0xea, 2, instb,  IX, "orb %i" },
  { 0xeb, 2, instb,  IX, "addb %i" },
  { 0xec, 2, instw,  IX, "ldd %i" },
  { 0xed, 2, instw,  IX, "std %i" },
  { 0xee, 2, instw,  IX, "ldu %i" },
  { 0xef, 2, instw,  IX, "stu %i" },
  { 0xf0, 3, instb,  0,  "subb %a" },
  { 0xf1, 3, instb,  0,  "cmpb %a" },
  { 0xf2, 3, instb,  0,  "sbcb %a" },
  { 0xf3, 3, instw,  0,  "addd %a" },
  { 0xf4, 3, instb,  0,  "andb %a" },
  { 0xf5, 3, instb,  0,  "bitb %a" },
  { 0xf6, 3, instb,  0,  "ldb %a" },
  { 0xf7, 3, instb,  0,  "stb %a" },
  { 0xf8, 3, instb,  0,  "eorb %a" },
  { 0xf9, 3, instb,  0,  "adcb %a" },
  { 0xfa, 3, instb,  0,  "orb %a" },
  { 0xfb, 3, instb,  0,  "addb %a" },
  { 0xfc, 3, instw,  0,  "ldd %a" },
  { 0xfd, 3, instw,  0,  "std %a" },
  { 0xfe, 3, instw,  0,  "ldu %a" },
  { 0xff, 3, instw,  0,  "stu %a" },
};

/* prefix table, prefix byte = 0x10 */

static dispblock pfx10disp[] = {
  { 0x20, 4, jrst,   LB, "lbra %d" },
  { 0x21, 4, inst,   LB, "lbrn %d" },
  { 0x22, 4, pushj,  LB, "lbhi %d" },
  { 0x23, 4, pushj,  LB, "lbls %d" },
  { 0x24, 4, pushj,  LB, "lbhs %d" },
  { 0x25, 4, pushj,  LB, "lblo %d" },
  { 0x26, 4, pushj,  LB, "lbne %d" },
  { 0x27, 4, pushj,  LB, "lbeq %d" },
  { 0x28, 4, pushj,  LB, "lbvc %d" },
  { 0x29, 4, pushj,  LB, "lbvs %d" },
  { 0x2a, 4, pushj,  LB, "lbpl %d" },
  { 0x2b, 4, pushj,  LB, "lbmi %d" },
  { 0x2c, 4, pushj,  LB, "lbge %d" },
  { 0x2d, 4, pushj,  LB, "lblt %d" },
  { 0x2e, 4, pushj,  LB, "lbgt %d" },
  { 0x2f, 4, pushj,  LB, "lble %d" },
  { 0x3f, 2, inst,   0,  "swi2" },
  { 0x83, 4, inst,   0,  "cmpd %2" },
  { 0x8c, 4, inst,   0,  "cmpy %2" },
  { 0x8e, 4, inst,   0,  "ldy %2" },
  { 0x93, 3, inst,   0,  "cmpd %0" },
  { 0x9c, 3, inst,   0,  "cmpy %0" },
  { 0x9e, 3, inst,   0,  "ldy %0" },
  { 0x9f, 3, inst,   0,  "sty %0" },
  { 0xa3, 3, inst,   IX, "cmpd %i" },
  { 0xac, 3, inst,   IX, "cmpy %i" },
  { 0xae, 3, inst,   IX, "ldy %i" },
  { 0xaf, 3, inst,   IX, "sty %i" },
  { 0xb3, 4, instw,  0,  "cmpd %a" },
  { 0xbc, 4, instw,  0,  "cmpy %a" },
  { 0xbe, 4, instw,  0,  "ldy %a" },
  { 0xbf, 4, instw,  0,  "sty %a" },
  { 0xce, 4, inst,   0,  "lds %2" },
  { 0xde, 3, inst,   0,  "lds %0" },
  { 0xdf, 3, inst,   0,  "sts %0" },
  { 0xee, 3, inst,   IX, "lds %0" },
  { 0xef, 3, inst,   IX, "sts %0" },
  { 0xfe, 4, instw,  0,  "lds %a" },
  { 0xff, 4, instw,  0,  "sts %a" },
  { 0,    0, arnold, 0,   0 },
};

/* prefix table, prefix byte = 0x11 */

static dispblock pfx11disp[] = {
  { 0x3f, 2, inst,   0,  "swi3" },
  { 0x83, 4, inst,   0,  "cmpu %2" },
  { 0x8c, 4, inst,   0,  "cmps %2" },
  { 0x93, 3, inst,   0,  "cmpu %0" },
  { 0x9c, 3, inst,   0,  "cmps %0" },
  { 0xa3, 3, inst,   IX, "cmpu %i" },
  { 0xac, 3, inst,   IX, "cmps %i" },
  { 0xb3, 4, inst,   0,  "cmpu %a" },
  { 0xbc, 4, inst,   0,  "cmps %a" },
  { 0,    0, arnold, 0,   0 },
};

/************************************************************************/

static void hex(word w)
{
  if (w > 9) {
    bufchar('$');
  }
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
  spacedelim();
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
    spacedelim();
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
  spacedelim();
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
    spacedelim();
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
    casestring(".ascii");
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
  return(a_a2w(pc) + sextb(b));
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

static int indexlength(byte postbyte)
{
  if (postbyte >= 128) {
    postbyte &= 0x0f;
    if (postbyte == 0x08) return(1);
    if (postbyte == 0x09) return(2);
    if (postbyte == 0x0c) return(1);
    if (postbyte == 0x0d) return(2);
    if (postbyte == 0x0f) return(2);
  }
  return(0);
}

static void copyindex(byte postbyte)
{
  char ixreg;

  ixreg = "XYUS"[(postbyte >> 5) & 0x03];

  if (postbyte & 0x80) {
    hex((postbyte & 0x1f));	/* DO SIGN EXTEND! */
    bufchar(',');
    casechar(ixreg);
  } else {
    if (postbyte & 0x10) {
      bufchar('[');
    }
    switch (postbyte & 0x0f) {
    case 0:  bufchar(','); casechar(ixreg); bufchar('+'); break;
    case 1:  bufchar(','); casechar(ixreg); bufchar('+'); bufchar('+'); break;
    case 2:  bufchar(','); bufchar('-'); bufchar('-'); casechar(ixreg); break;
    case 3:  bufchar(','); bufchar('-'); casechar(ixreg); break;
    case 4:  bufchar(','); casechar(ixreg); break;
    case 5:  casechar('A'); bufchar(','); casechar(ixreg); break;
    case 6:  casechar('B'); bufchar(','); casechar(ixreg); break;
    case 7:  /* does not exist */
    case 8:  hex(sextb(getbyte())); bufchar(','); casechar(ixreg); break;
    case 9:  hex(sextb(getword())); bufchar(','); casechar(ixreg); break;
    case 10: /* does not exist */
    case 11: casechar('D'); bufchar(','); casechar(ixreg); break;
    case 12: hex(sextb(getbyte())); bufchar(','); casestring("PCR"); break;
    case 13: hex(sextb(getword())); bufchar(','); casestring("PCR"); break;
    case 14: /* does not exist */
    case 15: hex(getword()); break;
    }
    if (postbyte & 0x10) {
      bufchar(']');
    }
  }
}

static void copytext(char* expand)
{
  char c;

  while ((c = *(expand++)) != (char) 0) {
    if (c == '%') {
      c = *(expand++);
      switch (c) {
      case '0':			/* Zero-page address */
	refaddr(getbyte());	/* THIS DOES NOT WORK! */
	break;
      case '1':			/* One byte of data */
	bufchar('#');
	hex(getbyte());
	break;
      case '2':			/* One word of data */
	bufchar('#');
	hex(getword());
	break;
      case 'a':			/* Absolute address */
	refaddr(getword());
	break;
      case 'd':			/* Relative address */
	refdisp(getdisp());
	break;
      case 'r':			/* Register pair */
	bufchar('#');
	hex(getbyte());
	break;
      case 'i':			/* Index postbyte... */
	copyindex(getbyte());
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
  
  switch (disp->itype) {
  case pfx10:
    disp = finddisp(getbyte(), pfx10disp);
    break;
  case pfx11:
    disp = finddisp(getbyte(), pfx11disp);
    break;
  case unused:
    disp = nil;
    break;
  }

  if (disp == nil) {
    dobyte(opcode);
    return;
  }

  pb_length = disp->length;

  if (disp->flags & IX) {
    pb_length += indexlength(getmemory(pc));
  }

  startline(true);		/* Here we know length.  Start off line. */
  copytext(disp->expand);	/* Copy expansion, expanding labels etc. */

  switch (disp->itype) {
  case instb:
    if (updateflag) {
      setstatus(touch, st_byte, 1);
    }
    break;
  case instw:
    if (updateflag) {
      setstatus(touch, st_word, 2);
    }
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

void m6809_spec(address* a, int func)
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

void m6809_peek(stcode prefer)
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
** exactly the same way as you type them in).  Labels in all known m6809
** assemblers have eight significant characters, and upper/lower case is
** considered equal.  We have a standard helper routine that copies and
** unifies (to lower) the case of a label to a work buffer, cutting off
** after a given number of bytes.  If you change this, just make sure
** that the buffer is large enough for what you will put into it, and
** don't forget to count the terminating null byte!
*/

char* m6809_lcan(char* name)
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

bool m6809_lchk(char* name)
{
  return(checkstring(name, ".", "0123456789."));
}

/*
** This routine is used to generate labels at specified addresses, from
** the command "SET LABEL <address>", if there is no label given.  In that
** case we make one up.
*/

void m6809_lgen(address* addr)
{
  genlabel(a_a2w(addr));
}

/**********************************************************************/

/*
** This routine will be called when we select this processor.  It is then
** our job to set up whatever we need in our environment, like telling
** the support routines if we are big- or little-endian.
*/

void m6809_init(void)
{
  /* Set up our functions: */
  
  spf_peek(m6809_peek);
  spf_spec(m6809_spec);
  spf_lcan(m6809_lcan);
  spf_lchk(m6809_lchk);
  spf_lgen(m6809_lgen);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line, expanded. */
  pv_abits = 16;		/* Number of address bits. */
  pv_bigendian = true;		/* We are big-endian. */
}

/*
** This routine prints a help string for this processor, so that the
** user can ask us what the **** we are.  We should give information on
** what the different assemblers mean and all that jazz.
*/

bool m6809_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help text for Motorola 6809 processor.\n\
");
    return(true);
  }
  return(false);
}
