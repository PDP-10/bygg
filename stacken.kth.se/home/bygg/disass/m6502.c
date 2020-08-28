/*
** This module implements driver code for the 6502 processor, and
** its friends.
*/

/*
** Fix the following:
**
** 6502      - original instruction set.
** 65c02     - include instructions marked with C02 bit in table.
** 65c02-rw  - include bit instructions from second table.
** 65816     - include extended instructions from second table.
**
*/

#include "disass.h"

/**********************************************************************/

evf_init m6502_init;
evf_help m6502_help;

struct entryvector m6502_vector = {
  "6502",			/* Name */
  "MOS 6502",			/* One-liner. */
  m6502_init,			/* Init routine */
  m6502_help,			/* Help routine */
};

/**********************************************************************/

evf_init m65c02_init;
evf_help m65c02_help;

struct entryvector m65c02_vector = {
  "65c02",			/* Name */
  "MOS 65C02",			/* One-liner. */
  m65c02_init,			/* Init routine */
  m65c02_help,			/* Help routine */
};

/**********************************************************************/

evf_init m65c02_rw_init;
evf_help m65c02_rw_help;

struct entryvector m65c02_rw_vector = {
  "65c02-rw",			/* Name */
  "MOS 65C02 (Rockwell/WDC)",	/* One-liner. */
  m65c02_rw_init,		/* Init routine */
  m65c02_rw_help,		/* Help routine */
};

/**********************************************************************/

evf_init m65816_init;
evf_help m65816_help;

struct entryvector m65816_vector = {
  "65816",			/* Name */
  "MOS 65816",			/* One-liner. */
  m65816_init,			/* Init routine */
  m65816_help,			/* Help routine */
};

/**********************************************************************/

/* Dispatch tables: */

/* itype values: */

#define unused  0
#define inst    1
#define instb   2
#define jrst    3
#define pushj   4
#define popj    5
#define split   6

/* flags: */

#define C02     0x01		/* 65C02 extension. */

/*
 * %a  - two-byte absolute address.
 * %d  - one-byte displacement.
 * %0  - zero-page address.
 * %1  - one-byte immed. constant.
 */

static dispblock m6502disp[256] = {
  { 0x00, 1, inst,   0,   "brk" },
  { 0x01, 2, inst,   0,   "ora (%0,X)" },
  { 0x02, 0, split,  0,   0 },
  { 0x03, 0, split,  0,   0 },
  { 0x04, 2, inst,   C02, "tsb %0" },
  { 0x05, 2, inst,   0,   "ora %0" },
  { 0x06, 2, inst,   0,   "asl %0" },
  { 0x07, 0, split,  0,   0 },
  { 0x08, 1, inst,   0,   "php" },
  { 0x09, 2, inst,   0,   "ora %1" },
  { 0x0a, 1, inst,   0,   "asl a" },
  { 0x0b, 0, split,  0,   0 },
  { 0x0c, 3, inst,   C02, "tsb %a" },
  { 0x0d, 3, inst,   0,   "ora %a" },
  { 0x0e, 3, inst,   0,   "asl %a" },
  { 0x0f, 0, split,  0,   0 },
  { 0x10, 2, pushj,  0,   "bpl %d" },
  { 0x11, 2, inst,   0,   "ora (%0),y" },
  { 0x12, 2, inst,   C02, "ora (%0)" },
  { 0x13, 0, split,  0,   0 },
  { 0x14, 2, inst,   C02, "trb %0" },
  { 0x15, 2, inst,   0,   "ora %0,X" },
  { 0x16, 2, inst,   0,   "asl %0,X" },
  { 0x17, 0, split,  0,   0 },
  { 0x18, 1, inst,   0,   "clc" },
  { 0x19, 3, inst,   0,   "ora %a,Y" },
  { 0x1a, 1, inst,   C02, "inc a" },
  { 0x1b, 0, split,  0,   0 },
  { 0x1c, 3, inst,   C02, "trb %a" },
  { 0x1d, 3, inst,   0,   "ora %a,X" },
  { 0x1e, 3, inst,   0,   "asl %a,X" },
  { 0x1f, 0, split,  0,   0 },
  { 0x20, 3, pushj,  0,   "jsr %a" },
  { 0x21, 2, inst,   0,   "and (%0,X)" },
  { 0x22, 0, split,  0,   0 },
  { 0x23, 0, split,  0,   0 },
  { 0x24, 2, inst,   0,   "bit %0" },
  { 0x25, 2, inst,   0,   "and %0" },
  { 0x26, 2, inst,   0,   "rol %0" },
  { 0x27, 0, split,  0,   0 },
  { 0x28, 1, inst,   0,   "plp" },
  { 0x29, 2, inst,   0,   "and %1" },
  { 0x2a, 1, inst,   0,   "rol a" },
  { 0x2b, 0, split,  0,   0 },
  { 0x2c, 3, inst,   0,   "bit %a" },
  { 0x2d, 3, inst,   0,   "and %a" },
  { 0x2e, 3, inst,   0,   "rol %a" },
  { 0x2f, 0, split,  0,   0 },
  { 0x30, 2, pushj,  0,   "bmi %d" },
  { 0x31, 2, inst,   0,   "and (%0),y" },
  { 0x32, 2, inst,   C02, "and (%0)" },
  { 0x33, 0, split,  0,   0 },
  { 0x34, 2, inst,   C02, "bit %0,X" },
  { 0x35, 2, inst,   0,   "and %0,X" },
  { 0x36, 2, inst,   0,   "rol %0,X" },
  { 0x37, 0, split,  0,   0 },
  { 0x38, 1, inst,   0,   "sec" },
  { 0x39, 3, inst,   0,   "and %a,Y" },
  { 0x3a, 1, inst,   C02, "dec a" },
  { 0x3b, 0, split,  0,   0 },
  { 0x3c, 3, inst,   C02, "bit %a,X" },
  { 0x3d, 3, inst,   0,   "and %a,X" },
  { 0x3e, 3, inst,   0,   "rol %a,X" },
  { 0x3f, 0, split,  0,   0 },
  { 0x40, 1, popj,   0,   "rti" },
  { 0x41, 2, inst,   0,   "eor (%0,X)" },
  { 0x42, 0, split,  0,   0 },
  { 0x43, 0, split,  0,   0 },
  { 0x44, 0, split,  0,   0 },
  { 0x45, 2, inst,   0,   "eor %0" },
  { 0x46, 2, inst,   0,   "lsr %0" },
  { 0x47, 0, split,  0,   0 },
  { 0x48, 1, inst,   0,   "pha" },
  { 0x49, 2, inst,   0,   "eor %1" },
  { 0x4a, 1, inst,   0,   "lsr a" },
  { 0x4b, 0, split,  0,   0 },
  { 0x4c, 3, jrst,   0,   "jmp %a" },
  { 0x4d, 3, inst,   0,   "eor %a" },
  { 0x4e, 3, inst,   0,   "lsr %a" },
  { 0x4f, 0, split,  0,   0 },
  { 0x50, 2, pushj,  0,   "bvc %d" },
  { 0x51, 2, inst,   0,   "eor (%0),y" },
  { 0x52, 2, inst,   C02, "eor (%0)" },
  { 0x53, 0, split,  0,   0 },
  { 0x54, 0, split,  0,   0 },
  { 0x55, 2, inst,   0,   "eor %0,X" },
  { 0x56, 2, inst,   0,   "lsr %0,X" },
  { 0x57, 0, split,  0,   0 },
  { 0x58, 1, inst,   0,   "cli" },
  { 0x59, 3, inst,   0,   "eor %a,Y" },
  { 0x5a, 1, inst,   C02, "phy" },
  { 0x5b, 0, split,  0,   0 },
  { 0x5c, 0, split,  0,   0 },
  { 0x5d, 3, inst,   0,   "eor %a,X" },
  { 0x5e, 3, inst,   0,   "lsr %a,X" },
  { 0x5f, 0, split,  0,   0 },
  { 0x60, 1, popj,   0,   "rts" },
  { 0x61, 2, inst,   0,   "adc (%0,X)" },
  { 0x62, 0, split,  0,   0 },
  { 0x63, 0, split,  0,   0 },
  { 0x64, 0, inst,   C02, "stz %0" },
  { 0x65, 2, inst,   0,   "adc %0" },
  { 0x66, 2, inst,   0,   "ror %0" },
  { 0x67, 0, split,  0,   0 },
  { 0x68, 1, inst,   0,   "pla" },
  { 0x69, 2, inst,   0,   "adc %1" },
  { 0x6a, 3, inst,   0,   "ror %a" },
  { 0x6b, 0, split,  0,   0 },
  { 0x6c, 3, popj,   0,   "jmp (%a)" },
  { 0x6d, 3, inst,   0,   "adc %a" },
  { 0x6e, 3, inst,   0,   "ror %a" },
  { 0x6f, 0, split,  0,   0 },
  { 0x70, 2, pushj,  0,   "bvs %d" },
  { 0x71, 2, inst,   0,   "adc (%0),y" },
  { 0x72, 2, inst,   C02, "adc (%0)" },
  { 0x73, 0, split,  0,   0 },
  { 0x74, 2, inst,   C02, "stz %0,X" },
  { 0x75, 2, inst,   0,   "adc %0,X" },
  { 0x76, 2, inst,   0,   "ror %0,X" },
  { 0x77, 0, split,  0,   0 },
  { 0x78, 1, inst,   0,   "sei" },
  { 0x79, 3, inst,   0,   "adc %a,Y" },
  { 0x7a, 1, inst,   C02, "ply" },
  { 0x7b, 0, split,  0,   0 },
  { 0x7c, 3, popj,   C02, "jmp (%a,X)" },
  { 0x7d, 3, inst,   0,   "adc %a,X" },
  { 0x7e, 3, inst,   0,   "ror %a,X" },
  { 0x7f, 0, split,  0,   0 },
  { 0x80, 2, jrst,   C02, "bra %d" },
  { 0x81, 2, inst,   0,   "sta (%0,X)" },
  { 0x82, 0, split,  0,   0 },
  { 0x83, 0, split,  0,   0 },
  { 0x84, 2, inst,   0,   "sty %0" },
  { 0x85, 2, inst,   0,   "sta %0" },
  { 0x86, 2, inst,   0,   "stx %0" },
  { 0x87, 0, split,  0,   0 },
  { 0x88, 1, inst,   0,   "dey" },
  { 0x89, 2, inst,   C02, "bit %1" },
  { 0x8a, 1, inst,   0,   "txa" },
  { 0x8b, 0, split,  0,   0 },
  { 0x8c, 3, inst,   0,   "sty %a" },
  { 0x8d, 3, inst,   0,   "sta %a" },
  { 0x8e, 3, inst,   0,   "stx %a" },
  { 0x8f, 0, split,  0,   0 },
  { 0x90, 2, pushj,  0,   "bcc %d" },
  { 0x91, 2, inst,   0,   "sta (%0),y" },
  { 0x92, 2, inst,   C02, "sta (%0)" },
  { 0x93, 0, split,  0,   0 },
  { 0x94, 2, inst,   0,   "sty %0,X" },
  { 0x95, 2, inst,   0,   "sta %0,X" },
  { 0x96, 2, inst,   0,   "stx %0,Y" },
  { 0x97, 0, split,  0,   0 },
  { 0x98, 1, inst,   0,   "tya" },
  { 0x99, 3, inst,   0,   "sta %a,Y" },
  { 0x9a, 1, inst,   0,   "txs" },
  { 0x9b, 0, split,  0,   0 },
  { 0x9c, 3, inst,   C02, "stz %a" },
  { 0x9d, 3, inst,   0,   "sta %a,X" },
  { 0x9e, 0, inst,   C02, "stz %a,X" },
  { 0x9f, 0, split,  0,   0 },
  { 0xa0, 2, inst,   0,   "ldy %1" },
  { 0xa1, 2, inst,   0,   "lda (%0,X)" },
  { 0xa2, 2, inst,   0,   "ldx %1" },
  { 0xa3, 0, split,  0,   0 },
  { 0xa4, 2, inst,   0,   "ldy %0" },
  { 0xa5, 2, inst,   0,   "lda %0" },
  { 0xa6, 2, inst,   0,   "ldx %0" },
  { 0xa7, 0, split,  0,   0 },
  { 0xa8, 1, inst,   0,   "tay" },
  { 0xa9, 2, inst,   0,   "lda %1" },
  { 0xaa, 1, inst,   0,   "tax" },
  { 0xab, 0, split,  0,   0 },
  { 0xac, 3, inst,   0,   "ldy %a" },
  { 0xad, 3, inst,   0,   "lda %a" },
  { 0xae, 3, inst,   0,   "ldx %a" },
  { 0xaf, 0, split,  0,   0 },
  { 0xb0, 2, pushj,  0,   "bcs %d" },
  { 0xb1, 2, inst,   0,   "lda (%0),y" },
  { 0xb2, 2, inst,   C02, "lda (%0)" },
  { 0xb3, 0, split,  0,   0 },
  { 0xb4, 2, inst,   0,   "ldy %0,X" },
  { 0xb5, 2, inst,   0,   "lda %0,X" },
  { 0xb6, 2, inst,   0,   "ldx %0,Y" },
  { 0xb7, 0, split,  0,   0 },
  { 0xb8, 1, inst,   0,   "clv" },
  { 0xb9, 3, inst,   0,   "lda %a,Y" },
  { 0xba, 1, inst,   0,   "tsx" },
  { 0xbb, 0, split,  0,   0 },
  { 0xbc, 3, inst,   0,   "ldy %a,X" },
  { 0xbd, 3, inst,   0,   "lda %a,X" },
  { 0xbe, 3, inst,   0,   "ldx %a,Y" },
  { 0xbf, 0, split,  0,   0 },
  { 0xc0, 2, inst,   0,   "cpy %1" },
  { 0xc1, 2, inst,   0,   "cmp (%0,X)" },
  { 0xc2, 0, split,  0,   0 },
  { 0xc3, 0, split,  0,   0 },
  { 0xc4, 2, inst,   0,   "cpy %0" },
  { 0xc5, 2, inst,   0,   "cmp %0" },
  { 0xc6, 2, inst,   0,   "dec %0" },
  { 0xc7, 0, split,  0,   0 },
  { 0xc8, 1, inst,   0,   "iny" },
  { 0xc9, 2, inst,   0,   "cmp %1" },
  { 0xca, 1, inst,   0,   "dex" },
  { 0xcb, 0, split,  0,   0 },
  { 0xcc, 3, inst,   0,   "cpy %a" },
  { 0xcd, 3, inst,   0,   "cmp %a" },
  { 0xce, 3, inst,   0,   "dec %a" },
  { 0xcf, 0, split,  0,   0 },
  { 0xd0, 2, pushj,  0,   "bne %d" },
  { 0xd1, 2, inst,   0,   "cmp (%0),y" },
  { 0xd2, 2, inst,   C02, "cmp (%0)" },
  { 0xd3, 0, split,  0,   0 },
  { 0xd4, 0, split,  0,   0 },
  { 0xd5, 2, inst,   0,   "cmp %0,X" },
  { 0xd6, 2, inst,   0,   "dec %0,X" },
  { 0xd7, 0, split,  0,   0 },
  { 0xd8, 1, inst,   0,   "cld" },
  { 0xd9, 3, inst,   0,   "cmp %a,Y" },
  { 0xda, 1, inst,   C02, "phx" },
  { 0xdb, 0, split,  0,   0 },
  { 0xdc, 0, split,  0,   0 },
  { 0xdd, 3, inst,   0,   "cmp %a,X" },
  { 0xde, 3, inst,   0,   "dec %a,X" },
  { 0xdf, 0, split,  0,   0 },
  { 0xe0, 2, inst,   0,   "cpx %1" },
  { 0xe1, 2, inst,   0,   "sbc (%0,X)" },
  { 0xe2, 0, split,  0,   0 },
  { 0xe3, 0, split,  0,   0 },
  { 0xe4, 2, inst,   0,   "cpx %0" },
  { 0xe5, 2, inst,   0,   "sbc %0" },
  { 0xe6, 2, inst,   0,   "inc %0" },
  { 0xe7, 0, split,  0,   0 },
  { 0xe8, 1, inst,   0,   "inx" },
  { 0xe9, 2, inst,   0,   "sbc %1" },
  { 0xea, 1, inst,   0,   "nop" },
  { 0xeb, 0, split,  0,   0 },
  { 0xec, 3, inst,   0,   "cpx %a" },
  { 0xed, 3, inst,   0,   "sbc %a" },
  { 0xee, 3, inst,   0,   "inc %a" },
  { 0xef, 0, split,  0,   0 },
  { 0xf0, 2, pushj,  0,   "beq %d" },
  { 0xf1, 2, inst,   0,   "sbc (%0),y" },
  { 0xf2, 2, inst,   C02, "sbc (%0)" },
  { 0xf3, 0, split,  0,   0 },
  { 0xf4, 0, split,  0,   0 },
  { 0xf5, 2, inst,   0,   "sbc %0,X" },
  { 0xf6, 2, inst,   0,   "inc %0,X" },
  { 0xf7, 0, split,  0,   0 },
  { 0xf8, 1, inst,   0,   "sed" },
  { 0xf9, 3, inst,   0,   "sbc %a,Y" },
  { 0xfa, 1, inst,   C02, "plx" },
  { 0xfb, 0, split,  0,   0 },
  { 0xfc, 0, split,  0,   0 },
  { 0xfd, 3, inst,   0,   "sbc %a,X" },
  { 0xfe, 3, inst,   0,   "inc %a,X" },
  { 0xff, 0, split,  0,   0 },
};

/*
 * Bit instructions for 65C02 versions which have them.
 * (this means 65C02 devices from Rockwell/WDC)
 */

static dispblock bit6502disp[] = {
  { 0x07, 2, inst,   0,   "rmb0 %0" },
  { 0x0f, 3, pushj,  0,   "bbr0 %0,%d" },
  { 0x17, 2, inst,   0,   "rmb1 %0" },
  { 0x1f, 3, pushj,  0,   "bbr1 %0,%d" },
  { 0x27, 2, inst,   0,   "rmb2 %0" },
  { 0x2f, 3, pushj,  0,   "bbr2 %0,%d" },
  { 0x37, 2, inst,   0,   "rmb3 %0" },
  { 0x3f, 3, pushj,  0,   "bbr3 %0,%d" },
  { 0x47, 2, inst,   0,   "rmb4 %0" },
  { 0x4f, 3, pushj,  0,   "bbr4 %0,%d" },
  { 0x57, 2, inst,   0,   "rmb5 %0" },
  { 0x5f, 3, pushj,  0,   "bbr5 %0,%d" },
  { 0x67, 2, inst,   0,   "rmb6 %0" },
  { 0x6f, 3, pushj,  0,   "bbr6 %0,%d" },
  { 0x77, 2, inst,   0,   "rmb7 %0" },
  { 0x7f, 3, pushj,  0,   "bbr7 %0,%d" },
  { 0x87, 2, inst,   0,   "smb0 %0" },
  { 0x8f, 3, pushj,  0,   "bbs0 %0,%d" },
  { 0x97, 2, inst,   0,   "smb1 %0" },
  { 0x9f, 3, pushj,  0,   "bbs1 %0,%d" },
  { 0xa7, 2, inst,   0,   "smb2 %0" },
  { 0xaf, 3, pushj,  0,   "bbs2 %0,%d" },
  { 0xb7, 2, inst,   0,   "smb3 %0" },
  { 0xbf, 3, pushj,  0,   "bbs3 %0,%d" },
  { 0xc7, 2, inst,   0,   "smb4 %0" },
  { 0xcf, 3, pushj,  0,   "bbs4 %0,%d" },
  { 0xd7, 2, inst,   0,   "smb5 %0" },
  { 0xdf, 3, pushj,  0,   "bbs5 %0,%d" },
  { 0xe7, 2, inst,   0,   "smb6 %0" },
  { 0xef, 3, pushj,  0,   "bbs6 %0,%d" },
  { 0xf7, 2, inst,   0,   "smb7 %0" },
  { 0xff, 3, pushj,  0,   "bbs7 %0,%d" },
  { 0x00, 0, arnold, 0, NULL },
};

/*
 * Extended instructions for 65C816 devices.
 */

static dispblock m65816disp[] = {
  { 0x02, 2, unused, 0,   "cop %1" },
  { 0x03, 0, unused, 0,   "ora d,s" },
  { 0x07, 0, unused, 0,   "ora [d]" },
  { 0x0b, 1, inst,   0,   "phd" },
  { 0x0f, 0, unused, 0,   "ora al" },
  { 0x13, 0, unused, 0,   "ora (d,s),y" },
  { 0x17, 0, unused, 0,   "ora [d],y" },
  { 0x1b, 1, inst,   0,   "tcs" },
  { 0x1f, 0, unused, 0,   "ora al,x" },
  { 0x22, 0, unused, 0,   "jsl al" },
  { 0x23, 0, unused, 0,   "and d,s" },
  { 0x27, 0, unused, 0,   "and [d]" },
  { 0x2b, 1, inst,   0,   "pld" },
  { 0x2f, 0, unused, 0,   "and al" },
  { 0x33, 0, unused, 0,   "and (d,s),y" },
  { 0x37, 0, unused, 0,   "and [d],y" },
  { 0x3b, 1, inst,   0,   "tsc" },
  { 0x3f, 0, unused, 0,   "and al,x" },
  { 0x42, 1, inst,   0,   "wdm" },
  { 0x43, 0, unused, 0,   "eor d,s" },
  { 0x44, 0, unused, 0,   "mvp s,d" },
  { 0x47, 0, unused, 0,   "eor [d]" },
  { 0x4b, 1, inst,   0,   "phk" },
  { 0x4f, 0, unused, 0,   "eor al" },
  { 0x53, 0, unused, 0,   "eor (d,s),y" },
  { 0x54, 0, unused, 0,   "mvn s,d" },
  { 0x57, 0, unused, 0,   "eor [d],y" },
  { 0x5b, 1, inst,   0,   "tcd" },
  { 0x5c, 0, unused, 0,   "jmp al" },
  { 0x5f, 0, unused, 0,   "eor al,x" },
  { 0x62, 0, unused, 0,   "per rl" },
  { 0x63, 0, unused, 0,   "adc d,s" },
  { 0x67, 0, unused, 0,   "adc [d]" },
  { 0x6b, 1, inst,   0,   "rtl" },
  { 0x6f, 0, unused, 0,   "adc al" },
  { 0x73, 0, unused, 0,   "adc (d,s),y" },
  { 0x77, 0, unused, 0,   "adc [d],y" },
  { 0x7b, 1, inst,   0,   "tdc" },
  { 0x7f, 0, unused, 0,   "adc al,x" },
  { 0x82, 0, unused, 0,   "brl rl" },
  { 0x83, 0, unused, 0,   "sta d,s" },
  { 0x87, 0, unused, 0,   "sta [d]" },
  { 0x8b, 1, inst,   0,   "phb" },
  { 0x8f, 0, unused, 0,   "sta al" },
  { 0x93, 0, unused, 0,   "sta (d,s),y" },
  { 0x97, 0, unused, 0,   "sta [d],y" },
  { 0x9b, 1, inst,   0,   "txy" },
  { 0x9f, 0, unused, 0,   "sta al,x" },
  { 0xa3, 0, unused, 0,   "lda d,s" },
  { 0xa7, 0, unused, 0,   "lda [d]" },
  { 0xab, 1, inst,   0,   "plb" },
  { 0xaf, 0, unused, 0,   "lda al" },
  { 0xb3, 0, unused, 0,   "lda (d,s),y" },
  { 0xb7, 0, unused, 0,   "lda [d],y" },
  { 0xbb, 1, inst,   0,   "tyx" },
  { 0xbf, 0, unused, 0,   "lda al,x" },
  { 0xc2, 2, unused, 0,   "rep %1" },
  { 0xc3, 0, unused, 0,   "cmp d,s" },
  { 0xc7, 0, unused, 0,   "cmp [d]" },
  { 0xcb, 1, inst,   0,   "wai" },
  { 0xcf, 0, unused, 0,   "cmp al" },
  { 0xd3, 0, unused, 0,   "cmp (d,s),y" },
  { 0xd4, 2, unused, 0,   "pei %0" },
  { 0xd7, 0, unused, 0,   "cmp [d],y" },
  { 0xdb, 1, inst,   0,   "stp" },
  { 0xdc, 0, unused, 0,   "jml (a)" },
  { 0xdf, 0, unused, 0,   "cmp al,x" },
  { 0xe2, 2, unused, 0,   "sep %1" },
  { 0xe3, 0, unused, 0,   "sbc d,s" },
  { 0xe7, 0, unused, 0,   "sbc [d]" },
  { 0xeb, 1, inst,   0,   "xba" },
  { 0xef, 0, unused, 0,   "sbc al" },
  { 0xf3, 0, unused, 0,   "sbc (d,s),y" },
  { 0xf4, 3, unused, 0,   "pea %a" },
  { 0xf7, 0, unused, 0,   "sbc [d],y" },
  { 0xfb, 1, inst,   0,   "xce" },
  { 0xfc, 3, unused, 0,   "jsr (%a,X)" },
  { 0xff, 0, unused, 0,   "sbc al,x" },
  { 0x00, 0, arnold, 0, NULL },
};

/*
** Start of our local variables:
*/

static address* touch;		/* Where in memory this instruction refers */

static byte opcode;		/* Opcode byte. */

static int model;		/* Processor sub-model. */
#  define mod_6502       1	/* Original 6502. */
#  define mod_65c02      2	/* CMOS (extended) version. */
#  define mod_65c02_rw   3	/* Rockwell/WDC bit instructions. */
#  define mod_65816      4	/* 65816. */

/**********************************************************************/

static void number(word w)
{
  switch (radix) {
    /*
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
    */
  case 10:
    bufdecimal(w, 1);
    break;
  case 16:
  default:
    bufchar('$');
    bufhex(w, 1);
    break;
  }
}

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

static void m6502_dobyte(void)
{
  defb(getbyte());
}

static void m6502_dochar(void)
{
  byte b;

  b = getmemory(istart);

  if (printable(b)) {
    pb_length = 1;
    pb_status = st_char;
    startline(true);
    casestring("defb");
    spacedelim();
    bufchar('\'');
    bufchar((char) b);
    bufchar('\'');
    checkblank();
  } else {
    defb(b);
  }
}

static void m6502_doword(void)
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

static void m6502_doptr(void)
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

static void genlabel(word w)
{
  char cbuf[20];

  sprintf(cbuf, "l_%04" PRIxw, w);
  l_insert(a_l2a(w), cbuf);
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

static void copytext(char* p)
{
  char c;

  while ((c = *(p++)) != (char) 0) {
    if (c == '%') {
      c = *(p++);
      switch (c) {
      case '0':			/* Zero-page address. */
	refaddr(getbyte());
	break;
      case '1':			/* One byte of data */
	bufchar('#');
	number(getbyte());
        break;
      case 'a':			/* Absolute address */
        refaddr(getword());
        break;
      case 'd':			/* Relative address */
        refdisp(getdisp());
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

static void m6502_doinstr(void)
{
  dispblock* disp;

  opcode = getbyte();
  disp = &m6502disp[opcode];

  if (disp->itype == split) {
    switch (model) {
    case mod_65c02_rw:
      disp = finddisp(opcode, bit6502disp, NULL);
      break;
    case mod_65816:
      disp = finddisp(opcode, m65816disp, NULL);
      break;
    default:
      disp = NULL;
    }
  }

  if (disp == NULL || disp->itype == unused) {
    defb(opcode);
    return;
  }

  if (disp->flags & C02 && model == mod_6502) {
    defb(opcode);
    return;
  }

  pb_length = disp->length;

  startline(true);

  copytext(disp->expand);

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

static void m6502_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    m6502_dochar();
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

void m6502_begin(void)
{
  bufstring(";Beginning of program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

void m6502_end(void)
{
  bufblankline();
  bufstring(";End of program");
}

void m6502_org(address* a)
{
  bufblankline();
  casestring("org");
  spacedelim();
  number(a_a2l(a));
  bufblankline();
}
/**********************************************************************/

char* m6502_lcan(char* name)
{
  static char work[10];

  return canonicalize(name, work, 6);
}

bool m6502_lchk(char* name)
{
  return checkstring(name, "", "0123456789");
}

void m6502_lgen(address* addr)
{
  genlabel(a_a2w(addr));
}

/**********************************************************************/

static void setpf(void)
{
  /* Set up our functions: */

  spf_lcan(m6502_lcan);
  spf_lchk(m6502_lchk);
  spf_lgen(m6502_lgen);

  /* set up our object handlers: */

  spf_dodef(m6502_dobyte);

  spf_doobj(st_inst, m6502_doinstr);
  spf_doobj(st_byte, m6502_dobyte);
  spf_doobj(st_word, m6502_doword);
  spf_doobj(st_ptr,  m6502_doptr);
  spf_doobj(st_char, m6502_dochar);
  spf_doobj(st_text, m6502_dotext);

  spf_begin(m6502_begin);
  spf_end(m6502_end);
  spf_org(m6502_org);

  /* set up our variables: */

  pv_bpa = 1;                   /* Bytes per Address unit. */
  pv_bpl = 4;                   /* Bytes per line. */
  pv_abits = 16;                /* Number of address bits. */
  pv_bigendian = false;         /* We are little-endian. */
}

/*
** 6502 -- MOS 6502 CPU.
*/

void m6502_init(void)
{
  model = mod_6502;
  setpf();
}

bool m6502_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for 6502 processor.\n\
");
    return true;
  }
  return false;
}

/*
** 65C02 -- MOS 65C02 CPU.
*/

void m65c02_init(void)
{
  model = mod_65c02;
  setpf();
}

bool m65c02_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for 65C02 processor.\n\
");
    return true;
  }
  return false;
}

/*
** 65C02-RW -- MOS 6502 CPU with Rockwell/WDC bit instructions.
*/

void m65c02_rw_init(void)
{
  model = mod_65c02_rw;
  setpf();
}

bool m65c02_rw_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for 65C02-RW processor.\n\
");
    return true;
  }
  return false;
}

/*
** 65816.
*/

void m65816_init(void)
{
  model = mod_65816;
  setpf();
}

bool m65816_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for 65816 processor.\n\
");
    return true;
  }
  return false;
}
