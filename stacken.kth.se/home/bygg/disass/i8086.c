/*
** This module implements driver code for the Intel 8086 family of
** processors.
*/

#include "disass.h"

evf_init i8086_init;
evf_help i8086_help;

struct entryvector i8086_vector = {
  "8086",			/* Name */
  "Intel 8086",			/* One-liner */
  i8086_init,			/* Init routine */
  i8086_help,			/* Help routine */
};

evf_init i8088_init;
evf_help i8088_help;

struct entryvector i8088_vector = {
  "8088",			/* Name */
  "Intel 8088",			/* One-liner */
  i8088_init,			/* Init routine */
  i8088_help,			/* Help routine */
};

evf_init i80186_init;
evf_help i80186_help;

struct entryvector i80186_vector = {
  "80186",			/* Name */
  "Intel 80186",		/* One-liner */
  i80186_init,			/* Init routine */
  i80186_help,			/* Help routine */
};

evf_init i80286_init;
evf_help i80286_help;

struct entryvector i80286_vector = {
  "80286",			/* Name */
  "Intel 80286",		/* One-liner */
  i80286_init,			/* Init routine */
  i80286_help,			/* Help routine */
};

evf_init i80386_init;
evf_help i80386_help;

struct entryvector i80386_vector = {
  "80386",			/* Name */
  "Intel 80386",		/* One-liner */
  i80386_init,			/* Init routine */
  i80386_help,			/* Help routine */
};

evf_init i80486_init;
evf_help i80486_help;

struct entryvector i80486_vector = {
  "80486",			/* Name */
  "Intel 80486",		/* One-liner */
  i80486_init,			/* Init routine */
  i80486_help,			/* Help routine */
};

evf_init i80486p_init;
evf_help i80486p_help;

struct entryvector i80486p_vector = {
  "80486-p",			/* Name */
  "Intel 80486 in protected mode", /* One-liner */
  i80486p_init,			/* Init routine */
  i80486p_help,			/* Help routine */
};

evf_init v25_init;
evf_help v25_help;

struct entryvector v25_vector = {
  "v25",			/* Name */
  "NEC V25",			/* One-liner */
  v25_init,			/* Init routine */
  v25_help,			/* Help routine */
};

evf_init x86_64_init;
evf_help x86_64_help;

struct entryvector x86_64_vector = {
  "x86-64",			/* Name */
  "AMD x86-64",			/* One-liner */
  x86_64_init,			/* Init routine */
  x86_64_help,			/* Help routine */
};

/*
** Global variables, from module common:
*/

/*
** Start of our local variables:
*/

static int cputype;		/* Subtype of processor. */
static bool protmode;		/* Protected mode? */
static bool x64mode;		/* 64-bit junk? */

static int ptrlen;		/* [temp] length of pointer, 2 or 4. */

static regindex csindex;	/* register index of "cs" register. */
static regindex dsindex;	/* register index of "ds" register. */
static regindex esindex;	/* register index of "es" register. */
static regindex ssindex;	/* register index of "ss" register. */
static regindex fsindex;	/* register index of "fs" register. */
static regindex gsindex;	/* register index of "gs" register. */

/************************************************************************/

/* itype values: */

#define unused  0
#define inst    1
#define instb   2
#define instw   3
#define pushj   4
#define popj    5
#define jrst    6

#define pfx0F   8

#define sub00  10		/* pfx = 0f 00 */
#define sub01  11		/* pfx = 0f 01 */
#define subAE  12		/* pfx = 0f ae */
#define subBA  13		/* pfx = 0f ba */

#define sub80  20		/* pfx = 80-83 */
#define subC0  21		/* pfx = c0-c1 */
#define subD0  22		/* pfx = d0-d1 */
#define subD2  23		/* pfx = d2-d3 */
#define subF6  24		/* pfx = f6-f7 */
#define subFE  25		/* pfx = fe */
#define subFF  26		/* pfx = ff */

#define pfxseg 30		/* segment prefix. */
#define pfx32a 31		/* 32-bit addr prefix. */
#define pfx32d 32		/* 32-bit data prefix. */

#define notyet unused

/* flags: */

#define WB      0x01		/* Word bit flag. */
#define EA      0x02		/* We have a [mod/reg/r-m] byte. */
#define SX      0x04		/* Sign-extend imm. data. */
#define SZ      0x08		/* We need to tell the size of EA. */
#define RX      0x10		/* This might be a REX prefix. */
#define X64     0x20		/* Not valid in X64 mode. */

/* ---- Main opcode table ---- */

static dispblock maindisp[256] = {
  { 0x00, 0, instb,  EA,       "add %e,%r" },
  { 0x01, 0, instw,  EA+WB,    "add %e,%r" },
  { 0x02, 0, instb,  EA,       "add %r,%e" },
  { 0x03, 0, instw,  EA+WB,    "add %r,%e" },
  { 0x04, 0, inst,   0,        "add $al,%1" },
  { 0x05, 0, inst,   WB,       "add $ax,%2" },
  { 0x06, 0, inst,   X64,      "push $es" },
  { 0x07, 0, inst,   X64,      "pop $es" },
  { 0x08, 0, instb,  EA,       "or %e,%r" },
  { 0x09, 0, instw,  EA+WB,    "or %e,%r" },
  { 0x0a, 0, instb,  EA,       "or %r,%e" },
  { 0x0b, 0, instw,  EA+WB,    "or %r,%e" },
  { 0x0c, 0, inst,   0,        "or $al,%1" },
  { 0x0d, 0, inst,   WB,       "or $ax,%2" },
  { 0x0e, 0, inst,   X64,      "push $cs" },
  { 0x0f, 0, pfx0F,  0,        0 },

  { 0x10, 0, instb,  EA,       "adc %e,%r" },
  { 0x11, 0, instw,  EA+WB,    "adc %e,%r" },
  { 0x12, 0, instb,  EA,       "adc %r,%e" },
  { 0x13, 0, instw,  EA+WB,    "adc %r,%e" },
  { 0x14, 0, inst,   0,        "adc $al,%1" },
  { 0x15, 0, inst,   WB,       "adc $ax,%2" },
  { 0x16, 0, inst,   X64,      "push $ss" },
  { 0x17, 0, inst,   X64,      "pop $ss" },
  { 0x18, 0, instb,  EA,       "sbb %e,%r" },
  { 0x19, 0, instw,  EA+WB,    "sbb %e,%r" },
  { 0x1a, 0, instb,  EA,       "sbb %r,%e" },
  { 0x1b, 0, instw,  EA+WB,    "sbb %r,%e" },
  { 0x1c, 0, inst,   0,        "sbb $al,%1" },
  { 0x1d, 0, inst,   WB,       "sbb $ax,%2" },
  { 0x1e, 0, inst,   X64,      "push $ds" },
  { 0x1f, 0, inst,   X64,      "pop $ds" },

  { 0x20, 0, instb,  EA,       "and %e,%r" },
  { 0x21, 0, instw,  EA+WB,    "and %e,%r" },
  { 0x22, 0, instb,  EA,       "and %r,%e" },
  { 0x23, 0, instw,  EA+WB,    "and %r,%e" },
  { 0x24, 0, inst,   0,        "and $al,%1" },
  { 0x25, 0, inst,   WB,       "and $ax,%2" },
  { 0x26, 0, pfxseg, 0,        0 },   /* segment prefix, es: */
  { 0x27, 0, inst,   X64,      "daa" },
  { 0x28, 0, instb,  EA,       "sub %e,%r" },
  { 0x29, 0, instw,  EA+WB,    "sub %e,%r" },
  { 0x2a, 0, instb,  EA,       "sub %r,%e" },
  { 0x2b, 0, instw,  EA+WB,    "sub %r,%e" },
  { 0x2c, 0, inst,   0,        "sub $al,%1" },
  { 0x2d, 0, inst,   WB,       "sub $ax,%2" },
  { 0x2e, 0, pfxseg, 0,        0 },   /* segment prefix, cs: */
  { 0x2f, 0, inst,   X64,      "das" },

  { 0x30, 0, instb,  EA,       "xor %e,%r" },
  { 0x31, 0, instw,  EA+WB,    "xor %e,%r" },
  { 0x32, 0, instb,  EA,       "xor %r,%e" },
  { 0x33, 0, instw,  EA+WB,    "xor %r,%e" },
  { 0x34, 0, inst,   0,        "xor $al,%1" },
  { 0x35, 0, inst,   WB,       "xor $ax,%2" },
  { 0x36, 0, pfxseg, 0,        0 },   /* segment prefix, ss: */
  { 0x37, 0, inst,   X64,      "aaa" },
  { 0x38, 0, instb,  EA,       "cmp %e,%r" },
  { 0x39, 0, instw,  EA+WB,    "cmp %e,%r" },
  { 0x3a, 0, instb,  EA,       "cmp %r,%e" },
  { 0x3b, 0, instw,  EA+WB,    "cmp %r,%e" },
  { 0x3c, 0, inst,   0,        "cmp $al,%1" },
  { 0x3d, 0, inst,   WB,       "cmp $ax,%2" },
  { 0x3e, 0, pfxseg, 0,        0 },   /* segment prefix, ds: */
  { 0x3f, 0, inst,   X64,      "aas" },

  /*
   * Handle the following 16 instructions as REX prefixes when in 64-bit
   * mode.  Aint this a dung heap...
   */

  { 0x40, 0, inst,   WB+RX,    "inc $ax" },
  { 0x41, 0, inst,   WB+RX,    "inc $cx" },
  { 0x42, 0, inst,   WB+RX,    "inc $dx" },
  { 0x43, 0, inst,   WB+RX,    "inc $bx" },
  { 0x44, 0, inst,   WB+RX,    "inc $sp" },
  { 0x45, 0, inst,   WB+RX,    "inc $bp" },
  { 0x46, 0, inst,   WB+RX,    "inc $si" },
  { 0x47, 0, inst,   WB+RX,    "inc $di" },
  { 0x48, 0, inst,   WB+RX,    "dec $ax" },
  { 0x49, 0, inst,   WB+RX,    "dec $cx" },
  { 0x4a, 0, inst,   WB+RX,    "dec $dx" },
  { 0x4b, 0, inst,   WB+RX,    "dec $bx" },
  { 0x4c, 0, inst,   WB+RX,    "dec $sp" },
  { 0x4d, 0, inst,   WB+RX,    "dec $bp" },
  { 0x4e, 0, inst,   WB+RX,    "dec $si" },
  { 0x4f, 0, inst,   WB+RX,    "dec $di" },

  { 0x50, 0, inst,   WB,       "push %$" },
  { 0x51, 0, inst,   WB,       "push %$" },
  { 0x52, 0, inst,   WB,       "push %$" },
  { 0x53, 0, inst,   WB,       "push %$" },
  { 0x54, 0, inst,   WB,       "push %$" },
  { 0x55, 0, inst,   WB,       "push %$" },
  { 0x56, 0, inst,   WB,       "push %$" },
  { 0x57, 0, inst,   WB,       "push %$" },
  { 0x58, 0, inst,   WB,       "pop %$" },
  { 0x59, 0, inst,   WB,       "pop %$" },
  { 0x5a, 0, inst,   WB,       "pop %$" },
  { 0x5b, 0, inst,   WB,       "pop %$" },
  { 0x5c, 0, inst,   WB,       "pop %$" },
  { 0x5d, 0, inst,   WB,       "pop %$" },
  { 0x5e, 0, inst,   WB,       "pop %$" },
  { 0x5f, 0, inst,   WB,       "pop %$" },

  { 0x60, 2, inst,   X64,      "pusha" },
  { 0x61, 2, inst,   X64,      "popa" },
  { 0x62, 2, instw,  WB+X64,   "bound %r,%e" },

  /* in 64-bit mode opcode 0x63 is MOVSXD ... */

  { 0x63, 2, inst,   WB,       "arpl %e,%r" },
  { 0x64, 3, pfxseg, 0,        0 },     /* segment prefix, fs: */
  { 0x65, 3, pfxseg, 0,        0 },     /* segment prefix, gs: */
  { 0x66, 0, pfx32d, 0,        0 },	/* prefix, register size = 32. */
  { 0x67, 0, pfx32a, 0,        0 },	/* prefix, address size = 32. */
  { 0x68, 0, inst,   0,        "push %2" },
  { 0x69, 1, instw,  EA+WB,    "imul %r,%e,%2" },
  { 0x6a, 0, inst,   0,        "push %1" },
  { 0x6b, 1, instw,  EA+SX,    "imul %r,%e,%1" },
  { 0x6c, 2, inst,   0,        "insb" },
  { 0x6d, 2, inst,   WB,       "insw" }, /* %s? */
  { 0x6e, 0, inst,   0,        "outsb" },
  { 0x6f, 0, inst,   WB,       "outsw" }, /* %s? */

  { 0x70, 0, pushj,  0,        "jo %d" },
  { 0x71, 0, pushj,  0,        "jno %d" },
  { 0x72, 0, pushj,  0,        "jb %d" },
  { 0x73, 0, pushj,  0,        "jnb %d" },
  { 0x74, 0, pushj,  0,        "je %d" },
  { 0x75, 0, pushj,  0,        "jne %d" },
  { 0x76, 0, pushj,  0,        "jbe %d" },
  { 0x77, 0, pushj,  0,        "jnbe %d" },
  { 0x78, 0, pushj,  0,        "js %d" },
  { 0x79, 0, pushj,  0,        "jns %d" },
  { 0x7a, 0, pushj,  0,        "jp %d" },
  { 0x7b, 0, pushj,  0,        "jnp %d" },
  { 0x7c, 0, pushj,  0,        "jl %d" },
  { 0x7d, 0, pushj,  0,        "jnl %d" },
  { 0x7e, 0, pushj,  0,        "jle %d" },
  { 0x7f, 0, pushj,  0,        "jnle %d" },

  { 0x80, 0, sub80,  0,        0 },
  { 0x81, 0, sub80,     WB,    0 },
  /*
  ** opcode 82 is invalid in 64-bit mode, undocumented otherwise.
  */
  { 0x82, 0, sub80,        SX, 0 }, /* Is this meaningful? */
  { 0x83, 0, sub80,     WB+SX, 0 },
  { 0x84, 0, instb,  EA,       "test %r,%e" },
  { 0x85, 0, instw,  EA+WB,    "test %r,%e" },
  { 0x86, 0, instb,  EA,       "xchg %r,%e" },
  { 0x87, 0, instw,  EA+WB,    "xchg %r,%e" },
  { 0x88, 0, instb,  EA,       "mov %e,%r" },
  { 0x89, 0, instw,  EA+WB,    "mov %e,%r" },
  { 0x8a, 0, instb,  EA,       "mov %r,%e" },
  { 0x8b, 0, instw,  EA+WB,    "mov %r,%e" },
  { 0x8c, 0, instw,  EA+WB,    "mov %e,%s" },
  { 0x8d, 0, inst,   EA+WB,    "lea %r,%e" }, /* WB => 16-bit reg. */
  { 0x8e, 0, instw,  EA+WB,    "mov %s,%e" },
  { 0x8f, 0, instw,  EA+WB,    "pop %e" },

  { 0x90, 0, inst,   0,        "nop" },
  { 0x91, 0, inst,   WB,       "xchg %$,$cx" },
  { 0x92, 0, inst,   WB,       "xchg %$,$dx" },
  { 0x93, 0, inst,   WB,       "xchg %$,$bx" },
  { 0x94, 0, inst,   WB,       "xchg %$,$sp" },
  { 0x95, 0, inst,   WB,       "xchg %$,$bp" },
  { 0x96, 0, inst,   WB,       "xchg %$,$si" },
  { 0x97, 0, inst,   WB,       "xchg %$,$di" },
  { 0x98, 0, inst,   0,        "cbw" },
  { 0x99, 0, inst,   0,        "cwd" },
  { 0x9a, 0, pushj,  X64,      "call %:" },
  { 0x9b, 0, inst,   0,        "wait" },
  { 0x9c, 0, inst,   0,        "pushf" },
  { 0x9d, 0, inst,   0,        "popf" },
  { 0x9e, 0, inst,   X64,      "sahf" },
  { 0x9f, 0, inst,   X64,      "lahf" },

  { 0xa0, 0, instb,  0,        "mov $al,%a" },
  { 0xa1, 0, instw,  WB,       "mov $ax,%a" },
  { 0xa2, 0, instb,  0,        "mov %a,$al" },
  { 0xa3, 0, instw,  WB,       "mov %a,$ax" },
  { 0xa4, 0, inst,   0,        "movsb" },
  { 0xa5, 0, inst,   0,        "movsw" },
  { 0xa6, 0, inst,   0,        "cmpsb" },
  { 0xa7, 0, inst,   0,        "cmpsw" }, /* %s? */
  { 0xa8, 0, inst,   0,        "test $al,%1" },
  { 0xa9, 0, inst,   WB,       "test $ax,%2" },
  { 0xaa, 0, inst,   0,        "stosb" },
  { 0xab, 0, inst,   0,        "stosw" }, /* %s? */
  { 0xac, 0, inst,   0,        "lodsb" },
  { 0xad, 0, inst,   0,        "lodsw" }, /* %s? */
  { 0xae, 0, inst,   0,        "scasb" },
  { 0xaf, 0, inst,   0,        "scasw" }, /* %s? */

  { 0xb0, 0, inst,   0,        "mov %$,%1" },
  { 0xb1, 0, inst,   0,        "mov %$,%1" },
  { 0xb2, 0, inst,   0,        "mov %$,%1" },
  { 0xb3, 0, inst,   0,        "mov %$,%1" },
  { 0xb4, 0, inst,   0,        "mov %$,%1" },
  { 0xb5, 0, inst,   0,        "mov %$,%1" },
  { 0xb6, 0, inst,   0,        "mov %$,%1" },
  { 0xb7, 0, inst,   0,        "mov %$,%1" },
  { 0xb8, 0, inst,   WB,       "mov %$,%2" },
  { 0xb9, 0, inst,   WB,       "mov %$,%2" },
  { 0xba, 0, inst,   WB,       "mov %$,%2" },
  { 0xbb, 0, inst,   WB,       "mov %$,%2" },
  { 0xbc, 0, inst,   WB,       "mov %$,%2" },
  { 0xbd, 0, inst,   WB,       "mov %$,%2" },
  { 0xbe, 0, inst,   WB,       "mov %$,%2" },
  { 0xbf, 0, inst,   WB,       "mov %$,%2" },

  { 0xc0, 1, subC0,  0,        0 }, /* shift/rotate? */
  { 0xc1, 1, subC0,  WB,       0 }, /* shift/rotate? */
  { 0xc2, 0, popj,   0,        "ret %2" },
  { 0xc3, 0, popj,   0,        "ret" },
  { 0xc4, 0, inst,   EA+WB+X64,  "les %r,%e" },
  { 0xc5, 0, inst,   EA+WB+X64,  "lds %r,%e" },
  { 0xc6, 0, instb,  EA+SZ,    "mov %e,%1" },
  { 0xc7, 0, instw,  EA+SZ+WB, "mov %e,%2" },
  { 0xc8, 2, inst,   0,        "enter %2,%1" },
  { 0xc9, 2, inst,   0,        "leave" },
  { 0xca, 0, popj,   0,        "retf %2" },
  { 0xcb, 0, popj,   0,        "retf" },
  { 0xcc, 0, inst,   0,        "int 3" },
  { 0xcd, 0, inst,   0,        "int %1" },
  { 0xce, 0, inst,   X64,      "into" },
  { 0xcf, 0, popj,   0,        "iret" },

  { 0xd0, 0, subD0,  0,        0 },	/* shift/rotate */
  { 0xd1, 0, subD0,  WB,       0 },	/* shift/rotate */
  { 0xd2, 0, subD2,  0,        0 },	/* shift/rotate */
  { 0xd3, 0, subD2,  WB,       0 },	/* shift/rotate */
  { 0xd4, 0, inst,   X64,      "aam %1" },
  { 0xd5, 0, inst,   X64,      "aad %1" },
  { 0xd6, 0, unused, 0,        0 }, /* SALC (undoc.) but not in 64-bit mode */
  { 0xd7, 0, inst,   0,        "xlat" },
  { 0xd8, 0, notyet, 0,        0 },	/* esc */
  { 0xd9, 0, notyet, 0,        0 },	/* esc */
  { 0xda, 0, notyet, 0,        0 },	/* esc */
  { 0xdb, 0, notyet, 0,        0 },	/* esc */
  { 0xdc, 0, notyet, 0,        0 },	/* esc */
  { 0xdd, 0, notyet, 0,        0 },	/* esc */
  { 0xde, 0, notyet, 0,        0 },	/* esc */
  { 0xdf, 0, notyet, 0,        0 },	/* esc */

  { 0xe0, 0, pushj,  0,        "loopnz %d" },
  { 0xe1, 0, pushj,  0,        "loopz %d" },
  { 0xe2, 0, pushj,  0,        "loop %d" },
  { 0xe3, 0, pushj,  0,        "jcxz %d" },
  { 0xe4, 0, inst,   0,        "in $al,%1" },
  { 0xe5, 0, inst,   WB,       "in $ax,%1" },
  { 0xe6, 0, inst,   0,        "out $al,%1" },
  { 0xe7, 0, inst,   WB,       "out $ax,%1" },
  { 0xe8, 0, pushj,  WB,       "call %d" },
  { 0xe9, 0, jrst,   WB,       "jmp %d" },
  { 0xea, 0, jrst,   WB+X64,   "jmp %:" },
  { 0xeb, 0, jrst,   0,        "jmp %d" },
  { 0xec, 0, inst,   0,        "in $al,$cx" },
  { 0xed, 0, inst,   WB,       "in $ax,$cx" },
  { 0xee, 0, inst,   0,        "out $al,$cx" },
  { 0xef, 0, inst,   WB,       "out $ax,$cx" },

  { 0xf0, 0, inst,   0,        "lock" }, /* pref? */
  { 0xf1, 0, unused, 0,        0 },
  { 0xf2, 0, inst,   0,        "rep" },	/* pref? */
  { 0xf3, 0, inst,   0,        "repz" }, /* pref? */
  { 0xf4, 0, inst,   0,        "hlt" },
  { 0xf5, 0, inst,   0,        "cmc" },
  { 0xf6, 0, subF6,  0,        0 },
  { 0xf7, 0, subF6,  WB,       0 },
  { 0xf8, 0, inst,   0,        "clc" },
  { 0xf9, 0, inst,   0,        "stc" },
  { 0xfa, 0, inst,   0,        "cli" },
  { 0xfb, 0, inst,   0,        "sti" },
  { 0xfc, 0, inst,   0,        "cld" },
  { 0xfd, 0, inst,   0,        "std" },
  { 0xfe, 0, subFE,  0,        0 },
  { 0xff, 0, subFF,  WB,       0 },
};

/*
** Temporary, until we do handle rex as a prefix.
*/

static dispblock rxdisp[] = {
  { 0x40, 0, inst,   0,       "rex" },
  { 0x41, 0, inst,   0,       "rex.b" },
  { 0x42, 0, inst,   0,       "rex.x" },
  { 0x43, 0, inst,   0,       "rex.xb" },
  { 0x44, 0, inst,   0,       "rex.r" },
  { 0x45, 0, inst,   0,       "rex.rb" },
  { 0x46, 0, inst,   0,       "rex.rx" },
  { 0x47, 0, inst,   0,       "rex.rxb" },
  { 0x48, 0, inst,   0,       "rex.w" },
  { 0x49, 0, inst,   0,       "rex.wb" },
  { 0x4a, 0, inst,   0,       "rex.wx" },
  { 0x4b, 0, inst,   0,       "rex.wxb" },
  { 0x4c, 0, inst,   0,       "rex.wr" },
  { 0x4d, 0, inst,   0,       "rex.wrb" },
  { 0x4e, 0, inst,   0,       "rex.wrx" },
  { 0x4f, 0, inst,   0,       "rex.wrxb" },
  { 0x00, 0, arnold, 0,        0 },
};

static dispblock v25disp[] = {
  { 0x9c, 0, inst,   0,        "btclr %1,%1,%d" },
  /* btclr <spc-fnc-reg>, <bit-num>, <disp-8> */
  { 0x25, 0, inst,   0,        "movspa" },
  { 0x2d, 0, inst,   WB,       "brkcs %v" },
  { 0x91, 0, inst,   0,        "retrbi" },
  { 0x92, 0, inst,   0,        "fint" },
  { 0x94, 0, inst,   WB,       "tsksw %v" },
  { 0x95, 0, inst,   WB,       "movspb %v" },
  { 0x9e, 0, inst,   0,        "stop" },
  { 0x00, 0, arnold, 0,        0 },
};

static dispblock pfx0Fdisp[] = {
  { 0x00, 0, sub00,  0,        0 },
  { 0x01, 0, sub01,  0,        0 },
  { 0x02, 2, inst,   EA,       "lar %r,%e" },
  { 0x03, 2, inst,   EA+WB,    "lsl %r,%e" },
  /* 0f 05 = syscall (64-bit mode) */
  { 0x06, 2, inst,   0,        "clts" },
  /* 0f 07 = sysret (64-bit mode) */
  { 0x08, 4, inst,   0,        "invd" },
  { 0x09, 4, inst,   0,        "wbinvd" },
  { 0x0b, 3, inst,   0,        "ud2" },

  /* seems like 0f 1f /0 is a multi-byte noop... */

  /*
  ** 21
  ** 23  mov to/from debug regs
  */

  { 0x30, 5, inst,   0,        "wrmsr" },
  { 0x31, 5, inst,   0,        "rdtsc" },
  { 0x32, 5, inst,   0,        "rdmsr" },
  { 0x33, 5, inst,   0,        "rdpmc" },
  { 0x34, 5, inst,   X64,      "sysenter" },
  { 0x35, 5, inst,   X64,      "sysexit" },

  /*
  ** format: "opcode %r,%e"
  ** introduced in P6 family.
  **
  ** 40 cmovo
  ** 41 cmovno
  ** 42 cmovb/cmovnae
  ** 43 cmovae/cmovnb/cmovnc
  ** 44 cmove/cmovz
  ** 45 cmovne/cmovnz
  ** 46 cmovbe/cmovna
  ** 47 cmova/cmovnbe
  ** 48 cmovs
  ** 49 cmovns
  ** 4a cmovp/cmovpe
  ** 4b cmovnp/cmovpo
  ** 4c cmovl/cmovnge
  ** 4d cmovge/cmovnl
  ** 4e cmovle/cmovng
  ** 4f cmovg/cmovnle
  */

  /*
  ** 6e, 7e  movd  (move doubleword)
  */

  { 0x77, 5, inst,   0,        "emmx" },

  { 0x80, 3, pushj,  WB,       "jo %d" },
  { 0x81, 3, pushj,  WB,       "jno %d" },
  { 0x82, 3, pushj,  WB,       "jb %d" },
  { 0x83, 3, pushj,  WB,       "jnb %d" },
  { 0x84, 3, pushj,  WB,       "je %d" },
  { 0x85, 3, pushj,  WB,       "jne %d" },
  { 0x86, 3, pushj,  WB,       "jbe %d" },
  { 0x87, 3, pushj,  WB,       "jnbe %d" },
  { 0x88, 3, pushj,  WB,       "js %d" },
  { 0x89, 3, pushj,  WB,       "jns %d" },
  { 0x8a, 3, pushj,  WB,       "jp %d" },
  { 0x8b, 3, pushj,  WB,       "jnp %d" },
  { 0x8c, 3, pushj,  WB,       "jl %d" },
  { 0x8d, 3, pushj,  WB,       "jnl %d" },
  { 0x8e, 3, pushj,  WB,       "jle %d" },
  { 0x8f, 3, pushj,  WB,       "jnle %d" },
  { 0x90, 3, instb,  EA,       "seto %e" },
  { 0x91, 3, instb,  EA,       "setno %e" },
  { 0x92, 3, instb,  EA,       "setb %e" },
  { 0x93, 3, instb,  EA,       "setnb %e" },
  { 0x94, 3, instb,  EA,       "sete %e" },
  { 0x95, 3, instb,  EA,       "setne %e" },
  { 0x96, 3, instb,  EA,       "setbe %e" },
  { 0x97, 3, instb,  EA,       "setnbe %e" },
  { 0x98, 3, instb,  EA,       "sets %e" },
  { 0x99, 3, instb,  EA,       "setns %e" },
  { 0x9a, 3, instb,  EA,       "setp %e" },
  { 0x9b, 3, instb,  EA,       "setnp %e" },
  { 0x9c, 3, instb,  EA,       "setl %e" },
  { 0x9d, 3, instb,  EA,       "setnl %e" },
  { 0x9e, 3, instb,  EA,       "setle %e" },
  { 0x9f, 3, instb,  EA,       "setnle %e" },
  { 0xa2, 5, inst,   0,        "cpuid" },
  { 0xa3, 3, inst,   EA+WB,    "bt %e,%r" },
  { 0xa4, 0, instw,  EA+WB,    "shld %e,%r,%1" },
  { 0xa5, 0, instw,  EA+WB,    "shld %e,%r,$cl" },
  { 0xaa, 5, inst,   0,        "rsm" },
  { 0xab, 3, inst,   EA+WB,    "bts %e,%r" },
  { 0xac, 0, instw,  EA+WB,    "shrd %e,%r,%1" },
  { 0xad, 0, instw,  EA+WB,    "shrd %e,%r,$cl" },
  { 0xae, 4, subAE,  0,        0 },
  { 0xaf, 0, instw,  EA+WB,    "imul %r,%e" },
  { 0xb0, 4, instb,  EA,       "cmpxchg %e,%r" },
  { 0xb1, 4, instw,  EA+WB,    "cmpxchg %e,%r" },
  { 0xb2, 3, inst,   EA+WB,    "lss %r,%e" },
  { 0xb3, 3, inst,   EA+WB,    "btr %e,%r" },
  { 0xb4, 3, inst,   EA+WB,    "lfs %r,%e" },
  { 0xb5, 3, inst,   EA+WB,    "lgs %r,%e" },
  { 0xb6, 3, instb,  EA+WB,    "movzx %r,%e" },	/* SKUM */
  { 0xb7, 3, instw,  EA+WB,    "movzx %r,%e" },	/* SKUM */
  { 0xba, 0, subBA,  0,        0 },
  { 0xbb, 3, instw,  EA+WB,    "btc %e,%r" },
  { 0xbc, 3, instw,  EA+WB,    "bsf %r,%e" },
  { 0xbd, 3, instw,  EA+WB,    "bsr %r,%e" },
  { 0xbe, 3, instb,  EA+WB,    "movsx %r,%e" },	/* SKUM */
  { 0xbf, 3, instw,  EA+WB,    "movsx %r,%e" },	/* SKUM */
  { 0xc0, 4, instb,  EA,       "xadd %e,%r" },
  { 0xc1, 4, instw,  EA+WB,    "xadd %e,%r" },
  /* 0xc3 movnti */
  { 0xc7, 5, notyet, 0,        "cmpxchg8b" }, /* XXX */

  { 0xc8, 4, inst,   WB,       "bswap %$" }, /* XXX reg-in-opcode stuff */
  { 0xc9, 4, inst,   WB,       "bswap %$" },
  { 0xca, 4, inst,   WB,       "bswap %$" },
  { 0xcb, 4, inst,   WB,       "bswap %$" },
  { 0xcc, 4, inst,   WB,       "bswap %$" },
  { 0xcd, 4, inst,   WB,       "bswap %$" },
  { 0xce, 4, inst,   WB,       "bswap %$" },
  { 0xcf, 4, inst,   WB,       "bswap %$" },
  { 0x00, 0, arnold, 0,        0 },
};

/* prefix = 0f 00 */

static dispblock sub00disp[] = {
  { 0000, 2, inst,   WB,       "sldt %e" },
  { 0010, 2, inst,   WB,       "str %e" },
  { 0020, 2, inst,   WB,       "lldt %e" },
  { 0030, 2, inst,   WB,       "ltr %e" },
  { 0040, 2, inst,   WB,       "verr %e" },
  { 0050, 2, inst,   WB,       "verw %e" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = 0f 01 */

static dispblock sub01disp[] = {
  { 0000, 2, inst,   WB,       "sgdt %e" },
  { 0010, 2, inst,   WB,       "sidt %e" },
  { 0020, 2, inst,   WB,       "lgdt %e" },
  { 0030, 2, inst,   WB,       "lidt %e" },
  { 0040, 2, inst,   WB,       "smsw %e" },
  { 0060, 2, inst,   WB,       "lmsw %e" },
  { 0070, 2, inst,   WB,       "invlpg %e" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = 0f ae */

static dispblock subAEdisp[] = {
  { 0030, 4, inst,   WB,       "stmxcsr %e" },
  { 0050, 4, inst,   0,        "lfence" },
  { 0060, 4, inst,   0,        "mfence" },
  { 0070, 4, inst,   WB,       "clflush %e" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = 0f ba */

static dispblock subBAdisp[] = {
  { 0040, 3, inst,   WB,       "bt %r,%i" },
  { 0050, 3, inst,   WB,       "bts %r,%i" },
  { 0060, 3, inst,   WB,       "btr %r,%i" },
  { 0070, 3, inst,   WB,       "btc %r,%i" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = 80/81/82/83 */

static dispblock sub80disp[] = {
  { 0000, 0, inst,   SZ,       "add %e,%i" },
  { 0010, 0, inst,   SZ,       "or %e,%i" },
  { 0020, 0, inst,   SZ,       "adc %e,%i" },
  { 0030, 0, inst,   SZ,       "sbb %e,%i" },
  { 0040, 0, inst,   SZ,       "and %e,%i" },
  { 0050, 0, inst,   SZ,       "sub %e,%i" },
  { 0060, 0, inst,   SZ,       "xor %e,%i" },
  { 0070, 0, inst,   SZ,       "cmp %e,%i" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = c0/c1 */

static dispblock subC0disp[] = {
  { 0000, 0, inst,   SZ,       "rol %e,%1" },
  { 0010, 0, inst,   SZ,       "ror %e,%1" },
  { 0020, 0, inst,   SZ,       "rcl %e,%1" },
  { 0030, 0, inst,   SZ,       "rcr %e,%1" },
  { 0040, 0, inst,   SZ,       "shl %e,%1" },
  { 0050, 0, inst,   SZ,       "shr %e,%1" },
  { 0070, 0, inst,   SZ,       "sar %e,%1" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = d0/d1 */

static dispblock subD0disp[] = {
  { 0000, 0, inst,   SZ,       "rol %e,1" },
  { 0010, 0, inst,   SZ,       "ror %e,1" },
  { 0020, 0, inst,   SZ,       "rcl %e,1" },
  { 0030, 0, inst,   SZ,       "rcr %e,1" },
  { 0040, 0, inst,   SZ,       "shl %e,1" },
  { 0050, 0, inst,   SZ,       "shr %e,1" },
  { 0070, 0, inst,   SZ,       "sar %e,1" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = d2/d3 */

static dispblock subD2disp[] = {
  { 0000, 0, inst,   SZ,       "rol %e,cl" },
  { 0010, 0, inst,   SZ,       "ror %e,cl" },
  { 0020, 0, inst,   SZ,       "rcl %e,cl" },
  { 0030, 0, inst,   SZ,       "rcr %e,cl" },
  { 0040, 0, inst,   SZ,       "shl %e,cl" },
  { 0050, 0, inst,   SZ,       "shr %e,cl" },
  { 0070, 0, inst,   SZ,       "sar %e,cl" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = f6/f7 */

static dispblock subF6disp[] = {
  { 0000, 0, inst,   SZ,       "test %e,%i" },
  { 0020, 0, inst,   SZ,       "not %e" },
  { 0030, 0, inst,   SZ,       "neg %e" },
  { 0040, 0, inst,   SZ,       "mul %e" },
  { 0050, 0, inst,   SZ,       "imul %e" },
  { 0060, 0, inst,   SZ,       "div %e" },
  { 0070, 0, inst,   SZ,       "idiv %e" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = fe */

static dispblock subFEdisp[] = {
  { 0000, 0, instb,  SZ,       "inc %e" },
  { 0010, 0, instb,  SZ,       "dec %e" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = ff */

static dispblock subFFdisp[] = {
  { 0000, 0, instw,  SZ,       "inc %e" },
  { 0010, 0, instw,  SZ,       "dec %e" },
  { 0020, 0, pushj,  0,        "call %e" },
  { 0030, 0, pushj,  0,        "callf %e" },
  { 0040, 0, jrst,   0,        "jmp %e" },
  { 0050, 0, jrst,   0,        "jmp %:" },
  { 0060, 0, instw,  0,        "push %e" },
  { 0000, 0, arnold, 0,        0 },
};

/************************************************************************/

/*
** hex() prints its argument in hexadecimal.
*/

static void hex(longword l)
{
  if (unixflag) {
    if (l > 9) {
      bufstring("0x");
    }
    bufhex(l, 1);
  } else {
    bufhex(l, 0);
    if (l > 9) {
      casechar('h');
    }
  }
}

/*
** octal() is the same as hex(), only radixally challenged.
*/

static void octal(longword l)
{
  if (l > 7) {
    bufchar('0');
  }
  bufoctal(l, 1);
}

/*
** number() prints out a number, according to the selected radix.  If the
** selected radix is something we don't support, use the default, i.e. hex.
*/

static void number(longword l)
{
  switch (radix) {
  case 8:
    octal(l);
    break;
  case 10:
    bufdecimal(l, 1);
    break;
  case 16:
  default:
    hex(l);
    break;
  }
}

/*
** signum() outputs a signed number.
*/

static void signum(long l)
{
  if (l < 0) {
    bufchar('-');
    l = -l;
  }
  number(l);
}

/*
** This routine will generate a label on the specified address, unless
** one already exist.
*/

static void genlabel(address* addr)
{
  char work[10];

  if (!l_exist(addr)) {
    sprintf(work, "L%05" PRIxl, (a_a2l(addr) & 0xfffff));
    while(l_lookup(work) != NULL) {
      sprintf(work, "L%" PRIxl, uniq());
    }
    l_insert(addr, work);
  }
}

/*
** usselect() outputs its first or second string argument, depending on the
** state of the unix syntax flag.
*/

static void usselect(char* nstr, char* ustr)
{
  if (unixflag)
    casestring(ustr);
  else
    casestring(nstr);
}


/*
** ucselect() is the single-char version of usselect().
*/

static void ucselect(char nchr, char uchr)
{
  if (unixflag)
    casechar(uchr);
  else
    casechar(nchr);
}

/*
** dobyte() will output the current item as a byte of data.
*/

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  usselect("db", ".byte");
  spacedelim();
  number(b);
  checkblank();
}

static void i8086_dobyte(void)
{
  defb(getbyte());
}

/*
** dochar() will try to output its argument as a character.  If it can't,
** it will default to dobyte().
*/

static void i8086_dochar(void)
{
  byte b;

  b = getmemory(istart);
  if (printable(b)) {
    pb_length = 1;
    startline(true);
    usselect("db", ".byte");
    spacedelim();
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

/*
** doword() will output the current item as a word (16 bits) of data.
*/

static void i8086_doword(void)
{
  word w;

  w = getword();
  pb_length = 2;
  startline(true);
  usselect("dw", ".word");
  spacedelim();
  number(w);
  checkblank();
}

/*
** dolong() will output the current item as a longword (32 bits) of data.
*/

static void i8086_dolong(void)
{
  longword l;

  l = getlong();
  pb_length = 4;
  startline(true);
  usselect("dd", ".long");
  spacedelim();
  number(l);
  checkblank();
}

/*
** doptr() tries to output its argument as a pointer, i.e. if there is
** a label that corresponds to the argument, it will be used, otherwise
** we will setter for a numeric value.
*/

static void i8086_doptr(void)
{
  longword l;
  address* a;

  if (ptrlen == 2) {
    l = getword();
  } else {
    l = getlong();
  }
  a = a_l2a(l);
  if (l_exist(a)) {
    if (ptrlen == 2) {
      pb_length = 2;
      startline(true);
      usselect("dw", ".word");
    } else {
      pb_length = 4;
      startline(true);
      usselect("dd", ".long");
    }
    spacedelim();
    bufstring(l_find(a));
    checkblank();
  } else {
    if (ptrlen == 2) {
      pb_length = 2;
      startline(true);
      usselect("dw", ".word");
      spacedelim();
      number(l);
    } else {
      pb_length = 4;
      startline(true);
      usselect("dd", ".long");
      spacedelim();
      number(l);
    }
    checkblank();
  }
}

/*
** dotext() will try to decode a text constant.
*/

static void i8086_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    i8086_dochar();
  } else {
    startline(true);
    usselect("db", ".ascii");
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
** Instruction decoding.  (yuk!)  The reason this code looks like a
** dungheap is that the processor is one...
*/

static byte eabyte;		/* EA byte, complete. */
static byte eamode;		/* EA mode bits. */
static byte eareg;		/* EA reg field. */
static byte earm;		/* EA r/m field. */

static byte siscale;		/* SI scale factor. */
static byte siindex;		/* SI index reg. */
static byte sibase;		/* SI base reg. */

static bool eawb;		/* EA is 16 (or 32) bits. */
static bool easx;		/* Sign-extend 8 -> 16 for imm. data. */
static bool easz;		/* EA needs explicite size. */

static bool ea32d;		/* Protected mode or 32-bit data prefix. */
static bool ea32a;		/* Protected mode or 32-bit addr prefix. */

static byte easeg;		/* Explicit segment prefix, or zero. */

static byte rexbits;		/* Bits from REX prefix, if any. */

static byte opcbyte;		/* Opcode byte, found by dodisp(). */

static address* touch;		/* What this instruction references. */

static void keepea(byte b)
{
  byte sibyte;

  eabyte = b;			/* Keep the whole byte. */
  eamode = (b & 0300) >> 6;	/* Keep mode bits. */
  eareg = (b & 0070) >> 3;	/* Keep reg field. */
  earm = b & 0007;		/* Keep r/m field. */

  sibyte = 0;
  if (earm == 4 && eamode <= 2 && ea32a) {
    sibyte = getbyte();
    pb_length += 1;
  }

  siscale = (sibyte >> 6) & 3;
  siindex = (sibyte >> 3) & 7;
  sibase = sibyte & 7;
}

static void emitsize(void)
{
  if (easz) {
    if (eawb) {
      if (ea32d) {
	casechar('d');
      }
      casestring("word ptr ");
    } else {
      casestring("byte ptr ");
    }
  }
}

static void emitpfx(void)
{
  switch (easeg) {
    case 0x26: casestring("es:"); break;
    case 0x2e: casestring("cs:"); break;
    case 0x36: casestring("ss:"); break;
    case 0x3e: casestring("ds:"); break;
    case 0x64: casestring("fs:"); break;
    case 0x65: casestring("gs:"); break;
  }
}

static void emitaddr(void)
{
  longword l;
  int sreg;			/* Segment (yuk) register. */

  if (ea32a) {
    l = getlong();
  } else {
    l = getword();
  }

  switch (easeg) {
    case 0x26: sreg = esindex; break;
    case 0x2e: sreg = csindex; break;
    case 0x36: sreg = ssindex; break;
    case 0x3e: sreg = dsindex; break;
    case 0x64: sreg = fsindex; break;
    case 0x65: sreg = gsindex; break;
    default: sreg = dsindex; break;
  }

  if (protmode || r_isdef(sreg, istart)) {
    if (!protmode) {
      l += (v_v2l(r_read(sreg, istart))) << 4;
    }
    touch = a_l2a(l);
    reference(touch);
    if (l_exist(touch)) {
      bufstring(l_find(touch));
    } else {
      if (updateflag) {
	genlabel(touch);
      }
      number(l);
    }
    endref();
  } else {
    number(l);
  }
}

static void emitoffset(byte len)
{
  emitsize();
  emitpfx();
  if (len == 1) {
    signum(sextb(getbyte()));
  }
  if (len == 2) {
    emitaddr();
  }
  bufchar('[');
  switch (earm) {
    case 0: casestring("bx+si"); break;
    case 1: casestring("bx+di"); break;
    case 2: casestring("bp+si"); break;
    case 3: casestring("bp+di"); break;
    case 4: casestring("si"); break;
    case 5: casestring("di"); break;
    case 6: casestring("bp"); break;
    case 7: casestring("bx"); break;
  }
  bufchar(']');
}

static void e32reg(byte reg)
{
  static char* r32[8] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
  };

  if (unixflag) {
    bufchar('%');
  }
  casestring(r32[reg & 007]);
}

static void emitsi(void)
{
  if (sibase != 5 || eamode != 0) {
    e32reg(sibase);
  }
  if (siindex != 4) {
    bufchar(',');
    e32reg(siindex);
    if (siscale != 0) {
      ucselect('*', ',');
      number(1 << siscale);
    }
  }
}

static void e32offset(byte len)
{
  emitsize();
  emitpfx();
  if (len == 0 && earm == 4 && sibase == 5) {
    signum(sextl(getlong()));
  }
  if (len == 1) {
    signum(sextb(getbyte()));
  }
  if (len == 2) {
    signum(sextl(getlong()));
  }
  if (unixflag) {
    bufchar('(');
  } else {
    bufchar('[');
  }
  if (earm == 4) {
    emitsi();
  } else {
    e32reg(earm);
  }
  if (!unixflag) {
    bufchar(']');
  } else {
    bufchar(')');
  }
}

static void emitreg(byte reg)
{
  static char* r8[8] =  { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
  static char* r16[8] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };

  if (unixflag) {
    bufchar('%');
  }
  if (eawb) {
    if (ea32d) {
      casechar('e');
    }
    casestring(r16[reg & 007]);
  } else {
    casestring(r8[reg & 007]);
  }
}

static void emitcolon(void)
{
  word segment, offset;

  offset = getword();
  segment = getword();

  number(segment);
  bufchar(':');
  number(offset);
}

static void emitea(void)
{
  if (ea32a) {
    if ((eamode == 0) && (earm == 5)) {
      emitsize();
      emitpfx();
      if (unixflag) {
	emitaddr();
      } else {
	bufchar('['); emitaddr(); bufchar(']');
      }
    } else {
      switch (eamode) {
	case 0: e32offset(0); break;
	case 1: e32offset(1); break;
	case 2: e32offset(2); break;
	case 3: emitreg(earm); break;
      }
    }
  } else {
    if ((eamode == 0) && (earm == 6)) {
      emitsize();
      emitpfx();
      bufchar('['); emitaddr(); bufchar(']');
    } else {
      switch (eamode) {
	case 0: emitoffset(0); break;
	case 1: emitoffset(1); break;
	case 2: emitoffset(2); break;
	case 3: emitreg(earm); break;
      }
    }
  }
}

static void emitdisp(void)
{
  long offset;
  longword pos;
  longword disp;

  if (eawb) {
    if (ea32a) {
      disp = getlong();
      offset = sextl(disp);
    } else {
      disp = getword();
      offset = sextw(disp);
    }
  } else {
    disp = getbyte();
    if (disp >= 0x80) {
      disp += 0xff00;
    }
    offset = sextw(disp);
  }

  pos = a_a2l(pc);

  if (ea32a) {
    touch = a_l2a((pos + offset) & 0xffffffff);
  } else {
    touch = a_l2a((pos + offset) & 0xffff);
  }

/* * * * * * * * * * * * */

  reference(touch);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    if (updateflag) {
      genlabel(touch);
    }
    offset += pb_length;	/* pb_length == pc - istart */
    bufchar('.');
    if (offset < 0) {
      bufchar('-'); number(-offset);
    }
    if (offset > 0) {
      bufchar('+'); number(offset);
    }
  }
  endref();
}

static void emitimm(void)
{
  if (unixflag)
    bufchar('$');

  if (eawb && !easx) {
    if (ea32d) {
      number(getlong());
    } else {
      number(getword());
    }
  } else {
    signum(sextb(getbyte()));
  }
}

static void emitseg(byte reg)
{
  static char* regname[4] = { "es", "cs", "ss", "ds" };

  /* handle fs & gs too. */
  /* handle 32-bit mode. */

  casestring(regname[reg & 3]);
}

static void emitv25(void)
{
  emitreg(getbyte());
}

/* Compute length of a displacement. */

static void dcl(void)
{
  if (eawb) {
    if (ea32a) {
      pb_length += 4;
    } else {
      pb_length += 2;
    }
  } else {
    pb_length += 1;
  }
}

/* Compute length of an effective address. */

static void ecl(void)
{
  if (ea32a) {
    switch (eamode) {
    case 0:
      if (earm == 4 && sibase == 5) {
	pb_length += 4;
      }
      if (earm == 5) {
	pb_length += 4;
      }
      break;
    case 1:
      pb_length += 1;
      break;
    case 2:
      pb_length += 4;
      break;
    }
  } else {
    if ((eamode == 0) && (earm == 6)) {
      pb_length += 2;
    } else {
      switch (eamode) {
	case 1: pb_length += 1; break;
	case 2: pb_length += 2; break;
      }
    }
  }
}

/* Compute length of an immediate constant. */

static void icl(void)
{
  if (eawb && !easx) {
    if (ea32d) {
      pb_length += 4;
    } else {
      pb_length += 2;
    }
  } else {
    pb_length += 1;
  }
}

static bool computelength(char* expand)
{
  char c;

  while ((c = *(expand++)) != (char) 0) {
    if (c == '%') {
      c = *(expand++);
      switch(c) {
      case '1': pb_length += 1; break;
      case '2':
	if (ea32d) {
	  pb_length += 4;
	} else {
	  pb_length += 2;
	}
	break;
      case ':': pb_length += 4; break; /* Hrlzm, even in p-mode? */
      case 'a': 
	/*
	 * In 64-bit mode, this is supposed to be a 64-bit address.
	 */
	if (ea32a)
	  pb_length += 4;
	else
	  pb_length += 2;
	break;
      case 'd': dcl(); break;
      case 'e': ecl(); break;
      case 'i': icl(); break;
      case 'v': pb_length += 1; break;
      }
    } 
  }

  /* Check for overrun here. */

  if (overrun()) {
    return false;
  }
  return true;			/* Or not, if we choose. */
}

static void copytext(char* expand)
{
  char c;
  bool delaycomma = false;
  longword arg;

  while ((c = *(expand++)) != (char) 0) {
    if (c == '%') {
      c = *(expand++);
      switch(c) {
      case '1':
	/*
	** Check argstatus here, for possible character constant.
	*/
	arg = getbyte();

	if (unixflag) {
	  bufchar('$');
	}
	if (argstatus == st_char && printable(arg)) {
	  bufchar('\'');
	  bufchar(arg);
	  bufchar('\'');
	} else {
	  number(arg);
	}
	break;
      case '2':
	/*
	** Check argstatus here, for possible pointer.
	*/
	if (ea32d) {
	  arg = getlong();
	} else {
	  arg = getword();
	}
	if (unixflag) {
	  bufchar('$');
	}
	if (argstatus == st_char && arg < 256 && printable(arg)) {
	  bufchar('\'');
	  bufchar(arg);
	  bufchar('\'');
	} else if (argstatus == st_ptr) {
	  touch = a_l2a(arg);
	  reference(touch);
	  if (l_exist(touch)) {
	    bufstring(l_find(touch));
	  } else {
	    number(arg);
	  }
	  endref();
	} else {
	  number(arg);
	}
	break;
      case ':':
	emitcolon();
	break;
      case '$':			/* embedded register number in opcode. */
	emitreg(opcbyte & 0x07);
	break;
      case 'a':
	if (unixflag) {
	  emitaddr();
	} else {
	  emitpfx();
	  bufchar('[');
	  emitaddr();
	  bufchar(']');
	}
	break;
      case 'd':
	emitdisp();
	break;
      case 'e':
	emitea();
	break;
      case 'i':
	emitimm();
	break;
      case 'r':
	emitreg(eareg);
	break;
      case 's':
	emitseg(eareg);
	break;
      case 'v':
	emitv25();
	break;
      }
    } else if (c == '$') {
      if (unixflag) {
	bufchar('%');
      }
      if (ea32d && eawb) {
	casechar('e');
      }
    } else if (c == ' ') {
      if (unixflag && easz) {
	if (eawb) {
	  if (ea32d)
	    casechar('l');
	  else
	    casechar('w');
	} else {
	  casechar('b');
	}
	easz = false;
      }
      spacedelim();
      if (unixflag) {
	bufhold(true);
      }
    } else if (c == ',') {
      if (unixflag) {
	if (delaycomma) {
	  argdelim(",");
	  bufunhold();
	  argdelim(",");
	  delaycomma = false;
	} else {
	  bufhold(false);
	  delaycomma = true;
	}
      } else {
	argdelim(",");
      }
    } else {
      casechar(c);
    }
  }
  if (unixflag) {
    if (delaycomma) {
      argdelim(",");
    }
    bufunhold();
  }
}

static dispblock* checkflags(dispblock* disp)
{
  if (pb_length > 15) {
    return NULL;
  }
  if (disp == NULL) {
    return NULL;
  }
  if (disp->length > cputype) {
    return NULL;
  }

  /*
   * Check the RX (rex-prefix) flag here, if in 64-bit mode.
   */

  if (x64mode && disp->flags & RX) {
    return finddisp(disp->opcode, rxdisp, NULL);
  }

  switch (disp->itype) {
  case pfxseg:
    if (easeg != 0x00) {
      return NULL;
    }
    easeg = disp->opcode;
    pb_length += 1;
    return checkflags(&maindisp[getbyte()]);
  case pfx32a:
    ea32a = !ea32a;		/* Flip flag. */
    pb_length += 1;
    return checkflags(&maindisp[getbyte()]);
  case pfx32d:
    ea32d = !ea32d;		/* Flip flag. */
    pb_length += 1;
    return checkflags(&maindisp[getbyte()]);
  }

  if (disp->flags & EA) {
    keepea(getbyte());
    pb_length += 1;
  }
  if (disp->flags & WB) {
    eawb = true;
  }
  if (disp->flags & SX) {
    easx = true;
  }
  if (disp->flags & SZ) {
    easz = true;
  }
  return disp;
}

static dispblock* dosub(dispblock tbl[])
{
  byte b;
  dispblock* disp;

  b = getbyte();
  pb_length += 1;
  keepea(b & 0307);		/* Keep mode bits and r/m field. */
  disp = finddisp((b & 0070), tbl, NULL);
  return checkflags(disp);
}

static dispblock* dodisp(dispblock* disp)
{
  pb_length += 1;

  disp = checkflags(disp);

  if (disp == NULL) {
    return NULL;
  }
  
  /* Save opcode here (from disp->opcode), so we can retrieve
   * embedded register number from it, if needed later.
   */

  opcbyte = disp->opcode;

  switch (disp->itype) {
    case unused: return NULL;
    case pfx0F:  return dodisp(finddisp(getbyte(), pfx0Fdisp, NULL));
    case sub00:  return dosub(sub00disp); /* op = 0f 00 */
    case sub01:  return dosub(sub01disp); /* op = 0f 01 */
    case subAE:  return dosub(subAEdisp); /* op = 0f ae */
    case subBA:  return dosub(subBAdisp); /* op = 0f ba */
    case sub80:  return dosub(sub80disp); /* op = 80-83 */
    case subC0:  return dosub(subC0disp); /* op = c0-c1 */
    case subD0:  return dosub(subD0disp); /* op = d0-d1 */
    case subD2:  return dosub(subD2disp); /* op = d2-d3 */
    case subF6:  return dosub(subF6disp); /* op = f6-f7 */
    case subFE:  return dosub(subFEdisp); /* op = fe */
    case subFF:  return dosub(subFFdisp); /* op = ff */
  }
  return disp;
}

static void i8086_doinstr(void)
{
  dispblock* disp;
  
  keepea(0);			/* Reset EA fields. */

  eawb = false;			/* Reset flags: */
  easx = false;
  easz = false;

  ea32a = protmode;
  ea32d = protmode;
  easeg = 0x00;			/* No explicit segment. */

  rexbits = 0;			/* No REX prefix bits (yet). */

  touch = NULL;			/* Don't touch. */
  
  pb_length = 0;		/* Initial instruction length. */

  disp = dodisp(&maindisp[getbyte()]);

  if (disp == NULL) {
    defb(getmemory(istart));
    pb_status = st_none;
    pb_deadend = true;
    return;
  }

  if (!computelength(disp->expand)) {
    defb(getmemory(istart));
    pb_deadend = true;
    return;
  }

  startline(true);

  copytext(disp->expand);

  switch (disp->itype) {
  case instb:
    suggest(touch, st_byte, 1);
    break;
  case instw:			/* Word is either 16 or 32 bits... */
    if (ea32d) {
      suggest(touch, st_long, 4); /* 32-bit. */
    } else {
      suggest(touch, st_word, 2); /* 16-bit. */
    }
    break;
  case inst:
    /* Here we should handle touching data from i.e. sub80 class. */
    break;
  case pushj:
    pb_detour = touch;
    break;
  case popj:
    pb_deadend = true;
    delayblank = true;
    break;
  case jrst:
    pb_detour = touch;
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
    number(a_a2l(a));
    if (c_exist(a)) {
      tabto(32);
      bufchar(';');
      bufstring(c_find(a));
    }
    bufnewline();
  }
}

/************************************************************************/

void i8086_begin(void)
{
  bufstring(";Beginning of program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

void i8086_org(address* a)
{
  bufblankline();
  casestring("org");
  spacedelim();
  bufstring(a_a2str(a));
  bufblankline();
}

void i8086_end(void)
{
  bufblankline();
  bufstring(";End of program");
}

/*
** This routine returns a canonical representation of a label, used for
** looking them up by name in the database.  (Labels are stored and used
** exactly the same way as you type them in).
*/

char* i8086_lcan(char* name)
{
  static char work[10];

  return canonicalize(name, work, 8);
}

/*
** This routine is used to check that a user-specified label string con-
** forms to the syntax for labels for the selected assembler.  Labels can
** have (in all known assemblers) letters and "." in the first position,
** and letters, "." and digits in all but the first.  Check this with a 
** standard helper routine.
*/

bool i8086_lchk(char* name)
{
  return checkstring(name, ".", "0123456789.");
}

/*
** This routine is used to generate labels at specified addresses, from
** the command "SET LABEL <address>", if there is no label given.  In that
** case we make one up.
*/

void i8086_lgen(address* addr)
{
  genlabel(addr);
}

/************************************************************************/

/*
** Common part of init, for all processors.
*/

static void setregs(void)
{
  /* Define registers: */

  csindex = r_define("cs", vty_long);
  dsindex = r_define("ds", vty_long);
  esindex = r_define("es", vty_long);
  ssindex = r_define("ss", vty_long);
  fsindex = 0;
  gsindex = 0;
  if (cputype >= 3) {
    fsindex = r_define("fs", vty_long);
    gsindex = r_define("gs", vty_long);
  }
}

static void setcommon(void)
{
  /* Set up our functions: */
  
  spf_lcan(i8086_lcan);
  spf_lchk(i8086_lchk);
  spf_lgen(i8086_lgen);

  /* set up our object handlers: */
  
  spf_dodef(i8086_dobyte);

  spf_doobj(st_inst, i8086_doinstr);
  spf_doobj(st_byte, i8086_dobyte);
  spf_doobj(st_word, i8086_doword);
  spf_doobj(st_long, i8086_dolong);
  spf_doobj(st_ptr,  i8086_doptr);
  spf_doobj(st_char, i8086_dochar);
  spf_doobj(st_text, i8086_dotext);

  spf_begin(i8086_begin);
  spf_end(i8086_end);
  spf_org(i8086_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 7;			/* Bytes per line, expanded. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = false;		/* We are little-endian. */
  pv_cstart = ";";		/* Default comment start string. */
  protmode = false;		/* Default is no protected mode. */
  ptrlen = 2;			/* For now. */

  x64mode = false;		/* Default is not in 64-bit mode. */

  setregs();			/* Define registers. */
}

/*
** This routine will be called when we select this processor.  It is then
** our job to set up whatever we need in our environment, like telling
** the support routines if we are big- or little-endian.
*/

void i8086_init(void)
{
  cputype = 0;			/* Means 8086/8088 */
  setcommon();			/* Set up register indexes. */
}

void i8088_init(void)
{
  cputype = 0;			/* Means 8086/8088 */
  setcommon();			/* Set up register indexes. */
}

void i80186_init(void)
{
  cputype = 1;			/* Means 80186/80188 */
  setcommon();			/* Set up register indexes. */
}

void i80286_init(void)
{
  cputype = 2;			/* Means 80286 */
  setcommon();			/* Set up register indexes. */
}

void i80386_init(void)
{
  cputype = 3;			/* Means 80386 */
  setcommon();			/* Set up register indexes. */
}

void i80486_init(void)
{
  cputype = 4;			/* Means 80486 */
  setcommon();			/* Set up register indexes. */
}

void i80486p_init(void)
{
  cputype = 4;			/* Means 80486 */
  setcommon();			/* Set up register indexes. */
  protmode = true;		/* Set up protected mode. */
  ptrlen = 4;			/* ... means 32-bit addrs. */
}

void x86_64_init(void)
{
  cputype = 5;			/* Pentium something. */
  setcommon();			/* Set up common things. */
  protmode = true;		/* We use protected mode. Only. */
  ptrlen = 4;			/* ... maybe should be 8? */
  x64mode = true;		/* We are 64-bits. */
}

void v25_init(void)
{
  /* fix this! */
  cputype = 2;			/* Means 80286 */
  setcommon();			/* Set up register indexes. */
}

/*
** This routine returns a help string for this processor, so that the
** user can ask us what the **** we are.  We should give information on
** what the different assemblers mean and all that jazz.
*/

bool i8086_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 8086.\n\
");
    return true;
  }
  return false;
}

bool i8088_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 8088.\n\
");
    return true;
  }
  return false;
}

bool i80186_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80186.\n\
");
    return true;
  }
  return false;
}

bool i80286_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80286.\n\
");
    return true;
  }
  return false;
}

bool i80386_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80386.\n\
");
    return true;
  }
  return false;
}

bool i80486_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80486.\n\
");
    return true;
  }
  return false;
}

bool i80486p_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80486 in protected mode.\n\
");
    return true;
  }
  return false;
}

bool x86_64_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for AMD x86-64.\n\
");
    return true;
  }
  return false;
}

bool v25_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the NEC V25.\n\
");
    return true;
  }
  return false;
}
