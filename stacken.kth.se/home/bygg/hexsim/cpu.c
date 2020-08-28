/*
 * This module implements the cpu and memory.
 */

#include "hexsim.h"

/*
 *  Data types:
 */

struct context {
  hexaword ctx_registers[16];	/* Registers. */
  hexaword ctx_pc;		/* Program counter. */
  uint32_t ctx_psw;		/* Processor status word. */
  uint8_t num;			/* Our context number. */
};

/*
 *  Variables:
 */

struct context contexts[8];	/* Contexts. */
#  define CTX_USER  0		/*   User context. */
#  define CTX_EXEC  1		/*   Exec context. */
#  define CTX_PFH   2		/*   Page fault handler. */
#  define CTX_INT0  3		/*   Interrupt level 0. (soft) */
#  define CTX_INT1  4		/*   Interrupt level 1. */
#  define CTX_INT2  5		/*   Interrupt level 2. */
#  define CTX_INT3  6		/*   Interrupt level 3. */
#  define CTX_NMI   7		/*   NMI/trap handler. */

struct context * CTX;		/* Pointer to current context. */

uint16_t rcmask;		/* Register change mask. */

hexaword ICT;			/* Instruction Count. */
hexaword OPC;			/* Old PC, at start of instruction. */

hexaword PC;			/* Program Counter. */

hexaword AR;			/* "A" (arithmetic) register. */
hexaword ARX;			/* "A" extension. */
hexaword BR;			/* "B" (buffer) register. */
hexaword BRX;			/* "B" extension. */

hexaword IR;			/* Instruction register. */

hexaword EA;			/* EA (computed) */
uint8_t  EAT;			/* EA type. */
#  define EAT_LOCAL  1		/*    EA computation is local. */
#  define EAT_GLOBAL 2		/*    EA computation is global. */

int      ERROR;			/* Non-zero if an error occured during - */
				/* the emulation of this instruction. */

/*
 *  The following are the sub-fields of a single instruction, split
 *  up from the first 32 bits of it.
 */

uint8_t  OP;			/* Opcode field. */
uint8_t  AC;			/* AC (register) field. */
uint8_t  L;			/* L bit. */
uint16_t NUM;			/* Number field. */
uint8_t  I;			/* I bit. */
uint8_t  XSF;			/* X scale factor. */
uint8_t  X;			/* Index register. */

/*
 *  CPU registers etc. that should be visible to others as well.
 */

hexaword EBR;			/* Exec Base Register. */
hexaword UBR;			/* User Base Register. */
hexaword SPT;			/* SPT base address. */
hexaword CST;			/* CST base address. */
hexaword CSTM;			/* CST mask word. */
hexaword PUR;			/* Process use register. */

uint8_t cpulevel;		/* One bit/level for active levels. */

/*
 *  UART:
 */

hexaword uart_inputbuf;		/* Room for eight chars. */
int uart_inputlen;		/* Number of chars in buf. */

int uart_totalchars;		/* Total count of chars received. */
int uart_silooverrun;		/* Total number of silo overruns. */

/*
 *  Known constants:
 */

const hexaword FWMASK  = 0xffffffffffffffffULL;
const hexaword LHMASK  = 0xffffffff00000000ULL;
const hexaword RHMASK  = 0x00000000ffffffffULL;
const hexaword RHSIGN  = 0x0000000080000000ULL;
const hexaword SIGNBIT = 0x8000000000000000ULL;
const hexaword ZERO    = 0x0000000000000000ULL;

/*
 * Op table:
 */

struct opentry {
  uint8_t  opcode;
  uint8_t  optype;
  char*    opname;
  void    (*handler)(struct opentry* i);
  uint32_t opflags;
};

/* types of ops. */

#define OP_U 0			/* This opcode is unassigned. */
#define OP_I 1			/* This is a normal instruction. */
#define OP_A 2			/* This is arith immed. */
#define OP_F 3			/* This is float immed. */

/* flags: */

#define IM      0x01		/* EA is immed. */

#define TL      0x02		/* Test left. */

#define AD      0x04		/* Immed. with address semantics. */

#define SZ      0x30		/* Size field, two bits. */
#  define S8      0x00		/*   Default size, 8 bytes. */
#  define S4      0x10		/*   Size is 4 bytes. */
#  define S2      0x20 		/*   Size is 2 bytes. */
#  define S1      0x30		/*   Size is 1 byte. */

#define RD      0x40		/* Read memory. */
#define WR      0x80		/* Write memory. */

#define RD2     RD		/* XXX FIX THIS XXX */
#define WR2     WR		/* XXX FIX THIS XXX */

#define LA      0x0100		/* Load AR from AC before dispatch. */
#define LA1     0x0200		/* Load ARX ... */
#define LA2     (LA + LA1)
#define SA      0x0400		/* Store AR to AC after dispatch. */
#define SA1     0x0800		/* Store ARX ... */
#define SA2     (SA + SA1)

/* handlers: */

#define HANDLER(arg) static void arg(struct opentry* op)

HANDLER (H_NOOP);		/* No operation. */
HANDLER (H_UNASS);		/* Unassigned instruction. */
HANDLER (H_LUUO);		/* LUUO. */
HANDLER (H_ARIMM);		/* Arithmetic immed. */
HANDLER (H_FLIMM);		/* Floating point immed. */
HANDLER (H_JSP);		/* Jump and Save PC. */
HANDLER (H_JSR);		/* Jump to SubRoutine. */
HANDLER (H_JSYS);		/* Jump to system. */
HANDLER (H_JRST);		/* Handle JRST instructions. */
HANDLER (H_EXCH);		/* Exch instr. */
HANDLER (H_MOVE);		/* move (from memory). */
HANDLER (H_MOVN);		/* move negative. */
HANDLER (H_MOVEM);		/* move (to memory). */
HANDLER (H_PUSH);		/* push data. */
HANDLER (H_POP);		/* pop data. */
HANDLER (H_PUSHJ);		/* push & jump. */
HANDLER (H_POPJ);		/* pop & jump. */
HANDLER (H_PUSHM);		/* push multip. regs. */
HANDLER (H_POPM);		/* pop multip. regs. */
HANDLER (H_COMPAR);		/* cai/cam class. */
HANDLER (H_JUMP);		/* jump class. */
HANDLER (H_AOJ);
HANDLER (H_SOJ);

HANDLER (H_SETZM);		/* Set memory all zero. */
HANDLER (H_SETOM);		/* Set memory all ones. */

HANDLER (H_HRR);		/* Half-word moves. */
HANDLER (H_HRL);
HANDLER (H_HLR);
HANDLER (H_HLL);

HANDLER (H_SKIP);		/* skip/aos/sos class. */
HANDLER (H_AOS);
HANDLER (H_SOS);

HANDLER (H_XOR);		/* Bitwise arithmetic. */
HANDLER (H_AND);
HANDLER (H_IOR);
HANDLER (H_EQV);

HANDLER (H_ADD);		/* Fixed-point add. */
HANDLER (H_ADDC);		/* Fixed-point add with carry. */
HANDLER (H_SUB);		/* Fixed-point sub. */
HANDLER (H_SUBC);		/* Fixed-point sub with carry. */

HANDLER (H_IMUL);		/* Fixed-point mul, 64*64 => 64,  signed. */
HANDLER (H_MUL);		/* Fixed-point mul  64*64 => 128, signed. */
HANDLER (H_UIMUL);		/* Fixed-point mul. 64*64 => 64,  unsigned. */
HANDLER (H_UMUL);		/* Fixed-point mul. 64*64 => 128, unsigned. */
HANDLER (H_IDIV);		/* Fixed-point div/mod, 64/64,  signed. */
HANDLER (H_DIV);		/* Fixed-point div/mod, 128/64, signed. */
HANDLER (H_UIDIV);		/* Fixed-point div/mod, 64/64,  unsigned. */
HANDLER (H_UDIV);		/* Fixed-point div/mod, 128/64, unsigned. */

HANDLER (H_MADD);		/* Fixed-point mul+add. */
HANDLER (H_MSUB);		/* Fixed-point mul+sub. */
HANDLER (H_UMADD);		/* Fixed-point mul+add, unsigned. */
HANDLER (H_UMSUB);		/* Fixed-point mul+sub, unsigned. */

HANDLER (H_FADD);		/* Floating-point add. */
HANDLER (H_FSUB);		/* Floating-point sub. */
HANDLER (H_FMUL);		/* Floating-point mul. */
HANDLER (H_FDIV);		/* Floating-point div. */

HANDLER (H_FMADD);		/* Floating-point mul+add. */
HANDLER (H_FMSUB);		/* Floating-point mul+sub. */

HANDLER (H_LSH);		/* Logical shift. */
HANDLER (H_LSHC);		/* Logical shift combined. */
HANDLER (H_ASH);		/* Arithmetic shift. */
HANDLER (H_ASHC);		/* Arithmetic shift combined. */
HANDLER (H_ROT);		/* Rotate. */
HANDLER (H_ROTC);		/* Rotate combined. */

HANDLER (H_TEST);		/* Txxx class. */

HANDLER (H_TTCALL);		/* Testing. */

HANDLER (H_NOTYET);		/* Until we write them. */

/* The table itself: */

static struct opentry optable[256] = {
  { 0x00, OP_U, "",        H_UNASS,  0 },
  { 0x01, OP_I, "luuo",    H_LUUO,   0 },
  { 0x02, OP_I, "luuo",    H_LUUO,   0 },
  { 0x03, OP_I, "luuo",    H_LUUO,   0 },
  { 0x04, OP_I, "luuo",    H_LUUO,   0 },
  { 0x05, OP_I, "luuo",    H_LUUO,   0 },
  { 0x06, OP_I, "luuo",    H_LUUO,   0 },
  { 0x07, OP_I, "luuo",    H_LUUO,   0 },
  { 0x08, OP_A, "arithi",  H_ARIMM,  IM },
  { 0x09, OP_U, "",        H_UNASS,  0 },
  { 0x0A, OP_F, "floati",  H_FLIMM,  IM },
  { 0x0B, OP_U, "",        H_UNASS,  0 },
  { 0x0C, OP_U, "",        H_UNASS,  0 },
  { 0x0D, OP_U, "",        H_UNASS,  0 },
  { 0x0E, OP_U, "",        H_UNASS,  0 },
  { 0x0F, OP_U, "",        H_UNASS,  0 },

  { 0x10, OP_I, "jsp",     H_JSP,    AD+SA },
  { 0x11, OP_I, "jsr",     H_JSR,    0 },
  { 0x12, OP_I, "jsys",    H_JSYS,   IM },
  { 0x13, OP_I, "jrst",    H_JRST,   0 },
  { 0x14, OP_U, "",        H_UNASS,  0 },
  { 0x15, OP_U, "",        H_UNASS,  0 },
  { 0x16, OP_U, "",        H_UNASS,  0 },
  { 0x17, OP_U, "",        H_UNASS,  0 },
  { 0x18, OP_I, "hrri",    H_HRR,    IM+LA+SA },
  { 0x19, OP_I, "hrr",     H_HRR,    RD+LA+SA },
  { 0x1A, OP_I, "hrli",    H_HRL,    IM+LA+SA },
  { 0x1B, OP_I, "hrl",     H_HRL,    RD+LA+SA },
  { 0x1C, OP_I, "hlri",    H_HLR,    IM+LA+SA },
  { 0x1D, OP_I, "hlr",     H_HLR,    RD+LA+SA },
  { 0x1E, OP_I, "hlli",    H_HLL,    IM+LA+SA },
  { 0x1F, OP_I, "hll",     H_HLL,    RD+LA+SA },

  { 0x20, OP_I, "movei",   H_MOVE,   IM+SA },
  { 0x21, OP_I, "movea",   H_MOVE,   IM+AD+SA },
  { 0x22, OP_I, "movni",   H_MOVN,   IM+SA },
  { 0x23, OP_I, "dmove",   H_MOVE,   RD2+SA2 },
  { 0x24, OP_I, "move",    H_MOVE,   RD+SA },
  { 0x25, OP_I, "move32",  H_MOVE,   RD+S4+SA },
  { 0x26, OP_I, "move16",  H_MOVE,   RD+S2+SA },
  { 0x27, OP_I, "move8",   H_MOVE,   RD+S1+SA },
  { 0x28, OP_I, "setzm",   H_SETZM,  WR },
  { 0x29, OP_I, "setom",   H_SETOM,  WR },
  { 0x2A, OP_I, "exch",    H_EXCH,   RD+WR+LA  /* SA done by handler */ },
  { 0x2B, OP_I, "dmovem",  H_MOVEM,  WR2+LA2 },
  { 0x2C, OP_I, "movem",   H_MOVEM,  WR+LA },
  { 0x2D, OP_I, "mov32m",  H_MOVEM,  WR+S4+LA },
  { 0x2E, OP_I, "mov16m",  H_MOVEM,  WR+S2+LA },
  { 0x2F, OP_I, "mov8m",   H_MOVEM,  WR+S1+LA },

  { 0x30, OP_I, "pushi",   H_PUSH,   IM+LA+SA },
  { 0x31, OP_I, "pusha",   H_PUSH,   IM+AD+LA+SA },
  { 0x32, OP_I, "pushm",   H_PUSHM,  IM+LA+SA },
  { 0x33, OP_I, "pushj",   H_PUSHJ,  IM+AD+LA+SA },
  { 0x34, OP_I, "push",    H_PUSH,   RD+LA+SA },
  { 0x35, OP_I, "push32",  H_PUSH,   RD+S4+LA+SA },
  { 0x36, OP_I, "push16",  H_PUSH,   RD+S2+LA+SA },
  { 0x37, OP_I, "push8",   H_PUSH,   RD+S1+LA+SA },
  { 0x38, OP_U, "",        H_UNASS,  0 },
  { 0x39, OP_U, "",        H_UNASS,  0 },
  { 0x3A, OP_I, "popm",    H_POPM,   IM+LA+SA },
  { 0x3B, OP_I, "popj",    H_POPJ,   IM+LA+SA },
  { 0x3C, OP_I, "pop",     H_POP,    WR+LA+SA },
  { 0x3D, OP_I, "pop32",   H_POP,    WR+S4+LA+SA },
  { 0x3E, OP_I, "pop16",   H_POP,    WR+S2+LA+SA },
  { 0x3F, OP_I, "pop8",    H_POP,    WR+S1+LA+SA },

  { 0x40, OP_I, "cai",     H_NOOP,   0 },
  { 0x41, OP_I, "cail",    H_COMPAR, IM+LA },
  { 0x42, OP_I, "caie",    H_COMPAR, IM+LA },
  { 0x43, OP_I, "caile",   H_COMPAR, IM+LA },
  { 0x44, OP_I, "caia",    H_COMPAR, IM+LA },
  { 0x45, OP_I, "caige",   H_COMPAR, IM+LA },
  { 0x46, OP_I, "cain",    H_COMPAR, IM+LA },
  { 0x47, OP_I, "caig",    H_COMPAR, IM+LA },
  { 0x48, OP_I, "cam",     H_NOOP,   RD }, /* Read, we might page-fault. */
  { 0x49, OP_I, "caml",    H_COMPAR, RD+LA },
  { 0x4A, OP_I, "came",    H_COMPAR, RD+LA },
  { 0x4B, OP_I, "camle",   H_COMPAR, RD+LA },
  { 0x4C, OP_I, "cama",    H_COMPAR, RD+LA },
  { 0x4D, OP_I, "camge",   H_COMPAR, RD+LA },
  { 0x4E, OP_I, "camn",    H_COMPAR, RD+LA },
  { 0x4F, OP_I, "camg",    H_COMPAR, RD+LA },

  { 0x50, OP_I, "jump",    H_NOOP,   0 },
  { 0x51, OP_I, "jumpl",   H_JUMP,   LA },
  { 0x52, OP_I, "jumpe",   H_JUMP,   LA },
  { 0x53, OP_I, "jumple",  H_JUMP,   LA },
  { 0x54, OP_I, "jumpa",   H_JUMP,   LA },
  { 0x55, OP_I, "jumpge",  H_JUMP,   LA },
  { 0x56, OP_I, "jumpn",   H_JUMP,   LA },
  { 0x57, OP_I, "jumpg",   H_JUMP,   LA },
  { 0x58, OP_I, "skip",    H_SKIP,   RD }, /* Writes non-zero AC. */
  { 0x59, OP_I, "skipl",   H_SKIP,   RD }, /* Writes non-zero AC. */
  { 0x5A, OP_I, "skipe",   H_SKIP,   RD }, /* Writes non-zero AC. */
  { 0x5B, OP_I, "skiple",  H_SKIP,   RD }, /* Writes non-zero AC. */
  { 0x5C, OP_I, "skipa",   H_SKIP,   RD }, /* Writes non-zero AC. */
  { 0x5D, OP_I, "skipge",  H_SKIP,   RD }, /* Writes non-zero AC. */
  { 0x5E, OP_I, "skipn",   H_SKIP,   RD }, /* Writes non-zero AC. */
  { 0x5F, OP_I, "skipg",   H_SKIP,   RD }, /* Writes non-zero AC. */

  { 0x60, OP_I, "aoj",     H_AOJ,    LA+SA },
  { 0x61, OP_I, "aojl",    H_AOJ,    LA+SA },
  { 0x62, OP_I, "aoje",    H_AOJ,    LA+SA },
  { 0x63, OP_I, "aojle",   H_AOJ,    LA+SA },
  { 0x64, OP_I, "aoja",    H_AOJ,    LA+SA },
  { 0x65, OP_I, "aojge",   H_AOJ,    LA+SA },
  { 0x66, OP_I, "aojn",    H_AOJ,    LA+SA },
  { 0x67, OP_I, "aojg",    H_AOJ,    LA+SA },
  { 0x68, OP_I, "soj",     H_SOJ,    LA+SA },
  { 0x69, OP_I, "sojl",    H_SOJ,    LA+SA },
  { 0x6A, OP_I, "soje",    H_SOJ,    LA+SA },
  { 0x6B, OP_I, "sojle",   H_SOJ,    LA+SA },
  { 0x6C, OP_I, "soja",    H_SOJ,    LA+SA },
  { 0x6D, OP_I, "sojge",   H_SOJ,    LA+SA },
  { 0x6E, OP_I, "sojn",    H_SOJ,    LA+SA },
  { 0x6F, OP_I, "sojg",    H_SOJ,    LA+SA },

  { 0x70, OP_I, "aos",     H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x71, OP_I, "aosl",    H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x72, OP_I, "aose",    H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x73, OP_I, "aosle",   H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x74, OP_I, "aosa",    H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x75, OP_I, "aosge",   H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x76, OP_I, "aosn",    H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x77, OP_I, "aosg",    H_AOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x78, OP_I, "sos",     H_SOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x79, OP_I, "sosl",    H_SOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x7A, OP_I, "sose",    H_SOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x7B, OP_I, "sosle",   H_SOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x7C, OP_I, "sosa",    H_SOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x7D, OP_I, "sosge",   H_SOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x7E, OP_I, "sosn",    H_SOS,    RD+WR }, /* Writes non-zero AC. */
  { 0x7F, OP_I, "sosg",    H_SOS,    RD+WR }, /* Writes non-zero AC. */

  { 0x80, OP_I, "addi",    H_ADD,    IM+LA+SA },
  { 0x81, OP_I, "add",     H_ADD,    RD+LA+SA },
  { 0x82, OP_I, "subi",    H_SUB,    IM+LA+SA },
  { 0x83, OP_I, "sub",     H_SUB,    RD+LA+SA },
  { 0x84, OP_I, "addci",   H_ADDC,   IM+LA+SA },
  { 0x85, OP_I, "addc",    H_ADDC,   RD+LA+SA },
  { 0x86, OP_I, "subci",   H_SUBC,   IM+LA+SA },
  { 0x87, OP_I, "subc",    H_SUBC,   RD+LA+SA },
  { 0x88, OP_I, "xori",    H_XOR,    IM+LA+SA },
  { 0x89, OP_I, "xor",     H_XOR,    RD+LA+SA },
  { 0x8A, OP_I, "andi",    H_AND,    IM+LA+SA },
  { 0x8B, OP_I, "and",     H_AND,    RD+LA+SA },
  { 0x8C, OP_I, "iori",    H_IOR,    IM+LA+SA },
  { 0x8D, OP_I, "ior",     H_IOR,    RD+LA+SA },
  { 0x8E, OP_I, "eqvi",    H_EQV,    IM+LA+SA },
  { 0x8F, OP_I, "eqv",     H_EQV,    RD+LA+SA },

  { 0x90, OP_I, "imuli",   H_IMUL,   IM+LA+SA      },
  { 0x91, OP_I, "imul",    H_IMUL,   RD+LA+SA      },
  { 0x92, OP_I, "muli",    H_MUL,    IM+LA+SA2     },
  { 0x93, OP_I, "mul",     H_MUL,    RD+LA+SA2     },
  { 0x94, OP_I, "uimuli",  H_UIMUL,  IM+LA+SA      },
  { 0x95, OP_I, "uimul",   H_UIMUL,  RD+LA+SA      },
  { 0x96, OP_I, "umuli",   H_UMUL,   IM+LA+SA2     },
  { 0x97, OP_I, "umul",    H_UMUL,   RD+LA+SA2     },
  { 0x98, OP_I, "idivi",   H_IDIV,   IM+LA+SA2     },
  { 0x99, OP_I, "idiv",    H_IDIV,   RD+LA+SA2     },
  { 0x9A, OP_I, "divi",    H_DIV,    IM+LA2+SA2    },
  { 0x9B, OP_I, "div",     H_DIV,    RD+LA2+SA2    },
  { 0x9C, OP_I, "uidivi",  H_UIDIV,  IM+LA+SA2     },
  { 0x9D, OP_I, "uidiv",   H_UIDIV,  RD+LA+SA2     },
  { 0x9E, OP_I, "udivi",   H_UDIV,   IM+LA2+SA2    },
  { 0x9F, OP_I, "udiv",    H_UDIV,   RD+LA2+SA2    },

  { 0xA0, OP_I, "maddi",   H_MADD,   IM,           },
  { 0xA1, OP_I, "madd",    H_MADD,   RD,           },
  { 0xA2, OP_I, "msubi",   H_MSUB,   IM,           },
  { 0xA3, OP_I, "msub",    H_MSUB,   RD,           },
  { 0xA4, OP_I, "umaddi",  H_UMADD,  IM,           },
  { 0xA5, OP_I, "umadd",   H_UMADD,  RD,           },
  { 0xA6, OP_I, "umsubi",  H_UMSUB,  IM,           },
  { 0xA7, OP_I, "umsub",   H_UMSUB,  RD,           },
  { 0xA8, OP_I, "lsh",     H_LSH,    IM+LA+SA    },
  { 0xA9, OP_I, "lshc",    H_LSHC,   IM+LA2+SA2  },
  { 0xAA, OP_I, "ash",     H_ASH,    IM+LA+SA    },
  { 0xAB, OP_I, "ashc",    H_ASHC,   IM+LA2+SA2  },
  { 0xAC, OP_I, "rot",     H_ROT,    IM+LA+SA    },
  { 0xAD, OP_I, "rotc",    H_ROTC,   IM+LA2+SA2  },
  { 0xAE, OP_U, "",        H_UNASS,              },
  { 0xAF, OP_U, "",        H_UNASS,              },

  { 0xB0, OP_I, "faddi",   H_FADD,   IM+LA+SA    },
  { 0xB1, OP_I, "fadd",    H_FADD,   RD+LA+SA    },
  { 0xB2, OP_I, "fsubi",   H_FSUB,   IM+LA+SA    },
  { 0xB3, OP_I, "fsub",    H_FSUB,   RD+LA+SA    },
  { 0xB4, OP_I, "fmuli",   H_FMUL,   IM+LA+SA    },
  { 0xB5, OP_I, "fmul",    H_FMUL,   RD+LA+SA    },
  { 0xB6, OP_I, "fdivi",   H_FDIV,   IM+LA+SA    },
  { 0xB7, OP_I, "fdiv",    H_FDIV,   RD+LA+SA    },
  { 0xB8, OP_I, "fmaddi",  H_FMADD,  IM+LA+SA2   },
  { 0xB9, OP_I, "fmadd",   H_FMADD,  RD+LA+SA2   },
  { 0xBA, OP_I, "fmsubi",  H_FMSUB,  IM+LA+SA2   },
  { 0xBB, OP_I, "fmsub",   H_FMSUB,  RD+LA+SA2   },
  { 0xBC, OP_U, "",        H_UNASS,  0 },
  { 0xBD, OP_U, "",        H_UNASS,  0 },
  { 0xBE, OP_U, "",        H_UNASS,  0 },
  { 0xBF, OP_U, "",        H_UNASS,  0 },

  { 0xC0, OP_I, "trn",     H_NOOP,   0 },
  { 0xC1, OP_I, "trne",    H_TEST,   IM+LA },
  { 0xC2, OP_I, "trna",    H_TEST,   IM+LA },
  { 0xC3, OP_I, "trnn",    H_TEST,   IM+LA },
  { 0xC4, OP_I, "trz",     H_TEST,   IM+LA+SA },
  { 0xC5, OP_I, "trze",    H_TEST,   IM+LA+SA },
  { 0xC6, OP_I, "trza",    H_TEST,   IM+LA+SA },
  { 0xC7, OP_I, "trzn",    H_TEST,   IM+LA+SA },
  { 0xC8, OP_I, "trc",     H_TEST,   IM+LA+SA },
  { 0xC9, OP_I, "trce",    H_TEST,   IM+LA+SA },
  { 0xCA, OP_I, "trca",    H_TEST,   IM+LA+SA },
  { 0xCB, OP_I, "trcn",    H_TEST,   IM+LA+SA },
  { 0xCC, OP_I, "tro",     H_TEST,   IM+LA+SA },
  { 0xCD, OP_I, "troe",    H_TEST,   IM+LA+SA },
  { 0xCE, OP_I, "troa",    H_TEST,   IM+LA+SA },
  { 0xCF, OP_I, "tron",    H_TEST,   IM+LA+SA },

  { 0xD0, OP_I, "tln",     H_NOOP,   0 },
  { 0xD1, OP_I, "tlne",    H_TEST,   IM+LA+TL },
  { 0xD2, OP_I, "tlna",    H_TEST,   IM+LA+TL },
  { 0xD3, OP_I, "tlnn",    H_TEST,   IM+LA+TL },
  { 0xD4, OP_I, "tlz",     H_TEST,   IM+LA+SA+TL },
  { 0xD5, OP_I, "tlze",    H_TEST,   IM+LA+SA+TL },
  { 0xD6, OP_I, "tlza",    H_TEST,   IM+LA+SA+TL },
  { 0xD7, OP_I, "tlzn",    H_TEST,   IM+LA+SA+TL },
  { 0xD8, OP_I, "tlc",     H_TEST,   IM+LA+SA+TL },
  { 0xD9, OP_I, "tlce",    H_TEST,   IM+LA+SA+TL },
  { 0xDA, OP_I, "tlca",    H_TEST,   IM+LA+SA+TL },
  { 0xDB, OP_I, "tlcn",    H_TEST,   IM+LA+SA+TL },
  { 0xDC, OP_I, "tlo",     H_TEST,   IM+LA+SA+TL },
  { 0xDD, OP_I, "tloe",    H_TEST,   IM+LA+SA+TL },
  { 0xDE, OP_I, "tloa",    H_TEST,   IM+LA+SA+TL },
  { 0xDF, OP_I, "tlon",    H_TEST,   IM+LA+SA+TL },

  { 0xE0, OP_U, "",        H_NOTYET, 0 },
  { 0xE1, OP_U, "",        H_NOTYET, 0 },
  { 0xE2, OP_U, "",        H_NOTYET, 0 },
  { 0xE3, OP_U, "",        H_NOTYET, 0 },
  { 0xE4, OP_U, "",        H_NOTYET, 0 },
  { 0xE5, OP_U, "",        H_NOTYET, 0 },
  { 0xE6, OP_U, "",        H_NOTYET, 0 },
  { 0xE7, OP_U, "",        H_NOTYET, 0 },
  { 0xE8, OP_I, "ttcall",  H_TTCALL, 0 }, /* Testing. */
  { 0xE9, OP_U, "",        H_NOTYET, 0 },
  { 0xEA, OP_U, "",        H_NOTYET, 0 },
  { 0xEB, OP_U, "",        H_NOTYET, 0 },
  { 0xEC, OP_U, "",        H_NOTYET, 0 },
  { 0xED, OP_U, "",        H_NOTYET, 0 },
  { 0xEE, OP_U, "",        H_NOTYET, 0 },
  { 0xEF, OP_U, "",        H_NOTYET, 0 },

  { 0xF0, OP_U, "",        H_NOTYET, 0 },
  { 0xF1, OP_U, "",        H_NOTYET, 0 },
  { 0xF2, OP_U, "",        H_NOTYET, 0 },
  { 0xF3, OP_U, "",        H_NOTYET, 0 },
  { 0xF4, OP_U, "",        H_NOTYET, 0 },
  { 0xF5, OP_U, "",        H_NOTYET, 0 },
  { 0xF6, OP_U, "",        H_NOTYET, 0 },
  { 0xF7, OP_U, "",        H_NOTYET, 0 },
  { 0xF8, OP_U, "",        H_NOTYET, 0 },
  { 0xF9, OP_U, "",        H_NOTYET, 0 },
  { 0xFA, OP_U, "",        H_NOTYET, 0 },
  { 0xFB, OP_U, "",        H_NOTYET, 0 },
  { 0xFC, OP_U, "",        H_NOTYET, 0 },
  { 0xFD, OP_U, "",        H_NOTYET, 0 },
  { 0xFE, OP_U, "",        H_NOTYET, 0 },
  { 0xFF, OP_U, "",        H_NOTYET, 0 },
};

/************************************************************************/

/*
 *  UART stuff.
 */

void uart_input(char c)
{
  hexaword next;

  /*
   *  Got one character from the "terminal". Stuff it into the
   *  silo, and count it.  If CPU interrupts are enabled, give
   *  one.
   */

  if (uart_inputlen >= 8) {
    uart_silooverrun += 1;	/* Count this. */
  } else {
    next = c;
    next <<= (uart_inputlen * 8);
    uart_inputbuf |= next;
    uart_inputlen += 1;
  }
  uart_totalchars += 1;  

  wc_cpu();
}

/*
 *  Routines to read and write registers.
 */

hexaword reg_read(uint8_t reg)
{
  reg &= 15;

  return CTX->ctx_registers[reg];
}

void reg_write(uint8_t reg, hexaword data)
{
  reg &= 15;

  rcmask |= 1 << reg;

  CTX->ctx_registers[reg] = data;
  wc_context(CTX->num);
}

/*
 *  reg_deposit() writes without changing the mask.  Used to set
 *  register values from the command handler.
 */

void reg_deposit(uint8_t reg, hexaword data)
{
  reg &= 15;

  CTX->ctx_registers[reg] = data;
  wc_context(CTX->num);
}

uint16_t reg_rcmask(void)
{
  return rcmask;
}

/*
 *  Take a (non-zero) error code, and do page-faulty things.
 */

int page_fault(int code)
{
  /*
   *  Debugging aid:
   */

  cpulevel |= (1 << 6);		/* Do not keep for long. */
  wc_cpu();			/* ... */

  PC = OPC;			/* Undo increments etc. */

  return code;			/* FIX THIS! */
}

/*
 *  Our internal versions of routines to read/write memory:
 */

void mem_load(int size, hexaword addr, hexaword * res)
{
  if (mem_read(size, ASP_VIRT, addr, res) != 0) {
    page_fault(1);
    ERROR = 1;
  }
}

void mem_store(int size, hexaword addr, hexaword val)
{
  if (mem_write(size, ASP_VIRT, addr, val) != 0) {
    page_fault(1);
    ERROR = 1;
  }
}

/*
 *  Test a word for a condition.  The condition is implied from the opcode.
 */

int cond_test(hexaword w)
{
  switch (OP & 7) {
  case 0:			/* <none> */
    break;
  case 1:			/* xxxL */
    if (w & SIGNBIT) return 1;
    break;
  case 2:			/* xxxE */
    if (w == ZERO) return 1;
    break;
  case 3:			/* xxxLE */
    if (w == ZERO) return 1;
    if (w & SIGNBIT) return 1;
    break;
  case 4:			/* xxxA */
    return 1;
  case 5:			/* xxxGE */
    if (!(w & SIGNBIT)) return 1;
    break;
  case 6:			/* xxxN */
    if (w != ZERO) return 1;
    break;
  case 7:			/* xxxG */
    if (w & SIGNBIT) return 0;
    if (w == ZERO) return 0;
    return 1;
  }
  return 0;
}

/*
 *  Change the bits in the SZ field to an actual byte count.
 */

int decode_sz(uint8_t flags)
{
  switch (flags & SZ) {
  case S4:
    return 4;
  case S2:
    return 2;
  case S1:
    return 1;
  }

  return 8;
}

/*
 *  Modify the PC in controlled ways.
 */

void pc_inc(int amount)
{
  /*
   *  We should increment only in the right 32 bits...
   */
  PC += amount;
  wc_pc();
  wc_memory();
}

hexaword pc_examine(void)
{
  return PC;
}

void pc_deposit(hexaword val)
{
  PC = val;
  wc_pc();
  wc_memory();
}

/*
 *  Perform the essentials of a goto.
 */

void pc_from_ea(void)
{
  if (EA & 3) {			/* Cannot jump to unaligned addr. */
    page_fault(1);		/* XXX find correct code. */
    return;
  }

  if (EAT == EAT_GLOBAL) {
    PC = EA;
  } else {
    PC &= LHMASK;
    PC += EA;
  }
  wc_pc();
}

/*
 *  Read/write/(set/clear)bits in the PSW:
 */

uint32_t psw_read(void)
{
  return CTX->ctx_psw;
}

void psw_write(uint32_t val)
{
  CTX->ctx_psw = val;
  wc_register();
}

void psw_setbit(uint32_t bit)
{
  CTX->ctx_psw |= bit;
  wc_register();
}

void psw_clrbit(uint32_t bit)
{
  CTX->ctx_psw &= ~bit;
  wc_register();
}

int psw_chkbit(uint32_t bit)
{
  if (CTX->ctx_psw & bit)
    return 1;

  return 0;
}

/*
 *  Print the internal CPU registers.  Use buf* routines for output,
 *  since we will be called both from the command line and the window
 *  driver.
 */

void cpu_show_hw_regs(void)
{
  static char* lv[8] = {
    "User", "Exec", "PF", "Int0", "Int1", "Int2", "Int3", "NMI",
  };
  int i, j = 0;
  
  bufnewline();
  bufstring("  EBR:    "); bufhw(EBR); bufnewline();
  bufstring("  UBR:    "); bufhw(UBR); bufnewline();
  bufstring("  SPT:    "); bufhw(SPT); bufnewline();
  bufstring("  CST:    "); bufhw(CST); bufnewline();
  bufstring("  PUR:    "); bufhw(PUR); bufnewline();
  bufstring("  CSTM:   "); bufhw(CSTM); bufnewline();
  bufnewline();
  bufstring("  cpu levels: "); bufhex(cpulevel, 2);
  if (cpulevel != 0) {
    bufstring(" (");
    for (i = 0; i < 8; i += 1) {
      if (cpulevel & (1 << i)) {
	if (j)
	  bufstring(", ");
	bufstring(lv[i]);
	j = 1;
      }
    }
    bufstring(")");
  }
  bufnewline();
  bufnewline();

  bufstring("  UART:  buffer ");
  if (uart_inputlen > 0) {
    for (i = 0; i < uart_inputlen; i += 1) {
      bufstring("0x");
      bufhex((uart_inputbuf >> (i * 8)) & 0xff, 2);
      bufchar(' ');
    }
  } else {
    bufstring("is empty");
  }
  bufnewline();
  bufstring("         total input chars = "); bufnumber(uart_totalchars);
  bufstring(", silo overruns = "); bufnumber(uart_silooverrun);
  bufnewline();
  bufnewline();
}

/*
 *  Disassamble a word as an instruction.  Returns length (in bytes)
 *  of instruction, zero if illegal.
 */

int cpu_disass(hexaword data, char buf[])
{
  int pos = 0;
  struct opentry* op;
  
  IR = data >> 32;

  OP  = (IR & 0xff000000) >> 24;
  AC  = (IR & 0x00f00000) >> 20;
  L   = (IR & 0x00080000) >> 19;
  NUM = (IR & 0x0007ff00) >> 8;
  I   = (IR & 0x00000080) >> 7;
  XSF = (IR & 0x00000070) >> 4;
  X   = (IR & 0x0000000f);

  if (L == 1) {
    EA = data & RHMASK;
  } else {
    EA = NUM;
    if (EA & 0x400)
      EA |= 0xfffff800;
    NUM = 0;
  }

  op = &optable[OP];

  if (op->optype == OP_U)
    return 0;

  pos += sprintf(&buf[pos], "%s ", op->opname);
  if (NUM != 0)
    pos += sprintf(&buf[pos], "{%u} ", NUM);
  if (AC != 0)
    pos += sprintf(&buf[pos], "%u, ", AC);
  if (I != 0)
    pos += sprintf(&buf[pos], "@");
  pos += sprintf(&buf[pos], "%" PRIx64, EA);
  if (X != 0) {
    pos += sprintf(&buf[pos], "(");
    pos += sprintf(&buf[pos], "%u", X);
    if (XSF != 0)
      pos += sprintf(&buf[pos], ",%u", 1 << XSF);
    pos += sprintf(&buf[pos], ")");
  }
  if (L == 1)
    return 8;

  return 4;
}

/*
 *  Execute a single instruction.  Return 1 if error, 0 if OK.
 */

int cpu_execute(void)
{
  struct opentry* opt;

  /*
   *  No registers changed yet in this instruction, and no memory.
   */

  rcmask = 0;
  memc_clear();

  ERROR = 0;			/* No error yet. */

  /*
   *  Count total number of instructions executed.
   */

  ICT += 1;
  OPC = PC;			/* Save in case of PF etc. */

  /*
   *  Cycle 1, instruction fetch:
   */

  mem_load(4, PC, &IR);
  if (ERROR)
    return 1;

  OP  = (IR & 0xff000000) >> 24;
  AC  = (IR & 0x00f00000) >> 20;
  L   = (IR & 0x00080000) >> 19;
  NUM = (IR & 0x0007ff00) >> 8;
  I   = (IR & 0x00000080) >> 7;
  XSF = (IR & 0x00000070) >> 4;
  X   = (IR & 0x0000000f);

  if (L == 1) {
    mem_load(4, PC+4, &EA);
    if (ERROR)
      return 1;
    pc_inc(8);
  } else {
    EA = NUM;
    if (EA & 0x400)
      EA |= 0xfffff800;
    NUM = 0;
    pc_inc(4);
  }

  /*  If NII (Next Instruction Inhibit) is set, clear it and return,
   *  since this is that next instruction.
   */

  if (psw_chkbit(PSW_NII)) {
    psw_clrbit(PSW_NII);
    return 0;
  }

  /*
   *  Cycle 2, compute Effective Argument:
   */

  if (EA & RHSIGN)		/* Sign extend EA for now. */
    EA |= LHMASK;

  EAT = EAT_LOCAL;
  if (X != 0) {
    AR = reg_read(X);
    if (AR & LHMASK)
      EAT = EAT_GLOBAL;
    AR <<= XSF;
    EA += AR;
  }

  if (EAT == EAT_LOCAL)
    EA &= RHMASK;

  if (I) {	/* Indirect? */
    if (EAT == EAT_LOCAL) {
      EA |= (PC & LHMASK);
    }
    mem_load(8, EA, &EA);
    if (ERROR)
      return 1;
    EAT = EAT_GLOBAL;
  }
  
  opt = &optable[OP];

  /*
   *  Cycle 3, fetch arguments:
   */

  if (opt->opflags & RD) {
    if (EAT == EAT_LOCAL) {
      EA |= (PC & LHMASK);
    }
    mem_load(decode_sz(opt->opflags), EA, &BR);
    if (ERROR)
      return 1;
  }

  if (opt->opflags & IM) {
    BR = EA;
  }

  if (opt->opflags & AD && EAT == EAT_LOCAL) {
    /* Default upper half of EA from PC. */
  }

  if (opt->opflags & LA) {
    AR = reg_read(AC);
  }
  if (opt->opflags & LA1) {
    ARX = reg_read(AC + 1);
  }

  /*
   *  Cycle 4, execute the instruction code:
   */

  (*opt->handler)(opt);
  if (ERROR)
    return 1;

  /*
   *  Cycle 5, store results:
   */

  if (opt->opflags & SA) {
    reg_write(AC, AR);
  }
  if (opt->opflags & SA1) {
    reg_write(AC + 1, ARX);
  }

  return 0;
}

/*
 *  Init this module, set PC to zero, yadda yadda.
 */

void cpu_init(void)
{
  CTX = &contexts[CTX_EXEC];
  PC = 0;
  cpulevel = 1 << CTX_EXEC;
}

/************************************************************************/

/*
 *  Handlers for specific instruction classes.  The dispatch table
 *  points to these per opcode.  Note that some common things are
 *  done before the dispatch, according to flags in the dispatch
 *  entry, this might do most of the job of the instruction!
 */

/*
 *  Here for instructions that are essentially no-ops.
 */

HANDLER (H_NOOP)
{
  /* Well, we do nothing here. */
}

HANDLER (H_UNASS)
{
  ERROR = 1;
}

HANDLER (H_LUUO)
{
  /* handle LUUOs here. */

  ERROR = 1;
}

/*
 *  Various arithmetic functions on register.
 */

HANDLER (H_ARIMM)
{
  ERROR = 1;
}

/*
 *  Various floating point functions.
 */

HANDLER (H_FLIMM)
{
  ERROR = 1;
}

/*
 *  JSP instruction.
 */

HANDLER (H_JSP)
{
  AR = PC;

  pc_from_ea();
}

HANDLER (H_JSR)
{
  ERROR = 1;
}

HANDLER (H_JSYS)
{
  ERROR = 1;
}

HANDLER (H_JRST)
{
  switch (AC) {
  case 0:			/* JRST */
    pc_from_ea();
    return;
  case 1:			/* PORTAL */
    /* if conditions are OK, clear public. */
    ERROR = 1;
    return;
  default:
    ERROR = 1;
    break;
  }
}

/*
 *  setzm/setom here.
 */

HANDLER (H_SETZM)
{
  AR = ZERO;
  mem_store(8, EA, AR);
}

HANDLER (H_SETOM)
{
  AR = FWMASK;
  mem_store(8, EA, AR);
}

/*
 *  Half-word moves here.
 */

HANDLER (H_HRR)
{
  AR &= LHMASK;
  BR &= RHMASK;
  AR |= BR;
}

HANDLER (H_HRL)
{
  AR &= RHMASK;
  BR &= RHMASK;
  AR |= BR << 32;
}

HANDLER (H_HLR)
{
  AR &= LHMASK;
  BR &= LHMASK;
  AR |= BR >> 32;
}

HANDLER (H_HLL)
{
  AR &= RHMASK;
  BR &= LHMASK;
  AR |= BR;
}

/*
 *  Move (to register) instructions here.
 */

HANDLER (H_MOVE)
{
  AR = BR;
}

/*
 *  Move negative (to register) here.
 */

HANDLER (H_MOVN)
{
  AR = -BR;
}

/*
 *  move (to memory) instructions:
 */

HANDLER (H_MOVEM)
{
  mem_store(decode_sz(op->opflags), EA, AR);
}

/*
 *  exch instruction:
 */

HANDLER (H_EXCH)
{
  mem_store(8, EA, AR);
  if (ERROR)
    return;

  reg_write(AC, BR);
}

/*
 *  push data.
 */

HANDLER (H_PUSH)
{
  if (AR & 7) {			/* Unaligned stack pointer. */
    page_fault(1);		/* XXX fix this XXX */
    return;
  }

  AR += 8;
  mem_store(8, AR, BR);
}

/*
 *  pushj.
 */

HANDLER (H_PUSHJ)
{
  if (EA & 3) {			/* Unaligned new PC. */
    page_fault(1);
    return;
  }

  if (AR & 7) {			/* Unaligned stack pointer. */
    page_fault(1);
    return;
  }

  AR += 8;
  mem_store(8, AR, PC);
  if (ERROR)
    return;

  PC = EA;
}

/*
 *  pop data.
 */

HANDLER (H_POP)
{
  if (AR & 7) {			/* Unaligned stack pointer. */
    page_fault(1);
    return;
  }

  mem_load(8, AR, &BR);
  if (ERROR)
    return;

  mem_store(decode_sz(op->opflags), EA, BR);
  if (ERROR)
    return;

  AR -= 8;
}

/*
 *  pop return addr.
 */

HANDLER (H_POPJ)
{
  if (EA != 0) {
    H_POPM(op);
    if (ERROR)
      return;
  }

  if (AR & 7) {			/* Unaligned stack pointer. */
    page_fault(1);
    return;
  }

  mem_load(8, AR, &BR);
  if (ERROR)
    return;

  /*
   *  We should either force the two rightmost bits of the new PC to
   *  zero, or page_fault if they are not.
   */

  PC = BR;
  AR -= 8;
}

/*
 *  push registers according to mask.
 */

HANDLER (H_PUSHM)
{
  int reg;
  
  if (AR & 7) {			/* Unaligned stack pointer. */
    page_fault(1);
    return;
  }

  for (reg = 0; reg < 16; reg += 1) {
    if (EA & (1 << reg)) {
      AR += 8;
      BR = reg_read(reg);
      mem_store(8, AR, BR);
      if (ERROR)
	return;
    }
  }
}

/*
 *  pop registers according to mask.
 */

HANDLER (H_POPM)
{
  int reg;

  if (AR & 7) {			/* Unaligned stack pointer. */
    page_fault(1);
    return;
  }

  for (reg = 15; reg >= 0; reg -= 1) {
    if (EA & (1 << reg)) {
      mem_load(8, AR, &BR);
      if (ERROR)
	return;
      reg_write(reg, BR);
      AR -= 8;
    }
  }
}

/*
 * cai/cam class instructions handled here.
 */

HANDLER (H_COMPAR)
{
  AR = AR - BR;

  if (cond_test(AR))
    psw_setbit(PSW_NII);
}

/*
 * JUMPx here.
 */

HANDLER (H_JUMP)
{
  if (cond_test(AR)) {
    pc_from_ea();
  }
}

/*
 *  AOJx here.
 */

HANDLER (H_AOJ)
{
  if (NUM == 0) NUM = 1;	/* Default amount. */

  AR += NUM;

  H_JUMP(op);
}

HANDLER (H_SOJ)
{
  if (NUM == 0) NUM = 1;	/* Default amount. */

  AR -= NUM;

  H_JUMP(op);
}

/*
 *  SKIPx here.
 */

HANDLER (H_SKIP)
{
  if (cond_test(BR))
    psw_setbit(PSW_NII);

  if (AC != 0)
    reg_write(AC, BR);
}

/*
 *  AOSx here.
 */

HANDLER (H_AOS) {
  if (NUM == 0)	NUM = 1;	/* Default amount. */

  BR = BR + NUM;

  mem_store(8, EA, BR);
  if (ERROR)
    return;

  if (cond_test(BR))
    psw_setbit(PSW_NII);

  if (AC != 0)
    reg_write(AC, BR);
}

/*
 *  SOSx here.
 */

HANDLER (H_SOS) {
  if (NUM == 0)	NUM = 1;	/* Default amount. */

  BR = BR - NUM;

  mem_store(8, EA, BR);
  if (ERROR)
    return;

  if (cond_test(BR))
    psw_setbit(PSW_NII);

  if (AC != 0)
    reg_write(AC, BR);
}

HANDLER (H_XOR) { AR = AR ^ BR; }

HANDLER (H_AND) { AR = AR & BR; }

HANDLER (H_IOR) { AR = AR | BR; }

HANDLER (H_EQV) { AR = AR ^ ~BR; }

HANDLER (H_ADD) { AR = AR + BR; }

HANDLER (H_ADDC) { AR = AR + BR; }

HANDLER (H_SUB) { AR = AR - BR; }

HANDLER (H_SUBC) { AR = AR - BR; }

/* Fixed-point mul, 64*64 => 64,  signed. */

HANDLER (H_IMUL)
{
  math_multiply(MUL_SIGNED, MUL_RES64);
}

/* Fixed-point mul  64*64 => 128, signed. */

HANDLER (H_MUL)
{
  math_multiply(MUL_SIGNED, MUL_RES128);
}

/* Fixed-point mul. 64*64 => 64,  unsigned. */

HANDLER (H_UIMUL)
{
  math_multiply(MUL_UNSIGNED, MUL_RES64);
}

/* Fixed-point mul. 64*64 => 128, unsigned. */

HANDLER (H_UMUL) {
  math_multiply(MUL_UNSIGNED, MUL_RES128);
}

/* Fixed-point div/mod, 64/64,  signed. */

HANDLER (H_IDIV)
{
  ARX = AR % BR;
  AR  = AR / BR;
}

/* Fixed-point div/mod, 128/64, signed. */

HANDLER (H_DIV) { ERROR = 1; }

/* Fixed-point div/mod, 64/64,  unsigned. */

HANDLER (H_UIDIV)
{
  ARX = AR % BR;
  AR  = AR / BR;
}

/* Fixed-point div/mod, 128/64, unsigned. */

HANDLER (H_UDIV) { ERROR = 1; }

HANDLER (H_MADD) { ERROR = 1; }

HANDLER (H_MSUB) { ERROR = 1; }

HANDLER (H_UMADD) { ERROR = 1; }

HANDLER (H_UMSUB) { ERROR = 1; }

HANDLER (H_FADD) { ERROR = 1; }

HANDLER (H_FSUB) { ERROR = 1; }

HANDLER (H_FMUL) { ERROR = 1; }

HANDLER (H_FDIV) { ERROR = 1; }

HANDLER (H_FMADD) { ERROR = 1; }

HANDLER (H_FMSUB) { ERROR = 1; }

HANDLER (H_LSH) { ERROR = 1; }

HANDLER (H_LSHC) { ERROR = 1; }

HANDLER (H_ASH) { ERROR = 1; }

HANDLER (H_ASHC) { ERROR = 1; }

HANDLER (H_ROT) { ERROR = 1; }

HANDLER (H_ROTC) { ERROR = 1; }

/*
 * trxx/tlxx instructions handled here.
 */

HANDLER (H_TEST)
{
  if (op->opflags & TL)
    BR <<= 32;

  switch (op->opcode & 3) {
  case 0:  /* Txx */
    break;
  case 1:  /* TxxE */
    if ((AR & BR) == 0)
      psw_setbit(PSW_NII);
    break;
  case 2:  /* TxxA */
    psw_setbit(PSW_NII);
    break;
  case 3:  /* TxxN */
    if ((AR & BR) != 0)
      psw_setbit(PSW_NII);
    break;
  }

  switch (op->opcode & 12) {
  case 0:  /* TxNx */
    return;
  case 4:  /* TxZx */
    AR &= ~BR;
    break;
  case 8:  /* TxCx */
    AR ^= BR;
    break;
  case 12: /* TxOx */
    AR |= BR;
    break;
  }
}

HANDLER (H_NOTYET)
{
  /* dummy handler for instructions not yet handled. */

  ERROR = 1;
}

HANDLER (H_TTCALL)
{
  /*
   *  AC == function, 1 = output char, 2 = input char (skip if any),
   *  3 = just test input (skip if any), 4 = skip if output is OK.
   */

  switch (AC) {
  case 1:			/* Output one char, immed. */
    term_char(EA & 0xff);
    break;
  case 2:			/* Get input char. */
    BR = uart_inputbuf & 0xff;
    mem_store(8, EA, BR);
    if (ERROR)
      return;
    uart_inputbuf >>= 8;
    uart_inputlen -= 1;
    wc_cpu();
    break;
  case 3:			/* Skip if there is input. */
    if (uart_inputlen > 0)
      psw_setbit(PSW_NII);
    break;
  case 4:			/* Skip if output OK. */
    psw_setbit(PSW_NII);	/* Right now, always OK. */
    break;
  default:
    ERROR = 1;
  }
}
