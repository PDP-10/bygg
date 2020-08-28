/*
** This module implements driver code for the Intel 8080/8085 processors,
** as well as the Zilog Z80 etc.
*/

#include "disass.h"

/**********************************************************************/

evf_init z80_init;
evf_help z80_help;

struct entryvector z80_vector = {
  "z80",			/* Name */
  "Zilog Z80",			/* One-liner. */
  z80_init,			/* Init routine */
  z80_help,			/* Help routine */
};

/**********************************************************************/

evf_init i8080_init;
evf_help i8080_help;

struct entryvector i8080_vector = {
  "8080",			/* Name */
  "Intel 8080",			/* One-liner. */
  i8080_init,			/* Init routine */
  i8080_help,			/* Help routine */
};

/**********************************************************************/

evf_init i8085_init;
evf_help i8085_help;

struct entryvector i8085_vector = {
  "8085",			/* Name */
  "Intel 8085",			/* One-liner. */
  i8085_init,			/* Init routine */
  i8085_help,			/* Help routine */
};

/**********************************************************************/

evf_init h64180_init;
evf_help h64180_help;

struct entryvector h64180_vector = {
  "64180",			/* Name */
  "Hitachi 64180",		/* One-liner. */
  h64180_init,			/* Init routine */
  h64180_help,			/* Help routine */
};

/**********************************************************************/

/* Dispatch tables: */

/* itype values: */

#define unused  0
#define inst    1
#define instb   2
#define instw   3
#define jrst    4
#define pushj   5
#define popj    6
#define pfxCB   7
#define pfxDD   8
#define pfxED   9
#define pfxFD  10
#define trap   11		/* RST nn */

/* flags: */

#define H64	0x01		/* 64180 only instr. */
#define I85     0x02		/* 8085 instruction. */

/* ---- Intel opcode table ---- */

static dispblock inteldisp[256] = {
  { 0x00, 1, inst,   0,   "nop" },
  { 0x01, 3, inst,   0,   "lxi b,%2" },
  { 0x02, 1, inst,   0,   "stax b" },
  { 0x03, 1, inst,   0,   "inx b" },
  { 0x04, 1, inst,   0,   "inr b" },
  { 0x05, 1, inst,   0,   "dcr b" },
  { 0x06, 2, inst,   0,   "mvi b,%c" },
  { 0x07, 1, inst,   0,   "rlc" },
  { 0x08, 1, unused, 0,   0 },
  { 0x09, 1, inst,   0,   "dad b" },
  { 0x0A, 1, inst,   0,   "ldax b" },
  { 0x0B, 1, inst,   0,   "dcx b" },
  { 0x0C, 1, inst,   0,   "inr c" },
  { 0x0D, 1, inst,   0,   "dcr c" },
  { 0x0E, 2, inst,   0,   "mvi c,%c" },
  { 0x0F, 1, inst,   0,   "rrc" },
  { 0x10, 2, unused, 0,   0 },
  { 0x11, 3, inst,   0,   "lxi d,%2" },
  { 0x12, 1, inst,   0,   "stax d" },
  { 0x13, 1, inst,   0,   "inx d" },
  { 0x14, 1, inst,   0,   "inr d" },
  { 0x15, 1, inst,   0,   "dcr d" },
  { 0x16, 2, inst,   0,   "mvi d,%1" },
  { 0x17, 1, inst,   0,   "ral" },
  { 0x18, 2, unused, 0,   0 },
  { 0x19, 1, inst,   0,   "dad d" },
  { 0x1A, 1, inst,   0,   "ldax d" },
  { 0x1B, 1, inst,   0,   "dcx d" },
  { 0x1C, 1, inst,   0,   "inr e" },
  { 0x1D, 1, inst,   0,   "dcr e" },
  { 0x1E, 2, inst,   0,   "mvi e,%c" },
  { 0x1F, 1, inst,   0,   "rar" },
  { 0x20, 1, inst,   I85, "rim" },
  { 0x21, 3, inst,   0,   "lxi h,%2" },
  { 0x22, 3, instw,  0,   "shld %a" },
  { 0x23, 1, inst,   0,   "inx h" },
  { 0x24, 1, inst,   0,   "inr h" },
  { 0x25, 1, inst,   0,   "dcr h" },
  { 0x26, 2, inst,   0,   "mvi h,%c" },
  { 0x27, 1, inst,   0,   "daa" },
  { 0x28, 2, unused, 0,   0 },
  { 0x29, 1, inst,   0,   "dad h" },
  { 0x2A, 3, instw,  0,   "lhld %a" },
  { 0x2B, 1, inst,   0,   "dcx h" },
  { 0x2C, 1, inst,   0,   "inr l" },
  { 0x2D, 1, inst,   0,   "dcr l" },
  { 0x2E, 2, inst,   0,   "mvi l,%c" },
  { 0x2F, 1, inst,   0,   "cma" },
  { 0x30, 1, inst,   I85, "sim" },
  { 0x31, 3, inst,   0,   "lxi sp,%2" },
  { 0x32, 3, instb,  0,   "sta %a" },
  { 0x33, 1, inst,   0,   "inx sp" },
  { 0x34, 1, inst,   0,   "inr m" },
  { 0x35, 1, inst,   0,   "dcr m" },
  { 0x36, 2, inst,   0,   "mvi m,%1" },
  { 0x37, 1, inst,   0,   "stc" },
  { 0x38, 2, unused, 0,   0 },
  { 0x39, 1, inst,   0,   "dad sp" },
  { 0x3A, 3, instb,  0,   "lda %a" },
  { 0x3B, 1, inst,   0,   "dcr sp" },
  { 0x3C, 1, inst,   0,   "inr a" },
  { 0x3D, 1, inst,   0,   "dcr a" },
  { 0x3E, 2, inst,   0,   "mvi a,%1" },
  { 0x3F, 1, inst,   0,   "cmc" },
  { 0x40, 1, inst,   0,   "mov b,b" },
  { 0x41, 1, inst,   0,   "mov b,c" },
  { 0x42, 1, inst,   0,   "mov b,d" },
  { 0x43, 1, inst,   0,   "mov b,e" },
  { 0x44, 1, inst,   0,   "mov b,h" },
  { 0x45, 1, inst,   0,   "mov b,l" },
  { 0x46, 1, inst,   0,   "mov b,m" },
  { 0x47, 1, inst,   0,   "mov b,a" },
  { 0x48, 1, inst,   0,   "mov c,b" },
  { 0x49, 1, inst,   0,   "mov c,c" },
  { 0x4A, 1, inst,   0,   "mov c,d" },
  { 0x4B, 1, inst,   0,   "mov c,e" },
  { 0x4C, 1, inst,   0,   "mov c,h" },
  { 0x4D, 1, inst,   0,   "mov c,l" },
  { 0x4E, 1, inst,   0,   "mov c,m" },
  { 0x4F, 1, inst,   0,   "mov c,a" },
  { 0x50, 1, inst,   0,   "mov d,b" },
  { 0x51, 1, inst,   0,   "mov d,c" },
  { 0x52, 1, inst,   0,   "mov d,d" },
  { 0x53, 1, inst,   0,   "mov d,e" },
  { 0x54, 1, inst,   0,   "mov d,h" },
  { 0x55, 1, inst,   0,   "mov d,l" },
  { 0x56, 1, inst,   0,   "mov d,m" },
  { 0x57, 1, inst,   0,   "mov d,a" },
  { 0x58, 1, inst,   0,   "mov e,b" },
  { 0x59, 1, inst,   0,   "mov e,c" },
  { 0x5A, 1, inst,   0,   "mov e,d" },
  { 0x5B, 1, inst,   0,   "mov e,e" },
  { 0x5C, 1, inst,   0,   "mov e,h" },
  { 0x5D, 1, inst,   0,   "mov e,l" },
  { 0x5E, 1, inst,   0,   "mov e,m" },
  { 0x5F, 1, inst,   0,   "mov e,a" },
  { 0x60, 1, inst,   0,   "mov h,b" },
  { 0x61, 1, inst,   0,   "mov h,c" },
  { 0x62, 1, inst,   0,   "mov h,d" },
  { 0x63, 1, inst,   0,   "mov h,e" },
  { 0x64, 1, inst,   0,   "mov h,h" },
  { 0x65, 1, inst,   0,   "mov h,l" },
  { 0x66, 1, inst,   0,   "mov h,m" },
  { 0x67, 1, inst,   0,   "mov h,a" },
  { 0x68, 1, inst,   0,   "mov l,b" },
  { 0x69, 1, inst,   0,   "mov l,c" },
  { 0x6A, 1, inst,   0,   "mov l,d" },
  { 0x6B, 1, inst,   0,   "mov l,e" },
  { 0x6C, 1, inst,   0,   "mov l,h" },
  { 0x6D, 1, inst,   0,   "mov l,l" },
  { 0x6E, 1, inst,   0,   "mov l,m" },
  { 0x6F, 1, inst,   0,   "mov l,a" },
  { 0x70, 1, inst,   0,   "mov m,b" },
  { 0x71, 1, inst,   0,   "mov m,c" },
  { 0x72, 1, inst,   0,   "mov m,d" },
  { 0x73, 1, inst,   0,   "mov m,e" },
  { 0x74, 1, inst,   0,   "mov m,h" },
  { 0x75, 1, inst,   0,   "mov m,l" },
  { 0x76, 1, inst,   0,   "halt" },
  { 0x77, 1, inst,   0,   "mov m,a" },
  { 0x78, 1, inst,   0,   "mov a,b" },
  { 0x79, 1, inst,   0,   "mov a,c" },
  { 0x7A, 1, inst,   0,   "mov a,d" },
  { 0x7B, 1, inst,   0,   "mov a,e" },
  { 0x7C, 1, inst,   0,   "mov a,h" },
  { 0x7D, 1, inst,   0,   "mov a,l" },
  { 0x7E, 1, inst,   0,   "mov a,m" },
  { 0x7F, 1, inst,   0,   "mov a,a" },
  { 0x80, 1, inst,   0,   "add b" },
  { 0x81, 1, inst,   0,   "add c" },
  { 0x82, 1, inst,   0,   "add d" },
  { 0x83, 1, inst,   0,   "add e" },
  { 0x84, 1, inst,   0,   "add h" },
  { 0x85, 1, inst,   0,   "add l" },
  { 0x86, 1, inst,   0,   "add m" },
  { 0x87, 1, inst,   0,   "add a" },
  { 0x88, 1, inst,   0,   "adc b" },
  { 0x89, 1, inst,   0,   "adc c" },
  { 0x8A, 1, inst,   0,   "adc d" },
  { 0x8B, 1, inst,   0,   "adc e" },
  { 0x8C, 1, inst,   0,   "adc h" },
  { 0x8D, 1, inst,   0,   "adc l" },
  { 0x8E, 1, inst,   0,   "adc m" },
  { 0x8F, 1, inst,   0,   "adc a" },
  { 0x90, 1, inst,   0,   "sub b" },
  { 0x91, 1, inst,   0,   "sub c" },
  { 0x92, 1, inst,   0,   "sub d" },
  { 0x93, 1, inst,   0,   "sub e" },
  { 0x94, 1, inst,   0,   "sub h" },
  { 0x95, 1, inst,   0,   "sub l" },
  { 0x96, 1, inst,   0,   "sub m" },
  { 0x97, 1, inst,   0,   "sub a" },
  { 0x98, 1, inst,   0,   "sbb b" },
  { 0x99, 1, inst,   0,   "sbb c" },
  { 0x9A, 1, inst,   0,   "sbb d" },
  { 0x9B, 1, inst,   0,   "sbb e" },
  { 0x9C, 1, inst,   0,   "sbb h" },
  { 0x9D, 1, inst,   0,   "sbb l" },
  { 0x9E, 1, inst,   0,   "sbb m" },
  { 0x9F, 1, inst,   0,   "sbb a" },
  { 0xA0, 1, inst,   0,   "ana b" },
  { 0xA1, 1, inst,   0,   "ana c" },
  { 0xA2, 1, inst,   0,   "ana d" },
  { 0xA3, 1, inst,   0,   "ana e" },
  { 0xA4, 1, inst,   0,   "ana h" },
  { 0xA5, 1, inst,   0,   "ana l" },
  { 0xA6, 1, inst,   0,   "ana m" },
  { 0xA7, 1, inst,   0,   "ana a" },
  { 0xA8, 1, inst,   0,   "xra b" },
  { 0xA9, 1, inst,   0,   "xra c" },
  { 0xAA, 1, inst,   0,   "xra d" },
  { 0xAB, 1, inst,   0,   "xra e" },
  { 0xAC, 1, inst,   0,   "xra h" },
  { 0xAD, 1, inst,   0,   "xra l" },
  { 0xAE, 1, inst,   0,   "xra m" },
  { 0xAF, 1, inst,   0,   "xra a" },
  { 0xB0, 1, inst,   0,   "ora b" },
  { 0xB1, 1, inst,   0,   "ora c" },
  { 0xB2, 1, inst,   0,   "ora d" },
  { 0xB3, 1, inst,   0,   "ora e" },
  { 0xB4, 1, inst,   0,   "ora h" },
  { 0xB5, 1, inst,   0,   "ora l" },
  { 0xB6, 1, inst,   0,   "ora m" },
  { 0xB7, 1, inst,   0,   "ora a" },
  { 0xB8, 1, inst,   0,   "cmp b" },
  { 0xB9, 1, inst,   0,   "cmp c" },
  { 0xBA, 1, inst,   0,   "cmp d" },
  { 0xBB, 1, inst,   0,   "cmp e" },
  { 0xBC, 1, inst,   0,   "cmp h" },
  { 0xBD, 1, inst,   0,   "cmp l" },
  { 0xBE, 1, inst,   0,   "cmp m" },
  { 0xBF, 1, inst,   0,   "cmp a" },
  { 0xC0, 1, inst,   0,   "rnz" },
  { 0xC1, 1, inst,   0,   "pop b" },
  { 0xC2, 3, pushj,  0,   "jnz %a" },
  { 0xC3, 3, jrst,   0,   "jmp %a" },
  { 0xC4, 3, pushj,  0,   "cnz %a" },
  { 0xC5, 1, inst,   0,   "push b" },
  { 0xC6, 2, inst,   0,   "adi %1" },
  { 0xC7, 1, trap,   0,   "rst 0" },
  { 0xC8, 1, inst,   0,   "rz" },
  { 0xC9, 1, popj,   0,   "ret" },
  { 0xCA, 3, pushj,  0,   "jz %a" },
  { 0xCB, 0, unused, 0,   0 },
  { 0xCC, 3, pushj,  0,   "cz %a" },
  { 0xCC, 3, pushj,  0,   "call %a" },
  { 0xCE, 2, inst,   0,   "aci %1" },
  { 0xCF, 1, trap,   0,   "rst 1" },
  { 0xD0, 1, inst,   0,   "rnc" },
  { 0xD1, 1, inst,   0,   "pop d" },
  { 0xD2, 3, pushj,  0,   "jnc %a" },
  { 0xD3, 2, inst,   0,   "out %1" },
  { 0xD4, 3, pushj,  0,   "cnc %a" },
  { 0xD5, 1, inst,   0,   "push d" },
  { 0xD6, 2, inst,   0,   "sui %1" },
  { 0xD7, 1, trap,   0,   "rst 2" },
  { 0xD8, 1, inst,   0,   "rc" },
  { 0xD9, 1, unused, 0,   0 },
  { 0xDA, 3, pushj,  0,   "jc %a" },
  { 0xDB, 2, inst,   0,   "in %1" },
  { 0xDC, 3, pushj,  0,   "cc %a" },
  { 0xDD, 0, unused, 0,   0 },
  { 0xDE, 2, inst,   0,   "sbi %1" },
  { 0xDF, 1, trap,   0,   "rst 3" },
  { 0xE0, 1, inst,   0,   "rpo" },
  { 0xE1, 1, inst,   0,   "pop h" },
  { 0xE2, 3, pushj,  0,   "jpo %a" },
  { 0xE3, 1, inst,   0,   "xthl" },
  { 0xE4, 3, pushj,  0,   "cpo %a" },
  { 0xE5, 1, inst,   0,   "push h" },
  { 0xE6, 2, inst,   0,   "ani %1" },
  { 0xE7, 1, trap,   0,   "rst 4" },
  { 0xE8, 1, inst,   0,   "rpe" },
  { 0xE9, 1, popj,   0,   "pchl" },
  { 0xEA, 3, pushj,  0,   "jpe %a" },
  { 0xEB, 1, inst,   0,   "xchg" },
  { 0xEC, 3, pushj,  0,   "cpe %a" },
  { 0xED, 0, unused, 0,   0 },
  { 0xEE, 2, inst,   0,   "xri %1" },
  { 0xEF, 1, trap,   0,   "rst 5" },
  { 0xF0, 1, inst,   0,   "rp" },
  { 0xF1, 1, inst,   0,   "pop psw" },
  { 0xF2, 3, pushj,  0,   "jp %a" },
  { 0xF3, 1, inst,   0,   "di" },
  { 0xF4, 3, pushj,  0,   "cp %a" },
  { 0xF5, 1, inst,   0,   "push psw" },
  { 0xF6, 2, inst,   0,   "ori %1" },
  { 0xF7, 1, trap,   0,   "rst 6" },
  { 0xF8, 1, inst,   0,   "rm" },
  { 0xF9, 1, inst,   0,   "sphl" },
  { 0xFA, 3, pushj,  0,   "jm %a" },
  { 0xFB, 1, inst,   0,   "ei" },
  { 0xFC, 3, pushj,  0,   "cm %a" },
  { 0xFD, 0, unused, 0,    0 },
  { 0xFE, 2, inst,   0,   "cpi %1" },
  { 0xFF, 1, trap,   0,   "rst 7" },
};

/* ---- zilog opcode table ---- */

static dispblock zilogdisp[256] = {
  { 0x00, 1, inst,   0,   "nop" },
  { 0x01, 3, inst,   0,   "ld bc,%2" },
  { 0x02, 1, inst,   0,   "ld (bc),a" },
  { 0x03, 1, inst,   0,   "inc bc" },
  { 0x04, 1, inst,   0,   "inc b" },
  { 0x05, 1, inst,   0,   "dec b" },
  { 0x06, 2, inst,   0,   "ld b,%c" },
  { 0x07, 1, inst,   0,   "rlca" },
  { 0x08, 1, inst,   0,   "ex af,af'" },
  { 0x09, 1, inst,   0,   "add hl,bc" },
  { 0x0A, 1, inst,   0,   "ld a,(bc)" },
  { 0x0B, 1, inst,   0,   "dec bc" },
  { 0x0C, 1, inst,   0,   "inc c" },
  { 0x0D, 1, inst,   0,   "dec c" },
  { 0x0E, 2, inst,   0,   "ld c,%c" },
  { 0x0F, 1, inst,   0,   "rrca" },
  { 0x10, 2, pushj,  0,   "djnz %d" },
  { 0x11, 3, inst,   0,   "ld de,%2" },
  { 0x12, 1, inst,   0,   "ld (de),a" },
  { 0x13, 1, inst,   0,   "inc de" },
  { 0x14, 1, inst,   0,   "inc d" },
  { 0x15, 1, inst,   0,   "dec d" },
  { 0x16, 2, inst,   0,   "ld d,%c" },
  { 0x17, 1, inst,   0,   "rla" },
  { 0x18, 2, jrst,   0,   "jr %d" },
  { 0x19, 1, inst,   0,   "add hl,de" },
  { 0x1A, 1, inst,   0,   "ld a,(de)" },
  { 0x1B, 1, inst,   0,   "dec de" },
  { 0x1C, 1, inst,   0,   "inc e" },
  { 0x1D, 1, inst,   0,   "dec e" },
  { 0x1E, 2, inst,   0,   "ld e,%c" },
  { 0x1F, 1, inst,   0,   "rra" },
  { 0x20, 2, pushj,  0,   "jr nz,%d" },
  { 0x21, 3, inst,   0,   "ld hl,%2" },
  { 0x22, 3, instw,  0,   "ld (%a),hl" },
  { 0x23, 1, inst,   0,   "inc hl" },
  { 0x24, 1, inst,   0,   "inc h" },
  { 0x25, 1, inst,   0,   "dec h" },
  { 0x26, 2, inst,   0,   "ld h,%c" },
  { 0x27, 1, inst,   0,   "daa" },
  { 0x28, 2, pushj,  0,   "jr z,%d" },
  { 0x29, 1, inst,   0,   "add hl,hl" },
  { 0x2A, 3, instw,  0,   "ld hl,(%a)" },
  { 0x2B, 1, inst,   0,   "dec hl" },
  { 0x2C, 1, inst,   0,   "inc l" },
  { 0x2D, 1, inst,   0,   "dec l" },
  { 0x2E, 2, inst,   0,   "ld l,%c" },
  { 0x2F, 1, inst,   0,   "cpl" },
  { 0x30, 2, pushj,  0,   "jr nc,%d" },
  { 0x31, 3, inst,   0,   "ld sp,%2" },
  { 0x32, 3, instb,  0,   "ld (%a),a" },
  { 0x33, 1, inst,   0,   "inc sp" },
  { 0x34, 1, inst,   0,   "inc (hl)" },
  { 0x35, 1, inst,   0,   "dec (hl)" },
  { 0x36, 2, inst,   0,   "ld (hl),%1" },
  { 0x37, 1, inst,   0,   "scf" },
  { 0x38, 2, pushj,  0,   "jr c,%d" },
  { 0x39, 1, inst,   0,   "add hl,sp" },
  { 0x3A, 3, instb,  0,   "ld a,(%a)" },
  { 0x3B, 1, inst,   0,   "dec sp" },
  { 0x3C, 1, inst,   0,   "inc a" },
  { 0x3D, 1, inst,   0,   "dec a" },
  { 0x3E, 2, inst,   0,   "ld a,%c" },
  { 0x3F, 1, inst,   0,   "ccf" },
  { 0x40, 1, inst,   0,   "ld b,b" },
  { 0x41, 1, inst,   0,   "ld b,c" },
  { 0x42, 1, inst,   0,   "ld b,d" },
  { 0x43, 1, inst,   0,   "ld b,e" },
  { 0x44, 1, inst,   0,   "ld b,h" },
  { 0x45, 1, inst,   0,   "ld b,l" },
  { 0x46, 1, inst,   0,   "ld b,(hl)" },
  { 0x47, 1, inst,   0,   "ld b,a" },
  { 0x48, 1, inst,   0,   "ld c,b" },
  { 0x49, 1, inst,   0,   "ld c,c" },
  { 0x4A, 1, inst,   0,   "ld c,d" },
  { 0x4B, 1, inst,   0,   "ld c,e" },
  { 0x4C, 1, inst,   0,   "ld c,h" },
  { 0x4D, 1, inst,   0,   "ld c,l" },
  { 0x4E, 1, inst,   0,   "ld c,(hl)" },
  { 0x4F, 1, inst,   0,   "ld c,a" },
  { 0x50, 1, inst,   0,   "ld d,b" },
  { 0x51, 1, inst,   0,   "ld d,c" },
  { 0x52, 1, inst,   0,   "ld d,d" },
  { 0x53, 1, inst,   0,   "ld d,e" },
  { 0x54, 1, inst,   0,   "ld d,h" },
  { 0x55, 1, inst,   0,   "ld d,l" },
  { 0x56, 1, inst,   0,   "ld d,(hl)" },
  { 0x57, 1, inst,   0,   "ld d,a" },
  { 0x58, 1, inst,   0,   "ld e,b" },
  { 0x59, 1, inst,   0,   "ld e,c" },
  { 0x5A, 1, inst,   0,   "ld e,d" },
  { 0x5B, 1, inst,   0,   "ld e,e" },
  { 0x5C, 1, inst,   0,   "ld e,h" },
  { 0x5D, 1, inst,   0,   "ld e,l" },
  { 0x5E, 1, inst,   0,   "ld e,(hl)" },
  { 0x5F, 1, inst,   0,   "ld e,a" },
  { 0x60, 1, inst,   0,   "ld h,b" },
  { 0x61, 1, inst,   0,   "ld h,c" },
  { 0x62, 1, inst,   0,   "ld h,d" },
  { 0x63, 1, inst,   0,   "ld h,e" },
  { 0x64, 1, inst,   0,   "ld h,h" },
  { 0x65, 1, inst,   0,   "ld h,l" },
  { 0x66, 1, inst,   0,   "ld h,(hl)" },
  { 0x67, 1, inst,   0,   "ld h,a" },
  { 0x68, 1, inst,   0,   "ld l,b" },
  { 0x69, 1, inst,   0,   "ld l,c" },
  { 0x6A, 1, inst,   0,   "ld l,d" },
  { 0x6B, 1, inst,   0,   "ld l,e" },
  { 0x6C, 1, inst,   0,   "ld l,h" },
  { 0x6D, 1, inst,   0,   "ld l,l" },
  { 0x6E, 1, inst,   0,   "ld l,(hl)" },
  { 0x6F, 1, inst,   0,   "ld l,a" },
  { 0x70, 1, inst,   0,   "ld (hl),b" },
  { 0x71, 1, inst,   0,   "ld (hl),c" },
  { 0x72, 1, inst,   0,   "ld (hl),d" },
  { 0x73, 1, inst,   0,   "ld (hl),e" },
  { 0x74, 1, inst,   0,   "ld (hl),h" },
  { 0x75, 1, inst,   0,   "ld (hl),l" },
  { 0x76, 1, inst,   0,   "halt" },
  { 0x77, 1, inst,   0,   "ld (hl),a" },
  { 0x78, 1, inst,   0,   "ld a,b" },
  { 0x79, 1, inst,   0,   "ld a,c" },
  { 0x7A, 1, inst,   0,   "ld a,d" },
  { 0x7B, 1, inst,   0,   "ld a,e" },
  { 0x7C, 1, inst,   0,   "ld a,h" },
  { 0x7D, 1, inst,   0,   "ld a,l" },
  { 0x7E, 1, inst,   0,   "ld a,(hl)" },
  { 0x7F, 1, inst,   0,   "ld a,a" },
  { 0x80, 1, inst,   0,   "add a,b" },
  { 0x81, 1, inst,   0,   "add a,c" },
  { 0x82, 1, inst,   0,   "add a,d" },
  { 0x83, 1, inst,   0,   "add a,e" },
  { 0x84, 1, inst,   0,   "add a,h" },
  { 0x85, 1, inst,   0,   "add a,l" },
  { 0x86, 1, inst,   0,   "add a,(hl)" },
  { 0x87, 1, inst,   0,   "add a,a" },
  { 0x88, 1, inst,   0,   "adc a,b" },
  { 0x89, 1, inst,   0,   "adc a,c" },
  { 0x8A, 1, inst,   0,   "adc a,d" },
  { 0x8B, 1, inst,   0,   "adc a,e" },
  { 0x8C, 1, inst,   0,   "adc a,h" },
  { 0x8D, 1, inst,   0,   "adc a,l" },
  { 0x8E, 1, inst,   0,   "adc a,(hl)" },
  { 0x8F, 1, inst,   0,   "adc a,a" },
  { 0x90, 1, inst,   0,   "sub b" },
  { 0x91, 1, inst,   0,   "sub c" },
  { 0x92, 1, inst,   0,   "sub d" },
  { 0x93, 1, inst,   0,   "sub e" },
  { 0x94, 1, inst,   0,   "sub h" },
  { 0x95, 1, inst,   0,   "sub l" },
  { 0x96, 1, inst,   0,   "sub (hl)" },
  { 0x97, 1, inst,   0,   "sub a" },
  { 0x98, 1, inst,   0,   "sbc a,b" },
  { 0x99, 1, inst,   0,   "sbc a,c" },
  { 0x9A, 1, inst,   0,   "sbc a,d" },
  { 0x9B, 1, inst,   0,   "sbc a,e" },
  { 0x9C, 1, inst,   0,   "sbc a,h" },
  { 0x9D, 1, inst,   0,   "sbc a,l" },
  { 0x9E, 1, inst,   0,   "sbc a,(hl)" },
  { 0x9F, 1, inst,   0,   "sbc a,a" },
  { 0xA0, 1, inst,   0,   "and b" },
  { 0xA1, 1, inst,   0,   "and c" },
  { 0xA2, 1, inst,   0,   "and d" },
  { 0xA3, 1, inst,   0,   "and e" },
  { 0xA4, 1, inst,   0,   "and h" },
  { 0xA5, 1, inst,   0,   "and l" },
  { 0xA6, 1, inst,   0,   "and (hl)" },
  { 0xA7, 1, inst,   0,   "and a" },
  { 0xA8, 1, inst,   0,   "xor b" },
  { 0xA9, 1, inst,   0,   "xor c" },
  { 0xAA, 1, inst,   0,   "xor d" },
  { 0xAB, 1, inst,   0,   "xor e" },
  { 0xAC, 1, inst,   0,   "xor h" },
  { 0xAD, 1, inst,   0,   "xor l" },
  { 0xAE, 1, inst,   0,   "xor (hl)" },
  { 0xAF, 1, inst,   0,   "xor a" },
  { 0xB0, 1, inst,   0,   "or b" },
  { 0xB1, 1, inst,   0,   "or c" },
  { 0xB2, 1, inst,   0,   "or d" },
  { 0xB3, 1, inst,   0,   "or e" },
  { 0xB4, 1, inst,   0,   "or h" },
  { 0xB5, 1, inst,   0,   "or l" },
  { 0xB6, 1, inst,   0,   "or (hl)" },
  { 0xB7, 1, inst,   0,   "or a" },
  { 0xB8, 1, inst,   0,   "cp b" },
  { 0xB9, 1, inst,   0,   "cp c" },
  { 0xBA, 1, inst,   0,   "cp d" },
  { 0xBB, 1, inst,   0,   "cp e" },
  { 0xBC, 1, inst,   0,   "cp h" },
  { 0xBD, 1, inst,   0,   "cp l" },
  { 0xBE, 1, inst,   0,   "cp (hl)" },
  { 0xBF, 1, inst,   0,   "cp a" },
  { 0xC0, 1, inst,   0,   "ret nz" },
  { 0xC1, 1, inst,   0,   "pop bc" },
  { 0xC2, 3, pushj,  0,   "jp nz,%a" },
  { 0xC3, 3, jrst,   0,   "jp %a" },
  { 0xC4, 3, pushj,  0,   "call nz,%a" },
  { 0xC5, 1, inst,   0,   "push bc" },
  { 0xC6, 2, inst,   0,   "add a,%1" },
  { 0xC7, 1, trap,   0,   "rst 0" },
  { 0xC8, 1, inst,   0,   "ret z" },
  { 0xC9, 1, popj,   0,   "ret" },
  { 0xCA, 3, pushj,  0,   "jp z,%a" },
  { 0xCB, 2, pfxCB,  0,   0 },
  { 0xCC, 3, pushj,  0,   "call z,%a" },
  { 0xCC, 3, pushj,  0,   "call %a" },
  { 0xCE, 2, inst,   0,   "adc a,%1" },
  { 0xCF, 1, trap,   0,   "rst 8" },
  { 0xD0, 1, inst,   0,   "ret nc" },
  { 0xD1, 1, inst,   0,   "pop de" },
  { 0xD2, 3, pushj,  0,   "jp nc,%a" },
  { 0xD3, 2, inst,   0,   "out (%1),a" },
  { 0xD4, 3, pushj,  0,   "call nc,%a" },
  { 0xD5, 1, inst,   0,   "push de" },
  { 0xD6, 2, inst,   0,   "sub %1" },
  { 0xD7, 1, trap,   0,   "rst 10h" },
  { 0xD8, 1, inst,   0,   "ret c" },
  { 0xD9, 1, inst,   0,   "exx" },
  { 0xDA, 3, pushj,  0,   "jp c,%a" },
  { 0xDB, 2, inst,   0,   "in a,(%1)" },
  { 0xDC, 3, pushj,  0,   "call c,%a" },
  { 0xDD, 0, pfxDD,  0,   0 },
  { 0xDE, 2, inst,   0,   "sbc a,%1" },
  { 0xDF, 1, trap,   0,   "rst 18h" },
  { 0xE0, 1, inst,   0,   "ret po" },
  { 0xE1, 1, inst,   0,   "pop hl" },
  { 0xE2, 3, pushj,  0,   "jp po,%a" },
  { 0xE3, 1, inst,   0,   "ex (sp),hl" },
  { 0xE4, 3, pushj,  0,   "call po,%a" },
  { 0xE5, 1, inst,   0,   "push hl" },
  { 0xE6, 2, inst,   0,   "and %1" },
  { 0xE7, 1, trap,   0,   "rst 20h" },
  { 0xE8, 1, inst,   0,   "ret pe" },
  { 0xE9, 1, popj,   0,   "jp (hl)" },
  { 0xEA, 3, pushj,  0,   "jp pe,%a" },
  { 0xEB, 1, inst,   0,   "ex de,hl" },
  { 0xEC, 3, pushj,  0,   "call pe,%a" },
  { 0xED, 0, pfxED,  0,   0 },
  { 0xEE, 2, inst,   0,   "xor %1" },
  { 0xEF, 1, trap,   0,   "rst 28h" },
  { 0xF0, 1, inst,   0,   "ret p" },
  { 0xF1, 1, inst,   0,   "pop af" },
  { 0xF2, 3, pushj,  0,   "jp p,%a" },
  { 0xF3, 1, inst,   0,   "di" },
  { 0xF4, 3, pushj,  0,   "call p,%a" },
  { 0xF5, 1, inst,   0,   "push af" },
  { 0xF6, 2, inst,   0,   "or %1" },
  { 0xF7, 1, trap,   0,   "rst 30h" },
  { 0xF8, 1, inst,   0,   "ret m" },
  { 0xF9, 1, inst,   0,   "ld sp,hl" },
  { 0xFA, 3, pushj,  0,   "jp m,%a" },
  { 0xFB, 1, inst,   0,   "ei" },
  { 0xFC, 3, pushj,  0,   "call m,%a" },
  { 0xFD, 0, pfxFD,  0,   0 },
  { 0xFE, 2, inst,   0,   "cp %1" },
  { 0xFF, 1, trap,   0,   "rst 38h" },
};

/* ---- DD/FD prefix table ---- */

static dispblock DDdisp[] = {
  { 0x09, 2, inst,   0,   "add %x,bc" },
  { 0x19, 2, inst,   0,   "add %x,de" },
  { 0x21, 4, inst,   0,   "ld %x,%2" },
  { 0x22, 4, instw,  0,   "ld (%a),%x" },
  { 0x23, 2, inst,   0,   "inc %x" },
  { 0x29, 2, inst,   0,   "add %x,%x" },
  { 0x2A, 4, instw,  0,   "ld %x,(%a)" },
  { 0x2B, 2, inst,   0,   "dec %x" },
  { 0x34, 3, inst,   0,   "inc (%y)" },
  { 0x35, 3, inst,   0,   "dec (%y)" },
  { 0x36, 4, inst,   0,   "ld (%y),%1" },
  { 0x39, 2, inst,   0,   "add %x,sp" },
  { 0x46, 3, inst,   0,   "ld b,(%y)" },
  { 0x4E, 3, inst,   0,   "ld c,(%y)" },
  { 0x56, 3, inst,   0,   "ld d,(%y)" },
  { 0x5E, 3, inst,   0,   "ld e,(%y)" },
  { 0x66, 3, inst,   0,   "ld h,(%y)" },
  { 0x6E, 3, inst,   0,   "ld l,(%y)" },
  { 0x70, 3, inst,   0,   "ld (%y),b" },
  { 0x71, 3, inst,   0,   "ld (%y),c" },
  { 0x72, 3, inst,   0,   "ld (%y),d" },
  { 0x73, 3, inst,   0,   "ld (%y),e" },
  { 0x74, 3, inst,   0,   "ld (%y),h" },
  { 0x75, 3, inst,   0,   "ld (%y),l" },
  { 0x77, 3, inst,   0,   "ld (%y),a" },
  { 0x7E, 3, inst,   0,   "ld a,(%y)" },
  { 0x86, 3, inst,   0,   "add a,(%y)" },
  { 0x8E, 3, inst,   0,   "adc a,(%y)" },
  { 0x96, 3, inst,   0,   "sub (%y)" },
  { 0x9E, 3, inst,   0,   "sbc a,(%y)" },
  { 0xA6, 3, inst,   0,   "and (%y)" },
  { 0xAE, 3, inst,   0,   "xor (%y)" },
  { 0xB6, 3, inst,   0,   "or (%y)" },
  { 0xBE, 3, inst,   0,   "cp (%y)" },
  { 0xCB, 4, pfxCB,  0,   0 },
  { 0xE1, 2, inst,   0,   "pop %x" },
  { 0xE3, 2, inst,   0,   "ex (sp),%x" },
  { 0xE5, 2, inst,   0,   "push %x" },
  { 0xE9, 2, popj,   0,   "jp (%x)" },
  { 0xF9, 2, inst,   0,   "ld sp,%x" },
  { 0,    0, arnold, 0,   0 },
};

/* ---- ED table ---- */

static dispblock EDdisp[] = {
  { 0x00, 3, inst,   H64, "in0 b,(%1)" },  /* hd 64180 */
  { 0x01, 3, inst,   H64, "out0 (%1),b" }, /* hd 64180 */
  { 0x04, 2, inst,   H64, "tst b" },       /* hd 64180 */
  { 0x08, 3, inst,   H64, "in0 c,(%1)" },  /* hd 64180 */
  { 0x09, 3, inst,   H64, "out0 (%1),c" }, /* hd 64180 */
  { 0x0C, 2, inst,   H64, "tst c" },       /* hd 64180 */
  { 0x10, 3, inst,   H64, "in0 d,(%1)" },  /* hd 64180 */
  { 0x11, 3, inst,   H64, "out0 (%1),d" }, /* hd 64180 */
  { 0x14, 2, inst,   H64, "tst d" },       /* hd 64180 */
  { 0x18, 3, inst,   H64, "in0 e,(%1)" },  /* hd 64180 */
  { 0x19, 3, inst,   H64, "out0 (%1),e" }, /* hd 64180 */
  { 0x1C, 2, inst,   H64, "tst e" },       /* hd 64180 */
  { 0x20, 3, inst,   H64, "in0 h,(%1)" },  /* hd 64180 */
  { 0x21, 3, inst,   H64, "out0 (%1),h" }, /* hd 64180 */
  { 0x24, 2, inst,   H64, "tst h" },       /* hd 64180 */
  { 0x28, 3, inst,   H64, "in0 l,(%1)" },  /* hd 64180 */
  { 0x29, 3, inst,   H64, "out0 (%1),l" }, /* hd 64180 */
  { 0x2C, 2, inst,   H64, "tst l" },       /* hd 64180 */
  { 0x34, 2, inst,   H64, "tst (hl)" },    /* hd 64180 */
  { 0x38, 3, inst,   H64, "in0 a,(%1)" },  /* hd 64180 */
  { 0x39, 3, inst,   H64, "out0 (%1),a" }, /* hd 64180 */
  { 0x3C, 2, inst,   H64, "tst a" },       /* hd 64180 */
  { 0x40, 2, inst,   0,   "in b,(c)" },
  { 0x41, 2, inst,   0,   "out (c),b" },
  { 0x42, 2, inst,   0,   "sbc hl,bc" },
  { 0x43, 4, instw,  0,   "ld (%a),bc" },
  { 0x44, 2, inst,   0,   "neg" },
  { 0x45, 2, popj,   0,   "retn" },
  { 0x46, 2, inst,   0,   "im 0" },
  { 0x47, 2, inst,   0,   "ld i,a" },
  { 0x48, 2, inst,   0,   "in c,(c)" },
  { 0x49, 2, inst,   0,   "out (c),c" },
  { 0x4A, 2, inst,   0,   "add hl,bc" },
  { 0x4B, 4, instw,  0,   "ld bc,(%a)" },
  { 0x4C, 2, inst,   H64, "mlt bc" },      /* hd 64180 */
  { 0x4D, 2, popj,   0,   "reti" },
  { 0x4F, 2, inst,   0,   "ld r,a" },
  { 0x50, 2, inst,   0,   "in d,(c)" },
  { 0x51, 2, inst,   0,   "out (c),d" },
  { 0x52, 2, inst,   0,   "sbc hl,de" },
  { 0x53, 4, instw,  0,   "ld (%a),de" },
  { 0x56, 2, inst,   0,   "im 1" },
  { 0x57, 2, inst,   0,   "ld a,i" },
  { 0x58, 2, inst,   0,   "in e,(c)" },
  { 0x59, 2, inst,   0,   "out (c),e" },
  { 0x5A, 2, inst,   0,   "adc hl,de" },
  { 0x5B, 4, instw,  0,   "ld de,(%a)" },
  { 0x5C, 2, inst,   H64, "mlt de" },	    /* hd 64180 */
  { 0x5E, 2, inst,   0,   "im 2" },
  { 0x5F, 2, inst,   0,   "ld a,r" },
  { 0x60, 2, inst,   0,   "in h,(c)" },
  { 0x61, 2, inst,   0,   "out (c),h" },
  { 0x62, 2, inst,   0,   "sbc hl,hl" },
  { 0x64, 3, inst,   H64, "tst %1" },	    /* hd 64180 */
  { 0x67, 2, inst,   0,   "rrd" },
  { 0x68, 2, inst,   0,   "in l,(c)" },
  { 0x69, 2, inst,   0,   "out (c),l" },
  { 0x6A, 2, inst,   0,   "adc hl,hl" },
  { 0x6C, 2, inst,   H64, "mlt hl" },	    /* hd 64180 */
  { 0x6F, 2, inst,   0,   "rld" },
  { 0x72, 2, inst,   0,   "sbc hl,sp" },
  { 0x73, 4, instw,  0,   "ld (%a),sp" },
  { 0x74, 3, inst,   H64, "tstio %1" },     /* hd 64180 */
  { 0x76, 2, inst,   H64, "slp" },	    /* hd 64180 */
  { 0x78, 2, inst,   0,   "in a,(c)" },
  { 0x79, 2, inst,   0,   "out (c),a" },
  { 0x7A, 2, inst,   0,   "adc hl,sp" },
  { 0x7B, 4, instw,  0,   "ld sp,(%a)" },
  { 0x7C, 2, inst,   H64, "mlt sp" },	    /* hd 64180 */
  { 0x83, 2, inst,   H64, "otim" },	    /* hd 64180 */
  { 0x8B, 2, inst,   H64, "otdm" },	    /* hd 64180 */
  { 0x93, 2, inst,   H64, "otimr" },	    /* hd 64180 */
  { 0x9B, 2, inst,   H64, "otdmr" },	    /* hd 64180 */
  { 0xA0, 2, inst,   0,   "ldi" },
  { 0xA1, 2, inst,   0,   "cpi" },
  { 0xA2, 2, inst,   0,   "ini" },
  { 0xA3, 2, inst,   0,   "outi" },
  { 0xA8, 2, inst,   0,   "ldd" },
  { 0xA9, 2, inst,   0,   "cpd" },
  { 0xAA, 2, inst,   0,   "ind" },
  { 0xAB, 2, inst,   0,   "outd" },
  { 0xB0, 2, inst,   0,   "ldir" },
  { 0xB1, 2, inst,   0,   "cpir" },
  { 0xB2, 2, inst,   0,   "inir" },
  { 0xB3, 2, inst,   0,   "otir" },
  { 0xB8, 2, inst,   0,   "lddr" },
  { 0xB9, 2, inst,   0,   "cpdr" },
  { 0xBA, 2, inst,   0,   "indr" },
  { 0xBB, 2, inst,   0,   "otdr" },
  { 0,    0, arnold, 0,   0 },
}; /* end of ED dispatch table. */

/*
** Global variables, from module common:
*/

extern int radix;		/* Hex, decimal, ... */

/*
** Start of our local variables:
*/

static address* touch;		/* Where in memory this instruction refers */

static byte opcode;		/* First byte to look at. */

static byte ixreg;		/* Current index reg, HL, IX or IY. */

#define HL 0
#define IX 1
#define IY 2

static bool zilogflag;		/* True if Z80 etc, false if 808x. */
static bool h64flag;		/* True if 64180 */
static bool i85flag;		/* True if 8085 */

static bool forcez;		/* Force Z80 mnemonics. */

/**********************************************************************/

static void hex(word w)
{
  bufhex(w, 0);
  if (w > 9) {
    casechar('h');
  }
}

static void dobyte(byte b)
{
  pb_length = 1;
  pb_status = st_byte;
  startline(true);
  casestring("defb");
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

static void dochar(byte b)
{
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
    dobyte(b);
    if ((b < 0200)
	&& (getstatus(istart) == st_char)
	&& (!c_exist(istart))) {
      c_insert(istart, charname(b));
    }
  }
}

static void doword(word w)
{
  pb_length = 2;
  startline(true);
  casestring("defw");
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

static void doptr(word w)
{
  address* a;

  pb_length = 2;
  a = a_l2a(w);
  startline(true);
  casestring("defw");
  spacedelim();
  reference(a);
  if (l_exist(a)) {
    bufstring(l_find(a));
  } else {
    hex(w);
  }
  endref();
  checkblank();
}

static void genlabel(word w)
{
  char cbuf[20];
  sprintf(cbuf, "l_%04x", w);
  l_insert(a_l2a(w), cbuf);
}

static void iname(bool dis, byte offset)
{
  int i;

  if (ixreg == HL) casestring("hl");
  if (ixreg == IX) casestring("ix");
  if (ixreg == IY) casestring("iy");
  if (dis && (ixreg != HL)) {
    i = sextb(offset);
    if (i < 0) {
      bufchar('-'); hex(-i);
    }
    if (i > 0) {
      bufchar('+'); hex(i);
    }
  }
}

static void decode_CB(void)
{
  byte bit76, bit543, bit210;
  byte b1;
  byte offset;

  if (ixreg != HL) {
    offset = getbyte();
  }

  b1 = getbyte();

  bit76 = (b1 >> 6) & 3;
  bit543 = (b1 >> 3) & 7;
  bit210 = b1 & 7;

  if (((bit76 == 0) && (bit543 == 6))
   || ((ixreg != HL) && (bit210 != 6))) {
    dobyte(opcode);
    return;
  }

  startline(true);

  if (bit76 == 0) {
    if (bit543 == 0) casestring("rlc");
    if (bit543 == 1) casestring("rrc");
    if (bit543 == 2) casestring("rl");
    if (bit543 == 3) casestring("rr");
    if (bit543 == 4) casestring("sla");
    if (bit543 == 5) casestring("sra");
    /* we already tested this. */
    if (bit543 == 7) casestring("srl");
    spacedelim();
  } else {
    if (bit76 == 1) casestring("bit");
    if (bit76 == 2) casestring("res");
    if (bit76 == 3) casestring("set");
    spacedelim();
    bufoctal(bit543, 1);
    bufchar(',');
  }
  if (bit210 == 0) casechar('b');
  if (bit210 == 1) casechar('c');
  if (bit210 == 2) casechar('d');
  if (bit210 == 3) casechar('e');
  if (bit210 == 4) casechar('h');
  if (bit210 == 5) casechar('l');
  if (bit210 == 6) {
    bufchar('('); iname(true, offset); bufchar(')');
  }
  if (bit210 == 7) casechar('a');
}  

static word getdisp(void)
{
  byte b;
  b = getbyte();
  return(a_a2w(pc) + sextb(b));
}

static void refaddr(word w)
{
  touch = a_l2a(w);
  reference(touch);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    hex(w);
    if (updateflag) {
      genlabel(w);
    }
  }
  endref();
}

static void refdisp(word w)
{
  int i;

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
      bufchar('-'); hex(-i);
    }
    if (i > 0) {
      bufchar('+'); hex(i);
    }
  }
  endref();
}

static void copytext(char* p)
{
  byte b;
  char c;
  char cbuf[10];

  while ((c = *(p++)) != (char) 0) {
    if (c == '%') {
      c = *(p++);
      if (c == 'c') {
	b = getmemory(pc);
	if ( printable(b) &&
	    (updateflag) &&
	    (!c_exist(istart)) ) {
	  sprintf(cbuf, "'%c'", b);
	  c_insert(istart, cbuf);
	}
	c = '1';
      }
      switch (c) {
      case '1':		/* One byte of data */
	if (argstatus == st_char) {
	  b = getbyte();
	  if (printable(b)) {
	    bufchar('\'');
	    bufchar((char) b);
	    bufchar('\'');
	  } else {
	    hex(b);
	  }
	} else {
	  hex(getbyte());
	}
	break;
      case '2':		/* One word of data */
	if (argstatus == st_ptr) {
	  refaddr(getword());
	} else {
	  hex(getword());
	}
	break;
      case 'a':		/* Absolute address */
	refaddr(getword());
	break;
      case 'd':		/* Relative address */
	refdisp(getdisp());
	break;
      case 'x':		/* Index, without displacement */
	iname(false, 0); break;
      case 'y':		/* Index, with displacement */
	iname(true, getbyte()); break;
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

static void decode(dispblock* disp)
{
  if ((disp == nil) || (disp->itype == unused)) {
    dobyte(opcode);
    return;
  }

  if (disp->flags & H64) {
    if (!h64flag) {
      dobyte(opcode);
      return;
    }
  }

  if (disp->flags & I85) {
    if (!i85flag) {
      dobyte(opcode);
      return;
    }
  }

  pb_length = disp->length;

  if (overrun()) {
    dobyte(opcode);
    return;
  }

  switch (disp->itype) {
  case pfxCB:
    decode_CB();
    return;
  case pfxDD:
    ixreg = IX;
    decode(finddisp(getbyte(), DDdisp));
    return;
  case pfxED:
    decode(finddisp(getbyte(), EDdisp));
    return;
  case pfxFD:
    ixreg = IY;
    decode(finddisp(getbyte(), DDdisp));
    return;
  }

  startline(true);		/* Here we know length.  Start off line. */

  if (forcez) {			/* Forcing Z80 mnemonics? */
    copytext(zilogdisp[opcode].expand);
  } else {			/* No, do normally. */
    copytext(disp->expand);	/* Copy expansion, expanding labels etc. */
  }

  switch (disp->itype) {	/* Do postprocessing. */
  case instb:
    suggest(touch, st_byte, 1);
    break;
  case instw:
    suggest(touch, st_word, 2);
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
  case trap:
    pb_detour = a_l2a(opcode - 0xc7);
    break;
  }
}

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

void z80_spec(address* a, int func)
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
    hex(a_a2l(a));
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

void z80_peek(stcode prefer)
{
  ixreg = HL;			/* Default index reg. */

  stddescription();

  if ((prefer == st_none) && e_exist(istart)) {
    pb_length = e_length(istart);
    startline(true);
    bufstring(e_find(istart));
  } else {
    switch (pb_status) {
    case st_none:
    case st_inst:
      opcode = getbyte();
      if (zilogflag) {
	decode(&zilogdisp[opcode]);
      } else {
	decode(&inteldisp[opcode]);
      }
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

/**********************************************************************/

static void checksyntax(void)
{
  char* syntax;

  forcez = false;

  syntax = s_read(s_index("syntax"));
  if (syntax != nil) {
    if (strcmp(syntax, "foreign") == 0) {
      if (!zilogflag) {
	forcez = true;
      }
    }
  }
}

/**********************************************************************/

char* z80_lcan(char* name)
{
  static char work[10];

  return(canonicalize(name, work, 6));
}

bool z80_lchk(char* name)
{
  return(checkstring(name, "", "0123456789_?"));
}

void z80_lgen(address* addr)
{
  genlabel(a_a2w(addr));
}

void z80_sset(symindex index)	/* Symbol got set. */
{
  checksyntax();		/* Check syntax to be used. */
}

void z80_sdel(symindex index)	/* Symbol got deleted. */
{
  if (index == 0) {
    checksyntax();		/* Check syntax to be used. */
  }
}

/**********************************************************************/

static void setpf(void)
{
  /* Set up our functions: */
  
  spf_peek(z80_peek);
  spf_spec(z80_spec);
  spf_lcan(z80_lcan);
  spf_lchk(z80_lchk);
  spf_lgen(z80_lgen);
  spf_sset(z80_sset);
  spf_sdel(z80_sdel);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 16;		/* Number of address bits. */
  pv_bigendian = false;		/* We are little-endian. */

  /* Default the flags. */

  h64flag = false;
  i85flag = false;
}

/*
** i8080 -- Intel 8080 CPU.
*/

void i8080_init(void)
{
  setpf();
  zilogflag = false;
  checksyntax();
}

bool i8080_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Intel 8080 processor.\n\
");
    return(true);
  }
  return(false);
}

/*
** i8085 -- Intel 8085 CPU.
*/

void i8085_init(void)
{
  setpf();
  zilogflag = false;
  i85flag = true;
  checksyntax();
}

bool i8085_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Intel 8085 processor.\n\
");
    return(true);
  }
  return(false);
}

/*
** z80 -- Zilog Z80 CPU.
*/

void z80_init(void)
{
  setpf();
  zilogflag = true;
  checksyntax();
}

bool z80_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Z80 processor.\n\
");
    return(true);
  }
  return(false);
}

/*
** h64180 -- Hitachi HD 64180 CPU.
*/

void h64180_init(void)
{
  setpf();
  zilogflag = true;
  h64flag = true;
  checksyntax();
}

bool h64180_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Hitachi HD64180 processor.\n\
");
    return(true);
  }
  return(false);
}
