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

/*
** Global variables, from module common:
*/

/* configurable variables: */

extern int radix;

/*
** Start of our local variables:
*/

static int cputype;		/* Subtype of processor. */
static bool protmode;		/* Protected mode? */

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
#define sub01  11		/* pfx = 0f 00 */
#define subBA  12		/* pfx = 0f ba */
#define sub80  13		/* pfx = 80-83 */
#define subC0  14		/* pfx = c0-c1 */
#define subD0  15		/* pfx = d0-d1 */
#define subD2  16		/* pfx = d2-d3 */
#define subF6  17		/* pfx = f6-f7 */
#define subFE  18		/* pfx = fe */
#define subFF  19		/* pfx = ff */

#define pfxseg 20		/* segment prefix. */
#define pfx32a 21		/* 32-bit addr prefix. */
#define pfx32d 22		/* 32-bit data prefix. */

#define notyet unused

/* flags: */

#define WB      0x01		/* Word bit flag. */
#define EA      0x02		/* We have a [mod/reg/r-m] word. */
#define SX      0x04		/* Sign-extend imm. data. */
#define SZ      0x08		/* We need to tell the size of EA. */

/* ---- Main opcode table ---- */

static dispblock maindisp[256] = {
  { 0x00, 0, instb,  EA,       "add %e,%r" },
  { 0x01, 0, instw,  EA+WB,    "add %e,%r" },
  { 0x02, 0, instb,  EA,       "add %r,%e" },
  { 0x03, 0, instw,  EA+WB,    "add %r,%e" },
  { 0x04, 0, inst,   0,        "add $al,%1" },
  { 0x05, 0, inst,   0,        "add $ax,%2" },
  { 0x06, 0, inst,   0,        "push $es" },
  { 0x07, 0, inst,   0,        "pop $es" },
  { 0x08, 0, instb,  EA,       "or %e,%r" },
  { 0x09, 0, instw,  EA+WB,    "or %e,%r" },
  { 0x0a, 0, instb,  EA,       "or %r,%e" },
  { 0x0b, 0, instw,  EA+WB,    "or %r,%e" },
  { 0x0c, 0, inst,   0,        "or $al,%1" },
  { 0x0d, 0, inst,   0,        "or $ax,%2" },
  { 0x0e, 0, inst,   0,        "push $cs" },
  { 0x0f, 0, pfx0F,  0,        0 },
  { 0x10, 0, instb,  EA,       "adc %e,%r" },
  { 0x11, 0, instw,  EA+WB,    "adc %e,%r" },
  { 0x12, 0, instb,  EA,       "adc %r,%e" },
  { 0x13, 0, instw,  EA+WB,    "adc %r,%e" },
  { 0x14, 0, inst,   0,        "adc $al,%1" },
  { 0x15, 0, inst,   0,        "adc $ax,%2" },
  { 0x16, 0, inst,   0,        "push $ss" },
  { 0x17, 0, inst,   0,        "pop $ss" },
  { 0x18, 0, instb,  EA,       "sbb %e,%r" },
  { 0x19, 0, instw,  EA+WB,    "sbb %e,%r" },
  { 0x1a, 0, instb,  EA,       "sbb %r,%e" },
  { 0x1b, 0, instw,  EA+WB,    "sbb %r,%e" },
  { 0x1c, 0, inst,   0,        "sbb $al,%1" },
  { 0x1d, 0, inst,   0,        "sbb $ax,%2" },
  { 0x1e, 0, inst,   0,        "push $ds" },
  { 0x1f, 0, inst,   0,        "pop $ds" },
  { 0x20, 0, instb,  EA,       "and %e,%r" },
  { 0x21, 0, instw,  EA+WB,    "and %e,%r" },
  { 0x22, 0, instb,  EA,       "and %r,%e" },
  { 0x23, 0, instw,  EA+WB,    "and %r,%e" },
  { 0x24, 0, inst,   0,        "and $al,%1" },
  { 0x25, 0, inst,   0,        "and $ax,%2" },
  { 0x26, 0, pfxseg, 0,        0 },   /* segment prefix, es: */
  { 0x27, 0, inst,   0,        "daa" },
  { 0x28, 0, instb,  EA,       "sub %e,%r" },
  { 0x29, 0, instw,  EA+WB,    "sub %e,%r" },
  { 0x2a, 0, instb,  EA,       "sub %r,%e" },
  { 0x2b, 0, instw,  EA+WB,    "sub %r,%e" },
  { 0x2c, 0, inst,   0,        "sub $al,%1" },
  { 0x2d, 0, inst,   0,        "sub $ax,%2" },
  { 0x2e, 0, pfxseg, 0,        0 },   /* segment prefix, cs: */
  { 0x2f, 0, inst,   0,        "das" },
  { 0x30, 0, instb,  EA,       "xor %e,%r" },
  { 0x31, 0, instw,  EA+WB,    "xor %e,%r" },
  { 0x32, 0, instb,  EA,       "xor %r,%e" },
  { 0x33, 0, instw,  EA+WB,    "xor %r,%e" },
  { 0x34, 0, inst,   0,        "xor $al,%1" },
  { 0x35, 0, inst,   0,        "xor $ax,%2" },
  { 0x36, 0, pfxseg, 0,        0 },   /* segment prefix, ss: */
  { 0x37, 0, inst,   0,        "aaa" },
  { 0x38, 0, instb,  EA,       "cmp %e,%r" },
  { 0x39, 0, instw,  EA+WB,    "cmp %e,%r" },
  { 0x3a, 0, instb,  EA,       "cmp %r,%e" },
  { 0x3b, 0, instw,  EA+WB,    "cmp %r,%e" },
  { 0x3c, 0, inst,   0,        "cmp $al,%1" },
  { 0x3d, 0, inst,   0,        "cmp $ax,%2" },
  { 0x3e, 0, pfxseg, 0,        0 },   /* segment prefix, ds: */
  { 0x3f, 0, inst,   0,        "aas" },
  { 0x40, 0, inst,   0,        "inc $ax" },
  { 0x41, 0, inst,   0,        "inc $cx" },
  { 0x42, 0, inst,   0,        "inc $dx" },
  { 0x43, 0, inst,   0,        "inc $bx" },
  { 0x44, 0, inst,   0,        "inc $sp" },
  { 0x45, 0, inst,   0,        "inc $bp" },
  { 0x46, 0, inst,   0,        "inc $si" },
  { 0x47, 0, inst,   0,        "inc $di" },
  { 0x48, 0, inst,   0,        "dec $ax" },
  { 0x49, 0, inst,   0,        "dec $cx" },
  { 0x4a, 0, inst,   0,        "dec $dx" },
  { 0x4b, 0, inst,   0,        "dec $bx" },
  { 0x4c, 0, inst,   0,        "dec $sp" },
  { 0x4d, 0, inst,   0,        "dec $bp" },
  { 0x4e, 0, inst,   0,        "dec $si" },
  { 0x4f, 0, inst,   0,        "dec $di" },
  { 0x50, 0, inst,   0,        "push $ax" },
  { 0x51, 0, inst,   0,        "push $cx" },
  { 0x52, 0, inst,   0,        "push $dx" },
  { 0x53, 0, inst,   0,        "push $bx" },
  { 0x54, 0, inst,   0,        "push $sp" },
  { 0x55, 0, inst,   0,        "push $bp" },
  { 0x56, 0, inst,   0,        "push $si" },
  { 0x57, 0, inst,   0,        "push $di" },
  { 0x58, 0, inst,   0,        "pop $ax" },
  { 0x59, 0, inst,   0,        "pop $cx" },
  { 0x5a, 0, inst,   0,        "pop $dx" },
  { 0x5b, 0, inst,   0,        "pop $bx" },
  { 0x5c, 0, inst,   0,        "pop $sp" },
  { 0x5d, 0, inst,   0,        "pop $bp" },
  { 0x5e, 0, inst,   0,        "pop $si" },
  { 0x5f, 0, inst,   0,        "pop $di" },
  { 0x60, 2, inst,   0,        "pusha" },
  { 0x61, 2, inst,   0,        "popa" },
  { 0x62, 2, notyet, 0,        0 },	/* bound */
  { 0x63, 2, notyet, 0,        0 },	/* arpl */
  { 0x64, 3, pfxseg, 0,        0 },     /* segment prefix, fs: */
  { 0x65, 3, pfxseg, 0,        0 },     /* segment prefix, gs: */
  { 0x66, 0, pfx32d, 0,        0 },	/* prefix, register size = 32. */
  { 0x67, 0, pfx32a, 0,        0 },	/* prefix, address size = 32. */
  { 0x68, 0, inst,   0,        "push %2" },
  { 0x69, 1, instw,  EA,       "imul %r,%e,%2" },
  { 0x6a, 0, inst,   0,        "push %1" },
  { 0x6b, 1, instw,  EA+SX,    "imul %r,%e,%1" },
  { 0x6c, 2, inst,   0,        "insb" },
  { 0x6d, 2, inst,   0,        "insw" },
  { 0x6e, 0, inst,   0,        "outsb" },
  { 0x6f, 0, inst,   0,        "outsw" },
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
  { 0x91, 0, inst,   0,        "xchg $ax,$cx" },
  { 0x92, 0, inst,   0,        "xchg $ax,$dx" },
  { 0x93, 0, inst,   0,        "xchg $ax,$bx" },
  { 0x94, 0, inst,   0,        "xchg $ax,$sp" },
  { 0x95, 0, inst,   0,        "xchg $ax,$bp" },
  { 0x96, 0, inst,   0,        "xchg $ax,$si" },
  { 0x97, 0, inst,   0,        "xchg $ax,$di" },
  { 0x98, 0, inst,   0,        "cbw" },
  { 0x99, 0, inst,   0,        "cwd" },
  { 0x9a, 0, pushj,  0,        "call %:" },
  { 0x9b, 0, inst,   0,        "wait" },
  { 0x9c, 0, inst,   0,        "pushf" },
  { 0x9d, 0, inst,   0,        "popf" },
  { 0x9e, 0, inst,   0,        "sahf" },
  { 0x9f, 0, inst,   0,        "lahf" },
  { 0xa0, 0, instb,  0,        "mov $al,[%a]" },
  { 0xa1, 0, instw,  WB,       "mov $ax,[%a]" },
  { 0xa2, 0, instb,  0,        "mov [%a],$al" },
  { 0xa3, 0, instw,  WB,       "mov [%a],$ax" },
  { 0xa4, 0, inst,   0,        "movsb" },
  { 0xa5, 0, inst,   0,        "movsw" },
  { 0xa6, 0, inst,   0,        "cmpsb" },
  { 0xa7, 0, inst,   0,        "cmpsw" },
  { 0xa8, 0, inst,   0,        "test $al,%1" },
  { 0xa9, 0, inst,   0,        "test $ax,%2" },
  { 0xaa, 0, inst,   0,        "stosb" },
  { 0xab, 0, inst,   0,        "stosw" },
  { 0xac, 0, inst,   0,        "lodsb" },
  { 0xad, 0, inst,   0,        "lodsw" },
  { 0xae, 0, inst,   0,        "scasb" },
  { 0xaf, 0, inst,   0,        "scasw" },
  { 0xb0, 0, inst,   0,        "mov $al,%1" },
  { 0xb1, 0, inst,   0,        "mov $cl,%1" },
  { 0xb2, 0, inst,   0,        "mov $dl,%1" },
  { 0xb3, 0, inst,   0,        "mov $bl,%1" },
  { 0xb4, 0, inst,   0,        "mov $ah,%1" },
  { 0xb5, 0, inst,   0,        "mov $ch,%1" },
  { 0xb6, 0, inst,   0,        "mov $dh,%1" },
  { 0xb7, 0, inst,   0,        "mov $bh,%1" },
  { 0xb8, 0, inst,   0,        "mov $ax,%2" },
  { 0xb9, 0, inst,   0,        "mov $cx,%2" },
  { 0xba, 0, inst,   0,        "mov $dx,%2" },
  { 0xbb, 0, inst,   0,        "mov $bx,%2" },
  { 0xbc, 0, inst,   0,        "mov $sp,%2" },
  { 0xbd, 0, inst,   0,        "mov $bp,%2" },
  { 0xbe, 0, inst,   0,        "mov $si,%2" },
  { 0xbf, 0, inst,   0,        "mov $di,%2" },
  { 0xc0, 1, subC0,  0,        0 }, /* shift/rotate? */
  { 0xc1, 1, subC0,  WB,       0 }, /* shift/rotate? */
  { 0xc2, 0, popj,   0,        "ret %2" },
  { 0xc3, 0, popj,   0,        "ret" },
  { 0xc4, 0, inst,   EA+WB,    "les %r,%e" },
  { 0xc5, 0, inst,   EA+WB,    "lds %r,%e" },
  { 0xc6, 0, instb,  EA+SZ,    "mov %e,%1" },
  { 0xc7, 0, instw,  EA+SZ+WB, "mov %e,%2" },
  { 0xc8, 2, inst,   0,        "enter %2,%1" },
  { 0xc9, 2, inst,   0,        "leave" },
  { 0xca, 0, popj,   0,        "retf %2" },
  { 0xcb, 0, popj,   0,        "retf" },
  { 0xcc, 0, inst,   0,        "int 3" },
  { 0xcd, 0, inst,   0,        "int %1" },
  { 0xce, 0, inst,   0,        "into" },
  { 0xcf, 0, popj,   0,        "iret" },
  { 0xd0, 0, subD0,  0,        0 },	/* shift/rotate */
  { 0xd1, 0, subD0,  WB,       0 },	/* shift/rotate */
  { 0xd2, 0, subD2,  0,        0 },	/* shift/rotate */
  { 0xd3, 0, subD2,  WB,       0 },	/* shift/rotate */
  { 0xd4, 0, inst,   0,        "aam %1" },
  { 0xd5, 0, inst,   0,        "aad %1" },
  { 0xd6, 0, unused, 0,        0 },
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
  { 0xe5, 0, inst,   0,        "in $ax,%1" },
  { 0xe6, 0, inst,   0,        "out $al,%1" },
  { 0xe7, 0, inst,   0,        "out $ax,%1" },
  { 0xe8, 0, pushj,  WB,       "call %d" },
  { 0xe9, 0, jrst,   WB,       "jmp %d" },
  { 0xea, 0, jrst,   0,        "jmp %:" },
  { 0xeb, 0, jrst,   0,        "jmp %d" },
  { 0xec, 0, inst,   0,        "in $al,$cx" },
  { 0xed, 0, inst,   0,        "in $ax,$cx" },
  { 0xee, 0, inst,   0,        "out $al,$cx" },
  { 0xef, 0, inst,   0,        "out $ax,$cx" },
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
  { 0x02, 2, inst,   0,        "lar" },
  { 0x03, 2, inst,   0,        "lsl" },
  { 0x06, 2, inst,   0,        "clts" },
  { 0x08, 4, inst,   0,        "invd" },
  { 0x09, 4, inst,   0,        "wbinvd" },
  { 0x30, 5, inst,   0,        "wrmsr" },
  { 0x32, 5, inst,   0,        "rdmsr" },
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
  { 0xa3, 3, inst,   0,        "bt" },
  /* what about a4/a5 (shld) ? */
  { 0xaa, 5, inst,   0,        "rsm" },
  { 0xab, 3, inst,   0,        "bts" },
  /* what about ac/ad (shrd) ? */
  { 0xb0, 4, instb,  0,        "cmpxchg" },
  { 0xb1, 4, instw,  0,        "cmpxchg" },
  { 0xb2, 3, inst,   0,        "lss" },
  { 0xb3, 3, inst,   0,        "btr" },
  { 0xb4, 3, inst,   0,        "lfs" },
  { 0xb5, 3, inst,   0,        "lgs" },
  { 0xb6, 3, instb,  0,        "movzx" },
  { 0xb7, 3, instw,  0,        "movzx" },
  { 0xba, 0, subBA,  0,        0 },
  { 0xbb, 3, inst,   0,        "btc" },
  { 0xbc, 3, notyet, 0,        "bsf ?" },
  { 0xbd, 3, notyet, 0,        "bsr ?" },
  { 0xbe, 3, instb,  0,        "movsx" },
  { 0xbf, 3, instw,  0,        "movsx" },
  { 0xc0, 4, instb,  0,        "xadd" },
  { 0xc1, 4, instw,  0,        "xadd" },
  { 0xc3, 5, notyet, 0,        "cmpxchg8b" },
  { 0xc8, 4, notyet, 0,        "bswap reg" },
  { 0xc9, 4, notyet, 0,        "bswap reg" },
  { 0xca, 4, notyet, 0,        "bswap reg" },
  { 0xcb, 4, notyet, 0,        "bswap reg" },
  { 0xcc, 4, notyet, 0,        "bswap reg" },
  { 0xcd, 4, notyet, 0,        "bswap reg" },
  { 0xce, 4, notyet, 0,        "bswap reg" },
  { 0xcf, 4, notyet, 0,        "bswap reg" },
  { 0x00, 0, arnold, 0,        0 },
};

/* prefix = 0f 00 */

static dispblock sub00disp[] = {
  { 0000, 2, inst,   0,        "sldt" },
  { 0010, 2, inst,   0,        "ltr" },
  { 0010, 2, inst,   0,        "str" },
  { 0020, 2, inst,   0,        "lldt" },
  { 0040, 2, inst,   0,        "verr" },
  { 0050, 2, inst,   0,        "verw" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = 0f 01 */

static dispblock sub01disp[] = {
  { 0000, 2, inst,   0,        "sgdt" },
  { 0010, 2, inst,   0,        "sidt" },
  { 0020, 2, inst,   0,        "lgdt" },
  { 0030, 2, inst,   0,        "lidt" },
  { 0040, 2, inst,   0,        "smsw" },
  { 0060, 2, inst,   0,        "lmsw" },
  { 0070, 2, inst,   0,        "invlpg" },
  { 0000, 0, arnold, 0,        0 },
};

/* prefix = 0f ba */

static dispblock subBAdisp[] = {
  { 0040, 3, inst,   0,        "bt" },
  { 0050, 3, inst,   0,        "bts" },
  { 0060, 3, inst,   0,        "btr" },
  { 0070, 3, inst,   0,        "btc" },
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
  { 0050, 0, jrst,   0,        "jmp %e" },
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
  }
  bufhex(l, 0);
  if (!unixflag) {
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
    sprintf(work, "L%05x", (a_a2l(addr) & 0xfffff));
    while(l_lookup(work) != nil) {
      sprintf(work, "L%x", uniq());
    }
    l_insert(addr, work);
  }
}

/*
** dobyte() will output the current item as a byte of data.
*/

static void dobyte(byte b) {
  pb_length = 1;
  startline(true);
  casestring("db");
  spacedelim();
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

static void dochar(byte b) {
  if (printable(b)) {
    pb_length = 1;
    startline(true);
    casestring("db");
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

static void doword(word w) {
  pb_length = 2;
  startline(true);
  casestring("dw");
  spacedelim();
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
** dolong() will output the current item as a longword (32 bits) of data.
*/

static void dolong(longword l)
{
  pb_length = 4;
  startline(true);
  casestring("dd");
  spacedelim();
  number(l);
  if (pb_actual == st_long) {
    while (getstatus(pc) == st_cont) {
      bufstring(", ");
      number(getlong());
      pb_length += 4;
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

  a = a_l2a(l);
  if (l_exist(a)) {
    if (ptrlen == 2) {
      pb_length = 2;
      startline(true);
      casestring("dw");
    } else {
      pb_length = 4;
      startline(true);
      casestring("dd");
    }
    spacedelim();
    bufstring(l_find(a));
    checkblank();
  } else {
    if (ptrlen == 2) {
      doword(l);
    } else {
      dolong(l);
    }
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
    casestring("db");
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

static bool eawb;		/* EA is 16 (or 32) bits. */
static bool easx;		/* Sign-extend 8 -> 16 for imm. data. */
static bool easz;		/* EA needs explicite size. */

static bool ea32d;		/* Protected mode or 32-bit data prefix. */
static bool ea32a;		/* Protected mode or 32-bit addr prefix. */

static byte easeg;		/* Explicit segment prefix, or zero. */

static address* touch;		/* What this instruction references. */

static void keepea(byte b)
{
  eabyte = b;			/* Keep the whole byte. */
  eamode = (b & 0300) >> 6;	/* Keep mode bits. */
  eareg = (b & 0070) >> 3;	/* Keep reg field. */
  earm = (b & 0007);		/* Keep r/m field. */
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

static void emitoffset(byte len)
{
  emitsize();
  emitpfx();
  if (len == 1) {
    signum(sextb(getbyte()));
  }
  if (len == 2) {
    number(getword());
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

static void e32offset(byte len)
{
  emitsize();
  emitpfx();
  if (len == 1) {
    number(getbyte());
  }
  if (len == 2) {
    number(getlong());
  }
  bufchar('[');
  switch (earm) {
    case 0: casestring("eax"); break;
    case 1: casestring("eac"); break;
    case 2: casestring("ead"); break;
    case 3: casestring("eab"); break;
    case 4: casestring("<scaled index>"); break;
    case 5: casestring("ebp"); break;
    case 6: casestring("esi"); break;
    case 7: casestring("edi"); break;
  }
  bufchar(']');

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

/**** 32-bit mode version of effective address bytes:

  mod  reg  r/m  seg what

   00  ---  000      [eax]
   00  ---  001      [ecx]
   00  ---  010      [edx]
   00  ---  011      [ebx]
   00  000  100      [eax + sc.i.]
   00  001  100      [ecx + sc.i.]
   00  010  100      [edx + sc.i.]
   00  011  100      [ebx + sc.i.]
   00  100  100  ss: [esp + sc.i.]
   00  101  100      [disp32 + sc.i.]
   00  110  100      [esi + sc.i.]
   00  111  100      [edi + sc.i.]
   00  ---  101      disp32
   00  ---  110      [esi]
   00  ---  111      [edi]

   01  ---  000      [eax + disp8]
   01  ---  001      [ecx + disp8]
   01  ---  010      [edx + disp8]
   01  ---  011      [ebx + disp8]
   01  000  100      [eax + sc.i. + disp8]
   01  001  100      [ecx + sc.i. + disp8]
   01  010  100      [edx + sc.i. + disp8]
   01  011  100      [ebx + sc.i. + disp8]
   01  100  100  ss: [esp + sc.i. + disp8]
   01  101  100  ss: [ebp + sc.i. + disp8]
   01  110  100      [esi + sc.i. + disp8]
   01  111  100      [edi + sc.i. + disp8]
   01  ---  101  ss: [ebp + disp8]
   01  ---  110      [esi + disp8]
   01  ---  111      [edi + disp8]

   10  ---  000      [eax + disp32]
   10  ---  001      [ecx + disp32]
   10  ---  010      [edx + disp32]
   10  ---  011      [ebx + disp32]
   10  000  100      [eax + sc.i. + disp32]
   10  001  100      [ecx + sc.i. + disp32]
   10  010  100      [edx + sc.i. + disp32]
   10  011  100      [ebx + sc.i. + disp32]
   10  100  100  ss: [esp + sc.i. + disp32]
   10  101  100  ss: [ebp + sc.i. + disp32]
   10  110  100      [esi + sc.i. + disp32]
   10  111  100      [edi + sc.i. + disp32]
   10  ---  101  ss: [ebp + disp32]
   10  ---  110      [esi + disp32]
   10  ---  111      [edi + disp32]

   11  ---  ---      dual 32-bit reg?

 scaled index is one byte:
 
   ssiiibbb:
   
   ss = 00/01/10/11 for 1x/2x/4x/8x respectively.
   iii = index register.
   bbb = base register.

 ****/

static void emitcolon(void)
{
  word segment, offset;

  offset = getword();
  segment = getword();

  number(segment);
  bufchar(':');
  number(offset);
}

static void emitaddr(void)
{
  longword l;
  
  if (ea32a) {
    l = getlong();
  } else {
    l = getword();
  }

  /* Here we should check segment pfx's etc. */

  touch = a_l2a(l);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    if (updateflag) {
      genlabel(touch);
    }
    number(l);
  }
}

static void emitea(void)
{
  if (ea32a) {
    if ((eamode == 0) && (earm == 5)) {
      emitsize();
      emitpfx();
      bufchar('['); emitaddr(); bufchar(']');
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
    touch = a_l2a((pos + disp) & 0xffffffff);
  } else {
    touch = a_l2a((pos + disp) & 0xffff);
  }

/* * * * * * * * * * * * */

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
}

static void emitimm(void)
{
  if (eawb && !easx) {
    if (ea32d) {
      number(getlong());
    } else {
      number(getword());
    }
  } else {
    number(getbyte());
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
    /* for now, we ignore scaled index bytes. */
    if ((eamode == 0) && (earm == 5)) {
      pb_length += 4;
    } else {
      switch (eamode) {
	case 1: pb_length += 1; break;
	case 2: pb_length += 4; break;
      }
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
      case 'a': pb_length += 2; break; /* Two or four bytes? */
      case 'd': dcl(); break;
      case 'e': ecl(); break;
      case 'i': icl(); break;
      case 'v': pb_length += 1; break;
      }
    } 
  }

  /* Check for overrun here. */

  if (overrun()) {
    return(false);
  }
  return(true);			/* Or not, if we choose. */
}

static void copytext(char* expand)
{
  char c;

  while ((c = *(expand++)) != (char) 0) {
    if (c == '%') {
      c = *(expand++);
      switch(c) {
	case '1': number(getbyte()); break;
	case '2': if (ea32d) {
		    number(getlong());
		  } else {
		    number(getword());
		  }
		  break;
	case ':': emitcolon(); break;
	case 'a': emitaddr(); break;
	case 'd': emitdisp(); break;
	case 'e': emitea(); break;
	case 'i': emitimm(); break;
	case 'r': emitreg(eareg); break;
	case 's': emitseg(eareg); break;
	case 'v': emitv25(); break;
      }
    } else if (c == '[') {
      emitpfx();
      bufchar('[');
    } else if (c == '$') {
      if (unixflag) {
	bufchar('%');
      }
      if (ea32d) {
	casechar('e');
      }
    } else if (c == ' ') {
      spacedelim();
    } else if (c == ',') {
      argdelim(",");
    } else {
      casechar(c);
    }
  }
}

static dispblock* checkflags(dispblock* disp)
{
  if (disp == nil) {
    return(nil);
  }
  if (disp->length > cputype) {
    return(nil);
  }
  switch (disp->itype) {
  case pfxseg:
    if (easeg != 0x00) {
      return(nil);
    }
    easeg = disp->opcode;
    pb_length += 1;
    return(checkflags(&maindisp[getbyte()]));
  case pfx32a:
    ea32a = !ea32a;		/* Flip flag. */
    pb_length += 1;
    return(checkflags(&maindisp[getbyte()]));
  case pfx32d:
    ea32d = !ea32d;		/* Flip flag. */
    pb_length += 1;
    return(checkflags(&maindisp[getbyte()]));
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
  return(disp);
}

static dispblock* dosub(dispblock tbl[])
{
  byte b;
  dispblock* disp;

  b = getbyte();
  pb_length += 1;
  keepea(b & 0307);		/* Keep mode bits and r/m field. */
  disp = finddisp((b & 0070), tbl);
  return(checkflags(disp));
}

static dispblock* dodisp(dispblock* disp)
{
  pb_length += 1;

  disp = checkflags(disp);

  if (disp == nil) {
    return(nil);
  }
  switch (disp->itype) {
    case unused: return(nil);
    case pfx0F:  return(dodisp(finddisp(getbyte(), pfx0Fdisp)));
    case sub00:  return(dosub(sub00disp)); /* op = 0f 00 */
    case sub01:  return(dosub(sub01disp)); /* op = 0f 00 */
    case subBA:  return(dosub(subBAdisp)); /* op = 0f ba */
    case sub80:  return(dosub(sub80disp)); /* op = 80-83 */
    case subC0:  return(dosub(subC0disp)); /* op = c0-c1 */
    case subD0:  return(dosub(subD0disp)); /* op = d0-d1 */
    case subD2:  return(dosub(subD2disp)); /* op = d2-d3 */
    case subF6:  return(dosub(subF6disp)); /* op = f6-f7 */
    case subFE:  return(dosub(subFEdisp)); /* op = fe */
    case subFF:  return(dosub(subFFdisp)); /* op = ff */
  }
  return(disp);
}

static void doinstr(void)
{
  dispblock* disp;
  
  keepea(0);			/* Reset EA fields. */

  eawb = false;			/* Reset flags: */
  easx = false;
  easz = false;

  ea32a = protmode;
  ea32d = protmode;
  easeg = 0x00;			/* No explicit segment. */

  touch = nil;			/* Don't touch. */
  
  pb_length = 0;		/* Initial instruction length. */

  disp = dodisp(&maindisp[getbyte()]);

  if (disp == nil) {
    dobyte(getmemory(istart));
    pb_status = st_none;
    pb_deadend = true;
    return;
  }

  if (!computelength(disp->expand)) {
    dobyte(getmemory(istart));
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

/*
** This routine handles things like generating the beginning and end of
** the output, as well as "moving" between different segments of memory.
*/

void i8086_spec(address* a, int func)
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
    spacedelim();
    bufstring(a_a2str(a));
    bufblankline();
  }

  if (func == SPC_END) {
    bufblankline();
    bufstring(";End of program");
  }
}

/*
** The main entry is the peek routine.  This should need a minimum of work.
*/

void i8086_peek(stcode prefer)
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
      if (ptrlen == 2) {
	doptr(getword());
      } else {
	doptr(getlong());
      }
      break;
    case st_long:
      dolong(getlong());
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

/*
** This routine returns a canonical representation of a label, used for
** looking them up by name in the database.  (Labels are stored and used
** exactly the same way as you type them in).
*/

char* i8086_lcan(char* name)
{
  static char work[10];

  return(canonicalize(name, work, 8));
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
  return(checkstring(name, ".", "0123456789."));
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
  if (cputype >= 3) {
    fsindex = r_define("fs", vty_long);
    gsindex = r_define("gs", vty_long);
  }
}

static void setcommon(void)
{
  /* Set up our functions: */
  
  spf_peek(i8086_peek);
  spf_spec(i8086_spec);
  spf_lcan(i8086_lcan);
  spf_lchk(i8086_lchk);
  spf_lgen(i8086_lgen);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line, expanded. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = false;		/* We are little-endian. */
  protmode = false;		/* Default is no protected mode. */
  ptrlen = 2;			/* For now. */

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
    return(true);
  }
  return(false);
}

bool i8088_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 8088.\n\
");
    return(true);
  }
  return(false);
}

bool i80186_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80186.\n\
");
    return(true);
  }
  return(false);
}

bool i80286_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80286.\n\
");
    return(true);
  }
  return(false);
}

bool i80386_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80386.\n\
");
    return(true);
  }
  return(false);
}

bool i80486_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80486.\n\
");
    return(true);
  }
  return(false);
}

bool i80486p_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the Intel 80486 in protected mode.\n\
");
    return(true);
  }
  return(false);
}

bool v25_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
This is the help string for the NEC V25.\n\
");
    return(true);
  }
  return(false);
}
