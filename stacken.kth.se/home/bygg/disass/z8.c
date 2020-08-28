/*
** This module implements driver code for the Z8 processor.
*/

#include "disass.h"

/************************************************************************/

evf_init z8_init;
evf_help z8_help;

struct entryvector z8_vector = {
  "z8",				/* Name. */
  "Zilog Z8",			/* One-liner. */
  z8_init,			/* Init routine. */
  z8_help,			/* Help routine. */
};

evf_init super8_init;
evf_help super8_help;

struct entryvector super8_vector = {
  "super8",			/* Name. */
  "Zilog Super-8",		/* One-liner. */
  super8_init,			/* Init routine. */
  super8_help,			/* Help routine. */
};

/************************************************************************/

/*
   *0   2       R       OP dst                  [OPC] [dst]
   *1   2       IR      OP dst                  [OPC] [dst]
   *2	2	r/r	OP dst, src		[OPC] [dst|src]
   *3	2	r/Ir	OP dst, src		[OPC] [dst|src]
   *4	3	R/R	OP dst, src		[OPC] [src] [dst]
   *5	3	R/IR	OP dst, src		[OPC] [src] [dst]
   *6	3	R/IM	OP dst, src		[OPC] [dst] [src]
   *7	3	IR/IM	OP dst, src		[OPC] [dst] [src]

   *8	2	r/R	ld %e,%r		[dst|OPC] [src]
   *9	2	R/r	ld %r,%e		[src|OPC] [dst]
   *a	2	RA	djnz %e,dst		[reg|OPC] [off]
   *b	2	RA	jr %c,dst		[cc| OPC] [off]
   *c	2	r/IM	ld %e,%1		[dst|OPC] [src]
   *d	3	DA	jp %c,dst		[cc| OPC] [addrhi] [addrlo]
   *e	1	r	inc %e			[dst|OPC]
*/

/*
   condition codes:

   0000 F             1000 -
   0001 LT            1001 GE
   0010 LE            1010 GT
   0011 ?   (ULE)     1011 ?    (UGT)
   0100 OV            1100 NOV
   0101 MI            1101 PL
   0110 EQ  (Z)       1110 NE   (NZ)
   0111 C   (ULT)     1111 NC   (UGE)

*/

/* dispatch tables: */

#define unused 0		/* Not used. */
#define inst   1		/* Normal instruction. */
#define jrst   2		/* Goto. */
#define pushj  3		/* Call. */
#define popj   4		/* Return. */
#define split  5		/* Sub-table lookup needed. */

#define SUP8     0x01		/* Super-8 alternative. */
#define SAMS     0x02		/* Samsung-8 only instr. */
#define SPEC     0x04		/* Special handling needed. */

static dispblock z8disp[256] = {
  { 0x00, 2, inst,   0,    "dec %+" },
  { 0x01, 2, inst,   0,    "dec %+" },
  { 0x02, 2, inst,   0,    "add %+" },
  { 0x03, 2, inst,   0,    "add %+" },
  { 0x04, 3, inst,   0,    "add %+" },
  { 0x05, 3, inst,   0,    "add %+" },
  { 0x06, 3, inst,   0,    "add %+" },
  { 0x07, 3, inst,   SUP8, "add %+" },

  { 0x08, 2, inst,   0,    "ld %e,%r" },
  { 0x09, 2, inst,   0,    "ld %r,%e" },
  { 0x0a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x0b, 2, inst,   0,    "jr f,%d" },
  { 0x0c, 2, inst,   0,    "ld %e,%1" },
  { 0x0d, 3, inst,   0,    "jp f,%a" },
  { 0x0e, 1, inst,   0,    "inc %e" },
  { 0x0f, 1, split,  0,    0 },

  { 0x10, 2, inst,   0,    "rlc %+" },
  { 0x11, 2, inst,   0,    "rlc %+" },
  { 0x12, 2, inst,   0,    "adc %+" },
  { 0x13, 2, inst,   0,    "adc %+" },
  { 0x14, 3, inst,   0,    "adc %+" },
  { 0x15, 3, inst,   0,    "adc %+" },
  { 0x16, 3, inst,   0,    "adc %+" },
  { 0x17, 3, inst,   SUP8, "adc %+" },

  { 0x18, 2, inst,   0,    "ld %e,%r" },
  { 0x19, 2, inst,   0,    "ld %r,%e" },
  { 0x1a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x1b, 2, pushj,  0,    "jr %c,%d" },
  { 0x1c, 2, inst,   0,    "ld %e,%1" },
  { 0x1d, 3, pushj,  0,    "jp %c,%a" },
  { 0x1e, 1, inst,   0,    "inc %e" },
  { 0x1f, 1, split,  0,    0 },

  { 0x20, 2, inst,   0,    "inc %+" },
  { 0x21, 2, inst,   0,    "inc %+" },
  { 0x22, 2, inst,   0,    "sub %+" },
  { 0x23, 2, inst,   0,    "sub %+" },
  { 0x24, 3, inst,   0,    "sub %+" },
  { 0x25, 3, inst,   0,    "sub %+" },
  { 0x26, 3, inst,   0,    "sub %+" },
  { 0x27, 3, inst,   SUP8, "sub %+" },

  { 0x28, 2, inst,   0,    "ld %e,%r" },
  { 0x29, 2, inst,   0,    "ld %r,%e" },
  { 0x2a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x2b, 2, pushj,  0,    "jr %c,%d" },
  { 0x2c, 2, inst,   0,    "ld %e,%1" },
  { 0x2d, 3, pushj,  0,    "jp %c,%a" },
  { 0x2e, 1, inst,   0,    "inc %e" },
  { 0x2f, 1, split,  0,    0 },

  { 0x30, 2, popj,   0,    "jp @rr%1" }, /* dst = 16 bits, register pair. */
  { 0x31, 2, inst,   SPEC, 0 },
  { 0x32, 2, inst,   0,    "sbc %+" },
  { 0x33, 2, inst,   0,    "sbc %+" },
  { 0x34, 3, inst,   0,    "sbc %+" },
  { 0x35, 3, inst,   0,    "sbc %+" },
  { 0x36, 3, inst,   0,    "sbc %+" },
  { 0x37, 3, inst,   SUP8, "sbc %+" },

  { 0x38, 2, inst,   0,    "ld %e,%r" },
  { 0x39, 2, inst,   0,    "ld %r,%e" },
  { 0x3a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x3b, 2, pushj,  0,    "jr %c,%d" },
  { 0x3c, 2, inst,   0,    "ld %e,%1" },
  { 0x3d, 3, pushj,  0,    "jp %c,%a" },
  { 0x3e, 1, inst,   0,    "inc %e" },
  { 0x3f, 1, split,  0,    0 },

  { 0x40, 2, inst,   0,    "da %+" },
  { 0x41, 2, inst,   0,    "da %+" },
  { 0x42, 2, inst,   0,    "or %+" },
  { 0x43, 2, inst,   0,    "or %+" },
  { 0x44, 3, inst,   0,    "or %+" },
  { 0x45, 3, inst,   0,    "or %+" },
  { 0x46, 3, inst,   0,    "or %+" },
  { 0x47, 3, inst,   SUP8, "or %+" },

  { 0x48, 2, inst,   0,    "ld %e,%r" },
  { 0x49, 2, inst,   0,    "ld %r,%e" },
  { 0x4a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x4b, 2, pushj,  0,    "jr %c,%d" },
  { 0x4c, 2, inst,   0,    "ld %e,%1" },
  { 0x4d, 3, pushj,  0,    "jp %c,%a" },
  { 0x4e, 1, inst,   0,    "inc %e" },
  { 0x4f, 1, split,  0,    0 },

  { 0x50, 2, inst,   0,    "pop %+" },
  { 0x51, 2, inst,   0,    "pop %+" },
  { 0x52, 2, inst,   0,    "and %+" },
  { 0x53, 2, inst,   0,    "and %+" },
  { 0x54, 3, inst,   0,    "and %+" },
  { 0x55, 3, inst,   0,    "and %+" },
  { 0x56, 3, inst,   0,    "and %+" },
  { 0x57, 3, inst,   SUP8, "and %+" },

  { 0x58, 2, inst,   0,    "ld %e,%r" },
  { 0x59, 2, inst,   0,    "ld %r,%e" },
  { 0x5a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x5b, 2, pushj,  0,    "jr %c,%d" },
  { 0x5c, 2, inst,   0,    "ld %e,%1" },
  { 0x5d, 3, pushj,  0,    "jp %c,%a" },
  { 0x5e, 1, inst,   0,    "inc %e" },
  { 0x5f, 1, split,  0,    0 },

  { 0x60, 2, inst,   0,    "com %+" },
  { 0x61, 2, inst,   0,    "com %+" },
  { 0x62, 2, inst,   0,    "tcm %+" },
  { 0x63, 2, inst,   0,    "tcm %+" },
  { 0x64, 3, inst,   0,    "tcm %+" },
  { 0x65, 3, inst,   0,    "tcm %+" },
  { 0x66, 3, inst,   0,    "tcm %+" },
  { 0x67, 3, inst,   SUP8, "tcm %+" },

  { 0x68, 2, inst,   0,    "ld %e,%r" },
  { 0x69, 2, inst,   0,    "ld %r,%e" },
  { 0x6a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x6b, 2, pushj,  0,    "jr %c,%d" },
  { 0x6c, 2, inst,   0,    "ld %e,%1" },
  { 0x6d, 3, pushj,  0,    "jp %c,%a" },
  { 0x6e, 1, inst,   0,    "inc %e" },
  { 0x6f, 1, split,  0,    0 },

  { 0x70, 2, inst,   0,    "push %+" },
  { 0x71, 2, inst,   0,    "push %+" },
  { 0x72, 2, inst,   0,    "tm %+" },
  { 0x73, 2, inst,   0,    "tm %+" },
  { 0x74, 3, inst,   0,    "tm %+" },
  { 0x75, 3, inst,   0,    "tm %+" },
  { 0x76, 3, inst,   0,    "tm %+" },
  { 0x77, 3, inst,   SUP8, "tm %+" },

  { 0x78, 2, inst,   0,    "ld %e,%r" },
  { 0x79, 2, inst,   0,    "ld %r,%e" },
  { 0x7a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x7b, 2, pushj,  0,    "jr %c,%d" },
  { 0x7c, 2, inst,   0,    "ld %e,%1" },
  { 0x7d, 3, pushj,  0,    "jp %c,%a" },
  { 0x7e, 1, inst,   0,    "inc %e" },
  { 0x7f, 1, split,  0,    0 },

  { 0x80, 2, inst,   0,    "decw %r" },
  { 0x81, 2, inst,   0,    "decw @%r" },
  { 0x82, 1, split,  0,    0 },
  { 0x83, 1, split,  0,    0 },
  { 0x84, 1, split,  0,    0 },
  { 0x85, 1, split,  0,    0 },
  { 0x86, 1, split,  0,    0 },
  { 0x87, 1, split,  0,    0 },

  { 0x88, 2, inst,   0,    "ld %e,%r" },
  { 0x89, 2, inst,   0,    "ld %r,%e" },
  { 0x8a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x8b, 2, jrst,   0,    "jr %d" },
  { 0x8c, 2, inst,   0,    "ld %e,%1" },
  { 0x8d, 3, jrst,   0,    "jp %a" },
  { 0x8e, 1, inst,   0,    "inc %e" },
  { 0x8f, 1, inst,   0,    "di" },

  { 0x90, 2, inst,   0,    "rl %+" },
  { 0x91, 2, inst,   0,    "rl %+" },
  { 0x92, 1, split,  0,    0 },
  { 0x93, 1, split,  0,    0 },
  { 0x94, 1, split,  0,    0 },
  { 0x95, 1, split,  0,    0 },
  { 0x96, 1, split,  0,    0 },
  { 0x97, 1, split,  0,    0 },

  { 0x98, 2, inst,   0,    "ld %e,%r" },
  { 0x99, 2, inst,   0,    "ld %r,%e" },
  { 0x9a, 2, pushj,  0,    "djnz %e,%d" },
  { 0x9b, 2, pushj,  0,    "jr %c,%d" },
  { 0x9c, 2, inst,   0,    "ld %e,%1" },
  { 0x9d, 3, pushj,  0,    "jp %c,%a" },
  { 0x9e, 1, inst,   0,    "inc %e" },
  { 0x9f, 1, inst,   0,    "ei" },

  { 0xa0, 2, inst,   0,    "incw %r" },
  { 0xa1, 2, inst,   0,    "incw @%r" },
  { 0xa2, 2, inst,   0,    "cp %+" },
  { 0xa3, 2, inst,   0,    "cp %+" },
  { 0xa4, 3, inst,   0,    "cp %+" },
  { 0xa5, 3, inst,   0,    "cp %+" },
  { 0xa6, 3, inst,   0,    "cp %+" },
  { 0xa7, 3, inst,   SUP8, "cp %+" },

  { 0xa8, 2, inst,   0,    "ld %e,%r" },
  { 0xa9, 2, inst,   0,    "ld %r,%e" },
  { 0xaa, 2, pushj,  0,    "djnz %e,%d" },
  { 0xab, 2, pushj,  0,    "jr %c,%d" },
  { 0xac, 2, inst,   0,    "ld %e,%1" },
  { 0xad, 3, pushj,  0,    "jp %c,%a" },
  { 0xae, 1, inst,   0,    "inc %e" },
  { 0xaf, 1, popj,   0,    "ret" },

  { 0xb0, 2, inst,   0,    "clr %+" },
  { 0xb1, 2, inst,   0,    "clr %+" },
  { 0xb2, 2, inst,   0,    "xor %+" },
  { 0xb3, 2, inst,   0,    "xor %+" },
  { 0xb4, 3, inst,   0,    "xor %+" },
  { 0xb5, 3, inst,   0,    "xor %+" },
  { 0xb6, 3, inst,   0,    "xor %+" },
  { 0xb7, 3, inst,   SUP8, "xor %+" },

  { 0xb8, 2, inst,   0,    "ld %e,%r" },
  { 0xb9, 2, inst,   0,    "ld %r,%e" },
  { 0xba, 2, pushj,  0,    "djnz %e,%d" },
  { 0xbb, 2, pushj,  0,    "jr %c,%d" },
  { 0xbc, 2, inst,   0,    "ld %e,%1" },
  { 0xbd, 3, pushj,  0,    "jp %c,%a" },
  { 0xbe, 1, inst,   0,    "inc %e" },
  { 0xbf, 1, popj,   0,    "iret" },

  { 0xc0, 2, inst,   0,    "rrc %+" },
  { 0xc1, 2, inst,   0,    "rrc %+" },
  { 0xc2, 1, split,  0,    0 },
  { 0xc3, 1, split,  0,    0 },
  { 0xc4, 1, split,  0,    0 },
  { 0xc5, 1, split,  0,    0 },
  { 0xc6, 1, split,  0,    0 },
  { 0xc7, 1, split,  0,    0 },

  { 0xc8, 2, inst,   0,    "ld %e,%r" },
  { 0xc9, 2, inst,   0,    "ld %r,%e" },
  { 0xca, 2, pushj,  0,    "djnz %e,%d" },
  { 0xcb, 2, pushj,  0,    "jr %c,%d" },
  { 0xcc, 2, inst,   0,    "ld %e,%1" },
  { 0xcd, 3, pushj,  0,    "jp %c,%a" },
  { 0xce, 1, inst,   0,    "inc %e" },
  { 0xcf, 1, inst,   0,    "rcf" },

  { 0xd0, 2, inst,   0,    "sra %+" },
  { 0xd1, 2, inst,   0,    "sra %+" },
  { 0xd2, 1, split,  0,    0 },
  { 0xd3, 1, split,  0,    0 },
  { 0xd4, 1, split,  0,    0 },
  { 0xd5, 1, unused, 0,    0 },
  { 0xd6, 1, split,  0,    0 },
  { 0xd7, 1, split,  0,    0 },

  { 0xd8, 2, inst,   0,    "ld %e,%r" },
  { 0xd9, 2, inst,   0,    "ld %r,%e" },
  { 0xda, 2, pushj,  0,    "djnz %e,%d" },
  { 0xdb, 2, pushj,  0,    "jr %c,%d" },
  { 0xdc, 2, inst,   0,    "ld %e,%1" },
  { 0xdd, 3, pushj,  0,    "jp %c,%a" },
  { 0xde, 1, inst,   0,    "inc %e" },
  { 0xdf, 1, inst,   0,    "scf" },

  { 0xe0, 2, inst,   0,    "rr %+" },
  { 0xe1, 2, inst,   0,    "rr %+" },
  { 0xe2, 1, split,  0,    0 },
  { 0xe3, 1, split,  0,    0 },
  { 0xe4, 3, inst,   SPEC, 0 },
  { 0xe5, 3, inst,   SPEC, 0 },
  { 0xe6, 3, inst,   0,    "ld %r,%1" },
  { 0xe7, 3, inst,   SUP8, "ld @%r,%1" },

  { 0xe8, 2, inst,   0,    "ld %e,%r" },
  { 0xe9, 2, inst,   0,    "ld %r,%e" },
  { 0xea, 2, pushj,  0,    "djnz %e,%d" },
  { 0xeb, 2, pushj,  0,    "jr %c,%d" },
  { 0xec, 2, inst,   0,    "ld %e,%1" },
  { 0xed, 3, pushj,  0,    "jp %c,%a" },
  { 0xee, 1, inst,   0,    "inc %e" },
  { 0xef, 1, inst,   0,    "ccf" },

  { 0xf0, 2, inst,   0,    "swap %+" },
  { 0xf1, 2, inst,   0,    "swap %+" },
  { 0xf2, 1, split,  0,    0 },
  { 0xf3, 1, split,  0,    0 },
  { 0xf4, 1, split,  0,    0 },
  { 0xf5, 3, inst,   SPEC, 0 },
  { 0xf6, 1, split,  0,    0 },
  { 0xf7, 1, split,  0,    0 },

  { 0xf8, 2, inst,   0,    "ld %e,%r" },
  { 0xf9, 2, inst,   0,    "ld %r,%e" },
  { 0xfa, 2, pushj,  0,    "djnz %e,%d" },
  { 0xfb, 2, pushj,  0,    "jr %c,%d" },
  { 0xfc, 2, inst,   0,    "ld %e,%1" },
  { 0xfd, 3, pushj,  0,    "jp %c,%a" },
  { 0xfe, 1, inst,   0,    "inc %e" },
  { 0xff, 1, inst,   0,    "nop" },
};

/*
 * Specials, Z8 mode:
 */

static dispblock specdisp[] = {
  { 0x4f, 1, inst,   0,    "wdh" },
  { 0x5f, 1, inst,   0,    "wdt" },
  { 0x6f, 1, popj,   0,    "stop" },
  { 0x7f, 1, popj,   0,    "halt" },
  { 0x82, 2, inst,   SPEC, 0 },
  { 0x83, 2, inst,   SPEC, 0 },
  { 0x92, 2, inst,   SPEC, 0 },
  { 0x93, 2, inst,   SPEC, 0 },
  { 0xc2, 2, inst,   SPEC, 0 },
  { 0xc3, 2, inst,   SPEC, 0 },
  { 0xc7, 3, inst,   SPEC, 0 },
  { 0xd2, 2, inst,   SPEC, 0 },
  { 0xd3, 2, inst,   SPEC, 0 },
  { 0xd4, 2, inst,   0,    "call @%r" },
  { 0xd6, 3, pushj,  0,    "call %a" },
  { 0xd7, 3, inst,   SPEC, 0 },
  { 0xe3, 2, inst,   SPEC, 0 },
  { 0xf3, 2, inst,   SPEC, 0 },
  { 0,    0, arnold, 0,    0 },
};

/*
 * Specials, super-8 mode:
 */

static dispblock super8disp[] = {
  { 0x07, 3, inst,   SPEC, 0 },
  { 0x0f, 1, inst,   0,    "next" },
  { 0x17, 3, inst,   SPEC, 0 },
  { 0x1f, 1, inst,   0,    "enter" },
  { 0x27, 3, inst,   SPEC, 0 },
  { 0x2f, 1, inst,   0,    "exit" },
  { 0x37, 3, pushj,  SPEC, 0 },
  { 0x3f, 1, inst,   0,    "wfi" },
  { 0x47, 3, inst,   SPEC, 0 },
  { 0x4f, 1, inst,   0,    "sb0" },
  { 0x57, 2, inst,   SPEC, 0 },
  { 0x5f, 1, inst,   0,    "sb1" },
  { 0x67, 3, inst,   SPEC, 0 },
  { 0x6f, 1, popj,   SAMS, "idle" }, /* Samsung, not zilog super-8. */
  { 0x77, 2, inst,   SPEC, 0 },
  { 0x7f, 1, popj,   SAMS, "stop" }, /* Samsung, not zilog super-8. */
  { 0x82, 3, inst,   0,    "pushud @%r,%r" },
  { 0x83, 3, inst,   0,    "pushui @%r,%r" },
  { 0x84, 3, inst,   SPEC, 0 },
  { 0x85, 3, inst,   SPEC, 0 },
  { 0x86, 3, inst,   SPEC, 0 },
  { 0x87, 3, inst,   SPEC, 0 },
  { 0x92, 3, inst,   SPEC, 0 },
  { 0x93, 3, inst,   SPEC, 0 },
  { 0x94, 3, inst,   SPEC, 0 },
  { 0x95, 3, inst,   SPEC, 0 },
  { 0x96, 3, inst,   SPEC, 0 },
  { 0x97, 3, inst,   SPEC, 0 },
  { 0xa7, 4, inst,   SPEC, 0 },
  { 0xb7, 4, inst,   SPEC, 0 },
  { 0xc2, 3, pushj,  SPEC, 0 },
  { 0xc3, 2, inst,   SPEC, 0 },
  { 0xc4, 3, inst,   SPEC, 0 },
  { 0xc5, 3, inst,   SPEC, 0 },
  { 0xc6, 4, inst,   0,    "ldw %r,%2" },
  { 0xc7, 2, inst,   SPEC, 0 },
  { 0xd2, 3, pushj,  SPEC, 0 },
  { 0xd3, 2, inst,   SPEC, 0 },
  { 0xd4, 2, inst,   SPEC, 0 },
  { 0xd6, 3, inst,   0,    "ld @%r,%1" },
  { 0xd7, 2, inst,   SPEC, 0 },
  { 0xe2, 2, inst,   SPEC, 0 },
  { 0xe3, 2, inst,   SPEC, 0 },
  { 0xe7, 3, inst,   SPEC, 0 },
  { 0xf2, 2, inst,   SPEC, 0 },
  { 0xf3, 2, inst,   SPEC, 0 },
  { 0xf4, 2, inst,   SPEC, 0 },
  { 0xf6, 3, pushj,  0,    "call %a" },
  { 0xf7, 3, inst,   SPEC, 0 },
  { 0,    0, arnold, 0,    0 },
};

/*
 * register names:
 */

static dispblock z8regs[] = {
  { 0x00, 0, 0, 0, "p0" },
  { 0x01, 0, 0, 0, "p1" },
  { 0x02, 0, 0, 0, "p2" },
  { 0x03, 0, 0, 0, "p3" },
  { 0xe0, 0, 0, 0, "r0" },
  { 0xe1, 0, 0, 0, "r1" },
  { 0xe2, 0, 0, 0, "r2" },
  { 0xe3, 0, 0, 0, "r3" },
  { 0xe4, 0, 0, 0, "r4" },
  { 0xe5, 0, 0, 0, "r5" },
  { 0xe6, 0, 0, 0, "r6" },
  { 0xe7, 0, 0, 0, "r7" },
  { 0xe8, 0, 0, 0, "r8" },
  { 0xe9, 0, 0, 0, "r9" },
  { 0xea, 0, 0, 0, "r10" },
  { 0xeb, 0, 0, 0, "r11" },
  { 0xec, 0, 0, 0, "r12" },
  { 0xed, 0, 0, 0, "r13" },
  { 0xee, 0, 0, 0, "r14" },
  { 0xef, 0, 0, 0, "r15" },
  { 0xf0, 0, 0, 0, "sio" },
  { 0xf1, 0, 0, 0, "tmr" },
  { 0xf2, 0, 0, 0, "t1" },
  { 0xf3, 0, 0, 0, "pre1" },
  { 0xf4, 0, 0, 0, "t0" },
  { 0xf5, 0, 0, 0, "pre0" },
  { 0xf6, 0, 0, 0, "p2m" },
  { 0xf7, 0, 0, 0, "p3m" },
  { 0xf8, 0, 0, 0, "p01m" },
  { 0xf9, 0, 0, 0, "ipr" },
  { 0xfa, 0, 0, 0, "irq" },
  { 0xfb, 0, 0, 0, "imr" },
  { 0xfc, 0, 0, 0, "flags" },
  { 0xfd, 0, 0, 0, "rp" },
  { 0xfe, 0, 0, 0, "sph" },
  { 0xff, 0, 0, 0, "spl" },
  { 0, 0, arnold, 0, 0 },
};

static dispblock super8regs[] = {
  { 0xc0, 0, 0, 0, "r0" },
  { 0xc1, 0, 0, 0, "r1" },
  { 0xc2, 0, 0, 0, "r2" },
  { 0xc3, 0, 0, 0, "r3" },
  { 0xc4, 0, 0, 0, "r4" },
  { 0xc5, 0, 0, 0, "r5" },
  { 0xc6, 0, 0, 0, "r6" },
  { 0xc7, 0, 0, 0, "r7" },
  { 0xc8, 0, 0, 0, "r8" },
  { 0xc9, 0, 0, 0, "r9" },
  { 0xca, 0, 0, 0, "r10" },
  { 0xcb, 0, 0, 0, "r11" },
  { 0xcc, 0, 0, 0, "r12" },
  { 0xcd, 0, 0, 0, "r13" },
  { 0xce, 0, 0, 0, "r14" },
  { 0xcf, 0, 0, 0, "r15" },
  { 0xd0, 0, 0, 0, "p0" },
  { 0xd1, 0, 0, 0, "p1" },
  { 0xd2, 0, 0, 0, "p2" },
  { 0xd3, 0, 0, 0, "p3" },
  { 0xd4, 0, 0, 0, "p4" },
  { 0xd5, 0, 0, 0, "flags" },
  { 0xd6, 0, 0, 0, "rp0" },
  { 0xd7, 0, 0, 0, "rp1" },
  { 0xd8, 0, 0, 0, "sph" },
  { 0xd9, 0, 0, 0, "spl" },
  { 0xda, 0, 0, 0, "iph" },
  { 0xdb, 0, 0, 0, "ipl" },
  { 0xdc, 0, 0, 0, "irq" },
  { 0xdd, 0, 0, 0, "imr" },
  { 0xde, 0, 0, 0, "sym" },
  { 0xe0, 0, 0, 0, "c0ct" },
  { 0xe1, 0, 0, 0, "c1ct" },
  { 0xe2, 0, 0, 0, "c0ch" },
  { 0xe3, 0, 0, 0, "c0cl" },
  { 0xe4, 0, 0, 0, "c1ch" },
  { 0xe5, 0, 0, 0, "c1cl" },
  { 0xeb, 0, 0, 0, "utc" },
  { 0xec, 0, 0, 0, "urc" },
  { 0xed, 0, 0, 0, "uie" },
  { 0xef, 0, 0, 0, "uio" },
  { 0xf0, 0, 0, 0, "p0m" },
  { 0xf1, 0, 0, 0, "pm" },
  { 0xf4, 0, 0, 0, "h0c" },
  { 0xf5, 0, 0, 0, "h1c" },
  { 0xf6, 0, 0, 0, "p4d" },
  { 0xf7, 0, 0, 0, "p4od" },
  { 0xf8, 0, 0, 0, "p2am" },
  { 0xf9, 0, 0, 0, "p2bm" },
  { 0xfa, 0, 0, 0, "p2cm" },
  { 0xfb, 0, 0, 0, "p2dm" },
  { 0xfc, 0, 0, 0, "p2aip" },
  { 0xfd, 0, 0, 0, "p2bip" },
  { 0xfe, 0, 0, 0, "emt" },
  { 0xff, 0, 0, 0, "ipr" },
  { 0, 0, arnold, 0, 0 },
};

static dispblock samsungregs[] = {
  { 0xc0, 0, 0, 0, "r0" },
  { 0xc1, 0, 0, 0, "r1" },
  { 0xc2, 0, 0, 0, "r2" },
  { 0xc3, 0, 0, 0, "r3" },
  { 0xc4, 0, 0, 0, "r4" },
  { 0xc5, 0, 0, 0, "r5" },
  { 0xc6, 0, 0, 0, "r6" },
  { 0xc7, 0, 0, 0, "r7" },
  { 0xc8, 0, 0, 0, "r8" },
  { 0xc9, 0, 0, 0, "r9" },
  { 0xca, 0, 0, 0, "r10" },
  { 0xcb, 0, 0, 0, "r11" },
  { 0xcc, 0, 0, 0, "r12" },
  { 0xcd, 0, 0, 0, "r13" },
  { 0xce, 0, 0, 0, "r14" },
  { 0xcf, 0, 0, 0, "r15" },
  { 0xd0, 0, 0, 0, "t0cnt" },
  { 0xd1, 0, 0, 0, "t0data" },
  { 0xd2, 0, 0, 0, "t0con" },
  { 0xd3, 0, 0, 0, "btcon" },
  { 0xd4, 0, 0, 0, "clkcon" },
  { 0xd5, 0, 0, 0, "flags" },
  { 0xd6, 0, 0, 0, "rp0" },
  { 0xd7, 0, 0, 0, "rp1" },
  { 0xd8, 0, 0, 0, "sph" },
  { 0xd9, 0, 0, 0, "spl" },
  { 0xda, 0, 0, 0, "iph" },
  { 0xdb, 0, 0, 0, "ipl" },
  { 0xdc, 0, 0, 0, "irq" },
  { 0xdd, 0, 0, 0, "imr" },
  { 0xde, 0, 0, 0, "sym" },
  { 0xdf, 0, 0, 0, "pp" },
  { 0xe0, 0, 0, 0, "p0" },
  { 0xe1, 0, 0, 0, "p1" },
  { 0xe2, 0, 0, 0, "p2" },
  { 0xe3, 0, 0, 0, "p3" },
  { 0xe5, 0, 0, 0, "p2int" },
  { 0xe6, 0, 0, 0, "p2pnd" },
  { 0xe7, 0, 0, 0, "p0pur" },
  { 0xe8, 0, 0, 0, "p0conh" },
  { 0xe9, 0, 0, 0, "p0conl" },
  { 0xea, 0, 0, 0, "p1conh" },
  { 0xeb, 0, 0, 0, "p1conl" },
  { 0xec, 0, 0, 0, "p2conh" },
  { 0xed, 0, 0, 0, "p2conl" },
  { 0xee, 0, 0, 0, "p2pur" },
  { 0xef, 0, 0, 0, "p3con" },
  { 0xf1, 0, 0, 0, "p0int" },
  { 0xf2, 0, 0, 0, "p0pnd" },
  { 0xf3, 0, 0, 0, "cacon" },
  { 0xf4, 0, 0, 0, "cadatah" },
  { 0xf5, 0, 0, 0, "cadatal" },
  { 0xf6, 0, 0, 0, "t1cnth" },
  { 0xf7, 0, 0, 0, "t1cntl" },
  { 0xf8, 0, 0, 0, "t1datah" },
  { 0xf9, 0, 0, 0, "t1datal" },
  { 0xfa, 0, 0, 0, "t1con" },
  { 0xfb, 0, 0, 0, "stopcon" },
  { 0xfd, 0, 0, 0, "btcnt" },
  { 0xfe, 0, 0, 0, "emt" },
  { 0xff, 0, 0, 0, "ipr" },
  { 0, 0, arnold, 0, 0 },
};

/* local variables: */

static address* touch;

static byte opcode;

static bool super8;		/* True if super-8 instruction set. */

/************************************************************************/

static void number(word w)
{
  switch (radix) {
  case 2:
    bufbinary(w, 1);
    if (w > 1)
      casechar('b');
    break;
  case 8:
    bufoctal(w, 1);
    if (w > 7)
      casechar('q');
    break;
  case 10:
    bufdecimal(w, 1);
    break;
  case 16:
  default:
    bufhex(w, 0);
    if (w > 9)
      casechar('h');
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
  
  sprintf(work, "L_%04" PRIxw, w);
  l_insert(a_l2a(w), work);
}

/*
** dobyte() will output the current item as a byte of data.
*/

static void defb(byte b)
{
  pb_length = 1;
  pb_status = st_byte;
  startline(true);
  casestring("defb");
  spacedelim();
  number(b);
  checkblank();
}

static void z8_dobyte(void)
{
  defb(getbyte());
}

static void z8_dochar(void)
{
  byte b;

  b = getbyte();
  pb_length = 1;
  startline(true);
  casestring("defb");
  spacedelim();

  if (printable(b)) {
    bufchar('"');
    if (b == '"') {
      bufchar('"');
    }
    bufchar((char) b);
    bufchar('"');
  } else {
    number(b);
  }

  checkblank();
}

static void z8_doword(void)
{
  word w;

  w = getword();
  pb_length = 2;
  startline(true);
  casestring("defw");
  spacedelim();
  number(w);
  checkblank();
}

static void z8_doptr(void)
{
  address* a;
  word w;

  w = getword();
  a = a_l2a(w);

  pb_length = 2;
  startline(true);
  casestring("defw");
  spacedelim();
  reference(a);
  if (l_exist(a)) {
    bufstring(l_find(a));
  } else {
    number(w);
  }
  endref();
  checkblank();
}

static void z8_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    setpc(istart);
    z8_dochar();
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

static word getdisp(void)
{
  byte b;

  b = getbyte();
  return a_a2w(pc) + sextb(b);
}

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

static void refdisp(word w)
{
  int i;

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
    i = w + 2 - a_a2l(pc);
    bufchar('$');
    if (i < 0) {
      bufchar('-'); number(-i);
    }
    if (i > 0) {
      bufchar('+'); number(i);
    }
  }
  endref();
}

static void iconst(byte b)
{
  if (argstatus == st_char && printable(b)) {
    bufchar('\'');
    bufchar((char) b);
    if (b == '\'')
      bufchar((char) b);
    bufchar('\'');
  } else {
    bufchar('#');
    number(b);
  }
}

static void refreg(byte b)
{
  dispblock* d;

  /* XXX Handle range c0-cf (super8) or e0-ef (z8) as r0-r15 */

  d = finddisp(b, super8? super8regs : z8regs, NULL);

  if (d != NULL)
    casestring(d->expand);
  else
    number(b);
}

static void r4l(byte b)
{
  casechar('r');
  bufdecimal(b >> 4, 1);
}

static void r4r(byte b)
{
  casechar('r');
  bufdecimal(b & 0x0f, 1);
}

static void stdarg(void)
{
  byte src, dst;
  byte mode;

  dst = getbyte();
  mode = opcode & 0x0f;

  switch (mode) {
  case 0:
  case 1:
    if (mode & 1)
      bufchar('@');
    refreg(dst);
    break;
  case 2:
  case 3:
    r4l(dst);
    argdelim(",");
    if (mode & 1)
      bufchar('@');
    r4r(dst);
    break;
  case 4:
  case 5:
    src = dst;
    dst = getbyte();
    refreg(dst);
    argdelim(",");
    if (mode & 1)
      bufchar('@');
    refreg(src);
    break;
  case 6:
  case 7:
    if (mode & 1)
      bufchar('@');
    refreg(dst);
    argdelim(",");
    iconst(getbyte());
    break;    
  default:
    /* this should not happen. */
    break;
  }
}

static void ccode(byte cc)
{
  static char* defcc[16] = {
    "f",   "lt",  "le",  "ule",
    "ov",  "mi",  "eq",  "c",
    "",    "ge",  "gt",  "ugt",
    "nov", "pl",  "ne",  "nc",
  };

  static char* unscc[16] = {
    NULL,  NULL,  NULL,  "ule",
    NULL,  NULL,  "z",   "ult",
    NULL,  NULL,  NULL,  "ugt",
    NULL,  NULL,  "nz",  "uge",
  };

  if (argsign == SIGN_UNSIGNED && unscc[cc] != NULL)
    casestring(unscc[cc]);
  else
    casestring(defcc[cc]);
}

static void copytext(char* p)
{
  char c;
  word w;

  while ((c = *p++) != 0) {
    if (c == '%') {
      c = *p++;
      switch (c) {
      case '+':			/* Standard argument format */
	stdarg();
	break;
      case '1':			/* One byte of in-line data */
	iconst(getbyte());
	break;
      case '2':			/* Two bytes of in-line data */
	w = getword();		/* (might be a pointer.) */
	touch = a_l2a(w);
	if (argstatus == st_ptr) {
	  if (l_exist(touch)) {
	    reference(touch);
	    bufstring(l_find(touch));
	  } else {
	    bufchar('#');
	    reference(touch);
	    number(w);
	  }
	  endref();
	} else {
	  bufchar('#');
	  number(w);
	}
	break;
      case 'a':			/* Absolute address */
	refaddr(getword());
	break;
      case 'c':			/* Embed. condition code */
	ccode(opcode >> 4);
	break;
      case 'd':			/* Relative address */
	refdisp(getdisp());
	break;
      case 'e':			/* Embed. register number */
	r4l(opcode);
	break;
      case 'r':			/* Register number byte */
	refreg(getbyte());
	break;
      default:			/* For now: */
	bufchar('%');
	bufchar(c);
	break;
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

static void putop(char* op)
{
  startline(true);

  casestring(op);
  spacedelim();
}

static void b1arg(byte b2)
{
  r4l(b2);
  bufchar('.');
  bufdecimal((b2 >> 1) & 0x07, 1);
}

static void b2arg(byte b2, byte b3)
{
  if (b2 & 1) {
    refreg(b3);
    bufchar('.');
    bufdecimal((b2 >> 1) & 0x07, 1);
    argdelim(",");
    r4l(b2);
  } else {
    r4l(b2);
    argdelim(",");
    refreg(b3);
    bufchar('.');
    bufdecimal((b2 >> 1) & 0x07, 1);
  }
}

/* handle arguments for mult/div instructions. */

static void mularg(byte b2, byte b3)
{
  refreg(b3);		/* 16-bit reg */
  argdelim(",");
  switch (opcode & 0x0f) {
  case 4:
    refreg(b2);
    break;
  case 5:
    bufchar('@');
    refreg(b2);
    break;
  case 6:
    iconst(b2);
    break;
  }
}

static void ldcarg(int len, byte b2, byte b3, byte b4)
{
  word w;

  w = (b4 << 8) + b3;

  switch (b2 & 0x0f) {
  case 0: /* direct addr, program memory */
    refaddr(w);
    break;
  case 1: /* direct addr, data memory */
    number(w);
    break;
  default:
    if (len == 4 && !(b2 & 1)) {
      refaddr(w);
    } else {
      bufchar('#');
      number(w);		/* Should be signed if len == 3. */
    }
    bufchar('[');
    casestring("rr");
    bufdecimal(b2 & 0x0e, 1);
    bufchar(']');
    break;
  }
}

static bool special(dispblock* disp)
{
  byte b2, b3, b4;

  b2 = b3 = b4 = 0;		/* Make GCC shut up. */

  if (disp->length >= 2)
    b2 = getbyte();
  if (disp->length >= 3)
    b3 = getbyte();
  if (disp->length >= 4)
    b4 = getbyte();

  /*
  ** There are three opcodes that handle the same for Z8 and Super-8.
  ** They are 0xe4, 0xe5 and 0xf5.  Think about handling them before
  ** the splitup between architectures.
  */

  if (super8) {
    /* Super-8 instruction set. */
    switch (opcode) {
    case 0x07: // BOR
      putop("bor");
      b2arg(b2, b3);
      break;
    case 0x17: // BCP
      if (b2 & 1)
	return false;
      putop("bcp");
      b2arg(b2, b3);
      break;
    case 0x27: // BXOR
      putop("bxor");
      b2arg(b2, b3);
      break;
    case 0x31: // SRP
      switch (b2 & 7) {
      case 0:
	putop("srp");
	bufchar('#');
	number(b2);
	break;
      case 1:
	putop("srp1");
	bufchar('#');
	number(b2 & 0xf8);
	break;
      case 2:
	putop("srp0");
	bufchar('#');
	number(b2 & 0xf8);
	break;
      default:
	return false;
      }
      break;
    case 0x37: // BTJRF/BTJRT
      putop(b2 & 1? "btjrt" : "btjrf");
      refdisp(a_a2w(pc) + sextb(b3));
      argdelim(",");
      b1arg(b2);
      break;
    case 0x47: // LDB
      putop("ldb");
      b2arg(b2, b3);
      break;
    case 0x57: // BITC
      if (b2 & 1)
	return false;
      putop("bitc");
      b1arg(b2);
      break;
    case 0x67: // BAND
      putop("band");
      b2arg(b2, b3);
      break;
    case 0x77: // BITS/BITR
      putop(b2 & 1? "bits" : "bitr");
      b1arg(b2);
      break;
    case 0x84: // MULT    rr,r      [OPC] [src] [dst]
    case 0x85: // MULT    rr,Ir     [OPC] [src] [dst]
    case 0x86: // MULT    rr,IM     [OPC] [src] [dst]
      putop("mult");
      mularg(b2, b3);
      break;
    case 0x87: // LD      x[r],r    [OPC] [dst|src] [x]
      putop("ld");
      r4l(b2);
      argdelim(",");
      iconst(b3);
      bufchar('[');
      r4r(b2);
      bufchar(']');
      break;
    case 0x92: // POPUD   R,IR      [OPC] [src] [dst]
      putop("popup");
      refreg(b2);
      argdelim(",");
      bufchar('@');
      refreg(b3);
      break;
    case 0x93: // POPUI   R,IR      [OPC] [src] [dst]
      putop("popup");
      refreg(b2);
      argdelim(",");
      bufchar('@');
      refreg(b3);
      break;
    case 0x94: // DIV     rr,r      [OPC] [src] [dst]
    case 0x95: // DIV     rr,Ir     [OPC] [src] [dst]
    case 0x96: // DIV     rr,IM     [OPC] [src] [dst]
      putop("div");
      mularg(b2, b3);
      break;
    case 0x97: // LD      r,x[r]    [OPC] [src|dst] [x]
      putop("ld");
      iconst(b3);
      bufchar('[');
      r4r(b2);
      bufchar(']');
      argdelim(",");
      r4l(b2);
      break;
    case 0xa7: // LDC/LDE
      putop(b2 & 1? "lde" : "ldc");
      r4l(b2);
      argdelim(",");
      ldcarg(4, b2, b3, b4);
      break;
    case 0xb7: // LDC/LDE
      putop(b2 & 1? "lde" : "ldc");
      ldcarg(4, b2, b3, b4);
      argdelim(",");
      r4l(b2);
      break;
    case 0xc2: // CPIJE    r,Ir,RA  [OPC] [dst|src] [off]
      putop("cpije");
      r4r(b2);
      argdelim(",");
      bufchar('@');
      r4l(b2);
      argdelim(",");
      refdisp(a_a2w(pc) + sextb(b3));
      break;
    case 0xc3: // LDC      r,Irr    [OPC] [dst|src]
      putop(b2 & 1? "lde" : "ldc");
      r4l(b2);
      argdelim(",");
      bufchar('@');
      casestring("rr");
      bufdecimal(b2 & 0x0e, 1);
      break;
    case 0xc4: // LDW      rr/rr    [OPC] [src] [dst]
      putop("ldw");
      refreg(b3);
      argdelim(",");
      refreg(b2);
      break;
    case 0xc5: // LDW      rr/Ir    [OPC] [src] [dst]
      putop("ldw");
      refreg(b3);
      argdelim(",");
      bufchar('@');
      refreg(b2);
      break;
    case 0xc7: // LD       r,Ir     [OPC] [dst|src]
      putop("ld");
      r4l(b2);
      argdelim(",");
      bufchar('@');
      r4r(b2);
      break;
    case 0xd2: // CPIJNE   r,Ir,RA  [OPC] [dst|src] [off]
      putop("cpijne");
      r4r(b2);
      argdelim(",");
      bufchar('@');
      r4l(b2);
      argdelim(",");
      refdisp(a_a2w(pc) + sextb(b3));
      break;
    case 0xd3: // LDC      Irr,r    [OPC] [src|dst]
      putop(b2 & 1? "lde" : "ldc");
      bufchar('@');
      casestring("rr");
      bufdecimal(b2 & 0x0e, 1);
      argdelim(",");
      r4l(b2);
      break;
    case 0xd4: // CALL     IA       [OPC] [dst]
      startline(true);
      copytext("call %1");
      break;
    case 0xd7: // LD       Ir,r     [OPC] [dst|src]
      putop("ld");
      bufchar('@');
      r4l(b2);
      argdelim(",");
      r4r(b2);
      break;
    case 0xe2: // LDCD     r,Irr    [OPC] [dst|src]
      putop(b2 & 1? "lded" : "ldcd");
      r4l(b2);
      argdelim(",");
      bufchar('@');
      casestring("rr");
      bufdecimal(b2 & 0x0e, 1);
      break;
    case 0xe3: // LDCI     r,Irr    [OPC] [dst|src]
      putop(b2 & 1? "ldei" : "ldci");
      r4l(b2);
      argdelim(",");
      bufchar('@');
      casestring("rr");
      bufdecimal(b2 & 0x0e, 1);
      break;
    case 0xe4: // LD
      putop("ld");
      refreg(b3);
      argdelim(",");
      refreg(b2);
      break;
    case 0xe5: // LD
      putop("ld");
      refreg(b3);
      argdelim(",");
      bufchar('@');
      refreg(b2);
      break;
    case 0xe7: // LDC      r,x[rr]  [OPC] [dst|src] [x]
      putop(b2 & 1? "lde" : "ldc");
      r4l(b2);
      argdelim(",");
      ldcarg(3, b2, b3, 0);
      break;
    case 0xf2: // LDCPD    Irr,r    [OPC] [src|dst]
      putop(b2 & 1? "ldepd" : "ldcpd");
      bufchar('@');
      casestring("rr");
      bufdecimal(b2 & 0x0e, 1);
      argdelim(",");
      r4l(b2);
      break;
    case 0xf3: // LDCPI    Irr,r    [OPC] [src|dst]
      putop(b2 & 1? "ldepi" : "ldcpi");
      bufchar('@');
      casestring("rr");
      bufdecimal(b2 & 0x0e, 1);
      argdelim(",");
      r4l(b2);
      break;
    case 0xf4: // CALL
      putop("call");
      bufchar('@');
      refreg(b2);
      break;
    case 0xf5: // LD
      putop("ld");
      bufchar('@');
      refreg(b3);
      argdelim(",");
      refreg(b2);
      break;
    case 0xf7: // LDC      x[rr],r  [OPC] [src|dst] [x]
      putop(b2 & 1? "lde" : "ldc");
      ldcarg(3, b2, b3, 0);
      argdelim(",");
      r4l(b2);
      break;
    default:
      /* This can only happen if there is bad data in the tables. */
      return false;
    }
  } else {
    /* Plain Z8 instructions. */
    switch (opcode) {
    case 0x31: //  "srp %1"
      putop("srp");
      bufchar('#');
      number(b2);
      break;
    case 0x82: //  "lde dst,src"  r/Irr  [OPC] [dst|src]
      putop("lde");
      r4l(b2);
      argdelim(",");
      bufchar('@');
      r4r(b2);
      break;
    case 0x83: //  "ldei dst,src" Ir/Irr [OPC] [dst|src]
      putop("ldei");
      bufchar('@');
      r4l(b2);
      argdelim(",");
      bufchar('@');
      r4r(b2);
      break;
    case 0x92: //  "lde dst,src"  Irr/r  [OPC] [src|dst]
      putop("lde");
      bufchar('@');
      r4r(b2);
      argdelim(",");
      r4l(b2);
      break;
    case 0x93: //  "ldei dst,src" Irr/Ir [OPC] [src|dst]
      putop("ldei");
      bufchar('@');
      r4r(b2);
      argdelim(",");
      bufchar('@');
      r4l(b2);
      break;
    case 0xc2: //  "ldc dst,src"  r/Irr  [OPC] [dst|src]
      putop("ldc");
      r4l(b2);
      argdelim(",");
      bufchar('@');
      r4r(b2);
      break;
    case 0xc3: //  "ldci dst,src" Ir/Irr [OPC] [dst|src]
      putop("ldci");
      bufchar('@');
      r4l(b2);
      argdelim(",");
      bufchar('@');
      r4r(b2);
      break;
    case 0xc7: //  "ld r, x(R)"   r/X    [OPC] [dst|x] [src]
      putop("ld");
      r4l(b2);
      argdelim(",");
      number(b3);
      bufchar('(');
      r4l(b2);
      bufchar(')');
      break;
    case 0xd2: //  "ldc dst,src"  Irr/r  [OPC] [src|dst]
      putop("ldc");
      bufchar('@');
      r4r(b2);
      argdelim(",");
      r4l(b2);
      break;
    case 0xd3: //  "ldci dst,src" Irr/Ir [OPC] [src|dst]
      putop("ldci");
      bufchar('@');
      r4r(b2);
      argdelim(",");
      bufchar('@');
      r4l(b2);
      break;
    case 0xd7: //  "ld x(R), r"   X/r    [OPC] [src|x] [dst]
      putop("ld");
      number(b3);
      bufchar('(');
      r4r(b2);
      bufchar(')');
      argdelim(",");
      r4l(b2);
      break;
    case 0xe3: //  "ld dst,src"   r/Ir   [OPC] [dst|src]
      putop("ld");
      r4l(b2);
      argdelim(",");
      bufchar('@');
      r4r(b2);
      break;	
    case 0xe4: //  "ld dst,src"   R/R    [OPC] [src] [dst]
      putop("ld");
      refreg(b3);
      argdelim(",");
      refreg(b2);
      break;
    case 0xe5: //  "ld dst,src"   R/IR   [OPC] [src] [dst]
      putop("ld");
      refreg(b3);
      argdelim(",");
      bufchar('@');
      refreg(b2);
      break;
    case 0xf3: //  "ld dst,src"   Ir/r   [OPC] [dst|src]
      putop("ld");
      bufchar('@');
      r4l(b2);
      argdelim(",");
      r4r(b2);
      break;
    case 0xf5: //  "ld dst,src"   IR/R   [OPC] [src] [dst]
      putop("ld");
      bufchar('@');
      refreg(b3);
      argdelim(",");
      refreg(b2);
      break;
    default:
      /* This can only happen if there is bad data in the tables. */
      return false;
    }
  }
  return true;
}

static void z8_doinstr(void)
{
  dispblock* disp;

  opcode = getbyte();
  disp = &z8disp[opcode];

  if (disp->itype == split) {
    disp = finddisp(opcode, super8? super8disp : specdisp, NULL);
  }

  if (super8 && disp->flags & SUP8) {
    disp = finddisp(opcode, super8disp, NULL);
  }

  if (disp == NULL || disp->itype == unused) {
    defb(opcode);
    return;
  }

  pb_length = disp->length;

  if (overrun()) {
    defb(opcode);
    return;
  }

  if (disp->flags & SPEC) {
    if (!special(disp)) {
      defb(opcode);
      return;
    }
  } else {
    startline(true);
    copytext(disp->expand);
  }

  switch (disp->itype) {
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

void z8_begin(void)
{
  bufstring(";Beginning of z8 program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

void z8_org(address* a)
{
  bufblankline();
  casestring("org");
  spacedelim();
  number(a_a2w(a));
  bufblankline();
}

void z8_end(void)
{
  bufblankline();
  bufstring(";End of z8 program");
}

/************************************************************************/

char* z8_lcan(char* name)
{
  static char work[10];

  return canonicalize(name, work, 6);
}

bool z8_lchk(char* name)
{
  return checkstring(name, "", "0123456789_");
}

void z8_lgen(address* addr)
{
  genlabel(a_a2w(addr));
}

/**********************************************************************/

void z8_init(void)
{
  /* Set up our functions: */
  
  spf_lcan(z8_lcan);
  spf_lchk(z8_lchk);
  spf_lgen(z8_lgen);

  /* set up our object handlers: */
  
  spf_dodef(z8_dobyte);

  spf_doobj(st_inst, z8_doinstr);
  spf_doobj(st_ptr,  z8_doptr);
  spf_doobj(st_word, z8_doword);
  spf_doobj(st_char, z8_dochar);
  spf_doobj(st_text, z8_dotext);
  spf_doobj(st_byte, z8_dobyte);

  spf_begin(z8_begin);
  spf_end(z8_end);
  spf_org(z8_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 16;		/* Number of address bits. */
  pv_bigendian = true;		/* This animal seems to be mostly BE. */
  pv_cstart = ";";		/* Comment start string. */

  super8 = false;		/* Assume. */
}

void super8_init(void)
{
  z8_init();
  super8 = true;		/* Change our mind. */
}

/*
** This routine prints a help string for this processor, so that the
** user can ask us what the **** we are.  We should give information on
** what the different assemblers mean and all that jazz.
*/

bool z8_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string missing...\n\
");
    return true;
  }
  return false;
}

bool super8_help(int helptype)
{
  return z8_help(helptype);
}
