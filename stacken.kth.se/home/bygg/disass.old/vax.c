/*
** This module implements driver code for the DEC VAX processor.
*/

#include "disass.h"

evf_init vax_init;
evf_help vax_help;

struct entryvector vax_vector = {
  "vax",			/* Name */
  "DEC VAX",			/* One-liner. */
  vax_init,			/* Init routine. */
  vax_help,			/* Help routine. */
};

/**********************************************************************/

/* itype values: */

#define unused  0
#define inst    1
#define jrst    2
#define pushj   3
#define popj    4

#define pfxFD   5

/* flags: */

#define CL	0x01		/* Compute length from string. */
#define EM      0x02		/* Routine starts with entry mask (word). */

/* ---- Main opcode table ---- */

static dispblock maindisp[256] = {
  { 0x00, 1, inst,   0,   "halt" },
  { 0x01, 1, inst,   0,   "nop" },
  { 0x02, 1, popj,   0,   "rei" },
  { 0x03, 1, inst,   0,   "bpt" },
  { 0x04, 1, popj,   0,   "ret" },
  { 0x05, 1, popj,   0,   "rsb" },
  { 0x06, 1, inst,   0,   "ldpctx" },
  { 0x07, 1, inst,   0,   "svpctx" },
  { 0x08, 1, inst,   CL,  "cvtps %w,%a,%w,%a" },
  { 0x09, 1, inst,   CL,  "cvtsp %w,%a,%w,%a" },
  { 0x0a, 1, inst,   CL,  "index %l,%l,%l,%l,%l,%w" },
  { 0x0b, 1, inst,   CL,  "crc %a,%l,%w,%a" },
  { 0x0c, 1, inst,   CL,  "prober %b,%w,%a" },
  { 0x0d, 1, inst,   CL,  "probew %b,%w,%a" },
  { 0x0e, 1, inst,   CL,  "insque %a,%a" },
  { 0x0f, 1, inst,   CL,  "remque %a,%l" },

  { 0x10, 2, pushj,  0,   "bsbb %1" },
  { 0x11, 2, jrst,   0,   "brb %1" },
  { 0x12, 2, pushj,  0,   "bneq %1" },
  { 0x13, 2, pushj,  0,   "beql %1" },
  { 0x14, 2, pushj,  0,   "bgtr %1" },
  { 0x15, 2, pushj,  0,   "bleq %1" },
  { 0x16, 1, pushj,  CL,  "jsb %a" },
  { 0x17, 1, jrst,   CL,  "jmp %a" },
  { 0x18, 2, pushj,  0,   "bgeq %1" },
  { 0x19, 2, pushj,  0,   "blss %1" },
  { 0x1a, 2, pushj,  0,   "bgtru %1" },
  { 0x1b, 2, pushj,  0,   "blequ %1" },
  { 0x1c, 2, pushj,  0,   "bvc %1" },
  { 0x1d, 2, pushj,  0,   "bvs %1" },
  { 0x1e, 2, pushj,  0,   "bcc %1" },
  { 0x1f, 2, pushj,  0,   "bcs %1" },

  { 0x20, 1, inst,   CL,  "addp4 %w,%a,%w,%a" },
  { 0x21, 1, inst,   CL,  "addp6 %w,%a,%w,%a,%w,%a" },
  { 0x22, 1, inst,   CL,  "subp4 %w,%a,%w,%a" },
  { 0x23, 1, inst,   CL,  "subp6 %w,%a,%w,%a,%w,%a" },
  { 0x24, 1, inst,   CL,  "cvtpt %w,%a,%a,%w,%a" },
  { 0x25, 1, inst,   CL,  "mulp %w,%a,%w,%a,%w,%a" },
  { 0x26, 1, inst,   CL,  "cvttp %w,%a,%a,%w,%a" },
  { 0x27, 1, inst,   CL,  "divp %w,%a,%w,%a,%w,%a" },
  { 0x28, 1, inst,   CL,  "movc3 %w,%a,%a" },
  { 0x29, 1, inst,   CL,  "cmpc3 %w,%a,%a" },
  { 0x2a, 1, inst,   CL,  "scanc %w,%a,%a,%b" },
  { 0x2b, 1, inst,   CL,  "spanc %w,%a,%a,%b" },
  { 0x2c, 1, inst,   CL,  "movc5 %w,%a,%b,%w,%a" },
  { 0x2d, 1, inst,   CL,  "cmpc5 %w,%a,%b,%w,%a" },
  { 0x2e, 1, inst,   CL,  "movtc %w,%a,%b,%a,%w,%a" },
  { 0x2f, 1, inst,   CL,  "movtuc %w,%a,%b,%a,%w,%a" },

  { 0x30, 3, pushj,  0,   "bsbw %2" },
  { 0x31, 3, jrst,   0,   "brw %2" },
  { 0x32, 1, inst,   CL,  "cvtwl %w,%l" },
  { 0x33, 1, inst,   CL,  "cvtwb %w,%b" },
  { 0x34, 1, inst,   CL,  "movp %w,%a,%a" },
  { 0x35, 1, inst,   CL,  "cmpp3 %w,%a,%a" },
  { 0x36, 1, inst,   CL,  "cvtpl %w,%a,%l" },
  { 0x37, 1, inst,   CL,  "cmpp4 %w,%a,%w,%a" },
  { 0x38, 1, inst,   CL,  "editpc %w,%a,%a,%a" },
  { 0x39, 1, inst,   CL,  "matchc %w,%a,%w,%a" },
  { 0x3a, 1, inst,   CL,  "locc %b,%w,%a" },
  { 0x3b, 1, inst,   CL,  "skpc %b,%w,%a" },
  { 0x3c, 1, inst,   CL,  "movzwl %w,%l" },
  { 0x3d, 1, inst,   CL,  "acbw %w,%w,%w,%2" },
  { 0x3e, 1, inst,   CL,  "movaw %w,%l" },
  { 0x3f, 1, inst,   CL,  "pushaw %a" },

  { 0x40, 1, inst,   CL,  "addf2 %f,%f" },
  { 0x41, 1, inst,   CL,  "addf3 %f,%f,%f" },
  { 0x42, 1, inst,   CL,  "subf2 %f,%f" },
  { 0x43, 1, inst,   CL,  "subf3 %f,%f,%f" },
  { 0x44, 1, inst,   CL,  "mulf2 %f,%f" },
  { 0x45, 1, inst,   CL,  "mulf3 %f,%f,%f" },
  { 0x46, 1, inst,   CL,  "divf2 %f,%f" },
  { 0x47, 1, inst,   CL,  "divf3 %f,%f,%f" },
  { 0x48, 1, inst,   CL,  "cvtfb %f,%b" },
  { 0x49, 1, inst,   CL,  "cvtfw %f,%w" },
  { 0x4a, 1, inst,   CL,  "cvtfl %f,%l" },
  { 0x4b, 1, inst,   CL,  "cvtrfl %f,%l" },
  { 0x4c, 1, inst,   CL,  "cvtbf %b,%f" },
  { 0x4d, 1, inst,   CL,  "cvtwf %w,%f" },
  { 0x4e, 1, inst,   CL,  "cvtlf %l,%f" },
  { 0x4f, 1, inst,   CL,  "acbf %f,%f,%f,%2" },

  { 0x50, 1, inst,   CL,  "movf %f,%f" },
  { 0x51, 1, inst,   CL,  "cmpf %f,%f" },
  { 0x52, 1, inst,   CL,  "mnegf %f,%f" },
  { 0x53, 1, inst,   CL,  "tstf %f" },
  { 0x54, 1, inst,   CL,  "emodf %f,%b,%f,%l,%f" },
  { 0x55, 1, inst,   CL,  "polyf %f,%w,%?" },
  { 0x56, 1, inst,   CL,  "cvtfd %f,%d" },
  { 0x57, 0, unused, 0,   0 },
  { 0x58, 1, inst,   CL,  "adawi %w,%w" },
  { 0x59, 0, unused, 0,   0 },
  { 0x5a, 0, unused, 0,   0 },
  { 0x5b, 0, unused, 0,   0 },
  { 0x5c, 1, inst,   CL,  "insqhi %a,%a" },
  { 0x5d, 1, inst,   CL,  "insqti %a,%a" },
  { 0x5e, 1, inst,   CL,  "remqhi %a,%l" },
  { 0x5f, 1, inst,   CL,  "remqti %a,%l" },

  { 0x60, 1, inst,   CL,  "addd2 %d,%d" },
  { 0x61, 1, inst,   CL,  "addd3 %d,%d,%d" },
  { 0x62, 1, inst,   CL,  "subd2 %d,%d" },
  { 0x63, 1, inst,   CL,  "subd3 %d,%d,%d" },
  { 0x64, 1, inst,   CL,  "muld2 %d,%d" },
  { 0x65, 1, inst,   CL,  "muld3 %d,%d,%d" },
  { 0x66, 1, inst,   CL,  "divd2 %d,%d" },
  { 0x67, 1, inst,   CL,  "divd3 %d,%d,%d" },
  { 0x68, 1, inst,   CL,  "cvtdb %d,%b" },
  { 0x69, 1, inst,   CL,  "cvtdw %d,%w" },
  { 0x6a, 1, inst,   CL,  "cvtdl %d,%l" },
  { 0x6b, 1, inst,   CL,  "cvtrdl %d,%l" },
  { 0x6c, 1, inst,   CL,  "cvtbd %b,%d" },
  { 0x6d, 1, inst,   CL,  "cvtwd %w,%d" },
  { 0x6e, 1, inst,   CL,  "cvtld %l,%d" },
  { 0x6f, 1, inst,   CL,  "acbd %d,%d,%d,%2" },

  { 0x70, 1, inst,   CL,  "movd %d,%d" },
  { 0x71, 1, inst,   CL,  "cmpd %d,%d" },
  { 0x72, 1, inst,   CL,  "mnegd %d,%d" },
  { 0x73, 1, inst,   CL,  "tstd %d" },
  { 0x74, 1, inst,   CL,  "emodd %d,%b,%d,%l,%d" },
  { 0x75, 1, inst,   CL,  "polyd %d,%w,%?" },
  { 0x76, 1, inst,   CL,  "cvtdf %d,%f" },
  { 0x77, 0, unused, 0,   0 },
  { 0x78, 1, inst,   CL,  "ashl %b,%l,%l" },
  { 0x79, 1, inst,   CL,  "ashq %b,%q,%q" },
  { 0x7a, 1, inst,   CL,  "emul %l,%l,%l,%q" },
  { 0x7b, 1, inst,   CL,  "ediv %l,%q,%w,%w" },
  { 0x7c, 1, inst,   CL,  "clrq %q" },
  { 0x7d, 1, inst,   CL,  "movq %q,%q" },
  { 0x7e, 1, inst,   CL,  "movaq %q,%l" },
  { 0x7f, 1, inst,   CL,  "pushaq %a" },

  { 0x80, 1, inst,   CL,  "addb2 %b,%b" },
  { 0x81, 1, inst,   CL,  "addb3 %b,%b,%b" },
  { 0x82, 1, inst,   CL,  "subb2 %b,%b" },
  { 0x83, 1, inst,   CL,  "subb3 %b,%b,%b" },
  { 0x84, 1, inst,   CL,  "mulb2 %b,%b" },
  { 0x85, 1, inst,   CL,  "mulb3 %b,%b,%b" },
  { 0x86, 1, inst,   CL,  "divb2 %b,%b" },
  { 0x87, 1, inst,   CL,  "divb3 %b,%b,%b" },
  { 0x88, 1, inst,   CL,  "bisb2 %b,%b" },
  { 0x89, 1, inst,   CL,  "bisb3 %b,%b,%b" },
  { 0x8a, 1, inst,   CL,  "bicb2 %b,%b" },
  { 0x8b, 1, inst,   CL,  "bicb3 %b,%b,%b" },
  { 0x8c, 1, inst,   CL,  "xorb2 %b,%b" },
  { 0x8d, 1, inst,   CL,  "xorb3 %b,%b,%b" },
  { 0x8e, 1, inst,   CL,  "mnegb %b,%b" },
  { 0x8f, 1, inst,   CL,  "caseb %b,%b,%b" },

  { 0x90, 1, inst,   CL,  "movb %b,%b" },
  { 0x91, 1, inst,   CL,  "cmpb %b,%b" },
  { 0x92, 1, inst,   CL,  "mcomb %b,%b" },
  { 0x93, 1, inst,   CL,  "bitb %b,%b" },
  { 0x94, 1, inst,   CL,  "clrb %b" },
  { 0x95, 1, inst,   CL,  "tstb %b" },
  { 0x96, 1, inst,   CL,  "incb %b" },
  { 0x97, 1, inst,   CL,  "decb %b" },
  { 0x98, 1, inst,   CL,  "cvtbl %b,%l" },
  { 0x99, 1, inst,   CL,  "cvtbw %b,%w" },
  { 0x9a, 1, inst,   CL,  "movzbl %b,%l" },
  { 0x9b, 1, inst,   CL,  "movzbw %b,%w" },
  { 0x9c, 1, inst,   CL,  "rotl %b,%l%" },
  { 0x9d, 1, inst,   CL,  "acbb %b,%b,%b,%2" },
  { 0x9e, 1, inst,   CL,  "movab %b,%l" },
  { 0x9f, 1, inst,   CL,  "pushab %a" },

  { 0xa0, 1, inst,   CL,  "addw2 %w,%w" },
  { 0xa1, 1, inst,   CL,  "addw3 %w,%w,%w" },
  { 0xa2, 1, inst,   CL,  "subw2 %w,%w" },
  { 0xa3, 1, inst,   CL,  "subw3 %w,%w,%w" },
  { 0xa4, 1, inst,   CL,  "mulw2 %w,%w" },
  { 0xa5, 1, inst,   CL,  "mulw3 %w,%w,%w" },
  { 0xa6, 1, inst,   CL,  "divw2 %w,%w" },
  { 0xa7, 1, inst,   CL,  "divw3 %w,%w,%w" },
  { 0xa8, 1, inst,   CL,  "bisw2 %w,%w" },
  { 0xa9, 1, inst,   CL,  "bisw3 %w,%w,%w" },
  { 0xaa, 1, inst,   CL,  "bicw2 %w,%w" },
  { 0xab, 1, inst,   CL,  "bicw3 %w,%w,%w" },
  { 0xac, 1, inst,   CL,  "xorw2 %w,%w" },
  { 0xad, 1, inst,   CL,  "xorw3 %w,%w,%w" },
  { 0xae, 1, inst,   CL,  "mnegw %w,%w" },
  { 0xaf, 1, inst,   CL,  "casew %w,%w,%w" },

  { 0xb0, 1, inst,   CL,  "movw %w,%w" },
  { 0xb1, 1, inst,   CL,  "cmpw %w,%w" },
  { 0xb2, 1, inst,   CL,  "mcomw %w,%w" },
  { 0xb3, 1, inst,   CL,  "bitw %w,%w" },
  { 0xb4, 1, inst,   CL,  "clrw %w" },
  { 0xb5, 1, inst,   CL,  "tstw %w" },
  { 0xb6, 1, inst,   CL,  "incw %w" },
  { 0xb7, 1, inst,   CL,  "decw %w" },
  { 0xb8, 1, inst,   CL,  "bispsw %w" },
  { 0xb9, 1, inst,   CL,  "bicpsw %w" },
  { 0xba, 1, inst,   CL,  "popr %w" },
  { 0xbb, 1, inst,   CL,  "pushr %w" },
  { 0xbc, 1, inst,   0,   "chmk" },
  { 0xbd, 1, inst,   0,   "chme" },
  { 0xbe, 1, inst,   0,   "chms" },
  { 0xbf, 1, inst,   0,   "chmu" },

  { 0xc0, 1, inst,   CL,  "addl2 %l,%l" },
  { 0xc1, 1, inst,   CL,  "addl3 %l,%l,%l" },
  { 0xc2, 1, inst,   CL,  "subl2 %l,%l" },
  { 0xc3, 1, inst,   CL,  "subl3 %l,%l,%l" },
  { 0xc4, 1, inst,   CL,  "mull2 %l,%l" },
  { 0xc5, 1, inst,   CL,  "mull3 %l,%l,%l" },
  { 0xc6, 1, inst,   CL,  "divl2 %l,%l" },
  { 0xc7, 1, inst,   CL,  "divl3 %l,%l,%l" },
  { 0xc8, 1, inst,   CL,  "bisl2 %l,%l" },
  { 0xc9, 1, inst,   CL,  "bisl3 %l,%l,%l" },
  { 0xca, 1, inst,   CL,  "bicl2 %l,%l" },
  { 0xcb, 1, inst,   CL,  "bicl3 %l,%l,%l" },
  { 0xcc, 1, inst,   CL,  "xorl2 %l,%l" },
  { 0xcd, 1, inst,   CL,  "xorl3 %l,%l,%" },
  { 0xce, 1, inst,   CL,  "mnegl %l,%l" },
  { 0xcf, 1, inst,   CL,  "casel %l,%l,%l" },

  { 0xd0, 1, inst,   CL,  "movl %l,%l" },
  { 0xd1, 1, inst,   CL,  "cmpl %l,%l" },
  { 0xd2, 1, inst,   CL,  "mcoml %l,%l" },
  { 0xd3, 1, inst,   CL,  "bitl %l,%l" },
  { 0xd4, 1, inst,   CL,  "clrl %l" },
  { 0xd5, 1, inst,   CL,  "tstl %l" },
  { 0xd6, 1, inst,   CL,  "incl %l" },
  { 0xd7, 1, inst,   CL,  "decl %l" },
  { 0xd8, 1, inst,   CL,  "adwc %l,%l" },
  { 0xd9, 1, inst,   CL,  "sbwc %l,%l" },
  { 0xda, 1, inst,   CL,  "mtpr %l,%l" },
  { 0xdb, 1, inst,   CL,  "mfpr %l,%l" },
  { 0xdc, 1, inst,   CL,  "movpsl %l" },
  { 0xdd, 1, inst,   CL,  "pushl %l" },
  { 0xde, 1, inst,   CL,  "moval %l,%l" },
  { 0xdf, 1, inst,   CL,  "pushal %a" },

  { 0xe0, 1, pushj,  CL,  "bbs %l,%a,%1" },
  { 0xe1, 1, pushj,  CL,  "bbc %l,%a,%1" },
  { 0xe2, 1, pushj,  CL,  "bbss %l,%a,%1" },
  { 0xe3, 1, pushj,  CL,  "bbcs %l,%a,%1" },
  { 0xe4, 1, pushj,  CL,  "bbsc %l,%a,%1" },
  { 0xe5, 1, pushj,  CL,  "bbcc %l,%a,%1" },
  { 0xe6, 1, pushj,  CL,  "bbssi %l,%a,%1" },
  { 0xe7, 1, pushj,  CL,  "bbcci %l,%a,%1" },
  { 0xe8, 1, pushj,  CL,  "blbs %l,%1" },
  { 0xe9, 1, pushj,  CL,  "blbc %l,%1" },
  { 0xea, 1, inst,   CL,  "ffs %l,%b,%b,%l" },
  { 0xeb, 1, inst,   CL,  "ffc %l,%b,%b,%l" },
  { 0xec, 1, inst,   CL,  "cmpv %l,%b,%b,%l" },
  { 0xed, 1, inst,   CL,  "cmpzv %l,%b,%b,%l" },
  { 0xee, 1, inst,   CL,  "extv %l,%b,%b,%l" },
  { 0xef, 1, inst,   CL,  "extzv %l,%b,%b,%l" },

  { 0xf0, 1, inst,   CL,  "insv %l,%l,%b,%b" },
  { 0xf1, 1, inst,   CL,  "acbl %l,%l,%l,%2" },
  { 0xf2, 1, pushj,  CL,  "aoblss %l,%l,%1" },
  { 0xf3, 1, pushj,  CL,  "aobleq %l,%l,%1" },
  { 0xf4, 1, pushj,  CL,  "sobgeq %l,%1" },
  { 0xf5, 1, pushj,  CL,  "sobgtr %l,%1" },
  { 0xf6, 1, inst,   CL,  "cvtlb %l,%b" },
  { 0xf7, 1, inst,   CL,  "cvtlw %l,%w" },
  { 0xf8, 1, inst,   CL,  "ashp %b,%w,%a,%b,%w,%a" },
  { 0xf9, 1, inst,   CL,  "cvtlp %l,%w,%a" },
  { 0xfa, 1, pushj,  CL+EM, "callg %a,%a" },
  { 0xfb, 1, pushj,  CL+EM, "calls %l,%a" },
  { 0xfc, 1, inst,   0,   "xfc" },
  { 0xfd, 0, pfxFD,  0,   0 },
  { 0xfe, 0, unused, 0,   0 },
  { 0xff, 0, unused, 0,   0 },
};

static dispblock pfxFDdisp[] = {
  { 0x32, 2, inst,   CL,  "cvtdh %d,%h" },
  { 0x33, 2, inst,   CL,  "cvtgf %g,%f" },
  { 0x40, 2, inst,   CL,  "addg2 %g,%g" },
  { 0x41, 2, inst,   CL,  "addg3 %g,%g,%g" },
  { 0x42, 2, inst,   CL,  "subg2 %g,%g" },
  { 0x43, 2, inst,   CL,  "subg3 %g,%g,%g" },
  { 0x44, 2, inst,   CL,  "mulg2 %g,%g" },
  { 0x45, 2, inst,   CL,  "mulg3 %g,%g,%g" },
  { 0x46, 2, inst,   CL,  "divg2 %g,%g" },
  { 0x47, 2, inst,   CL,  "divg3 %g,%g,%g" },
  { 0x48, 2, inst,   CL,  "cvtgb %g,%b" },
  { 0x49, 2, inst,   CL,  "cvtgw %g,%w" },
  { 0x4a, 2, inst,   CL,  "cvtgl %g,%l" },
  { 0x4b, 2, inst,   CL,  "cvtrgl %g,%l" },
  { 0x4c, 2, inst,   CL,  "cvtbg %b,%g" },
  { 0x4d, 2, inst,   CL,  "cvtwg %w,%g" },
  { 0x4e, 2, inst,   CL,  "cvtlg %l,%g" },
  { 0x4f, 2, inst,   CL,  "acbg %g,%g,%g,%2" },
  { 0x50, 2, inst,   CL,  "movg %g,%g" },
  { 0x51, 2, inst,   CL,  "cmpg %g,%g" },
  { 0x52, 2, inst,   CL,  "mnegg %g,%g" },
  { 0x53, 2, inst,   CL,  "tstg %g" },
  { 0x54, 2, inst,   CL,  "emodg %g,%w,%g,%l,%g" },
  { 0x55, 2, inst,   CL,  "polyg %g,%w,%?" },
  { 0x56, 2, inst,   CL,  "cvtgh %g,%h" },
  { 0x60, 2, inst,   CL,  "addh2 %h,%h" },
  { 0x61, 2, inst,   CL,  "addh3 %h,%h,%h" },
  { 0x62, 2, inst,   CL,  "subh2 %h,%h" },
  { 0x63, 2, inst,   CL,  "subh3 %h,%h,%h" },
  { 0x64, 2, inst,   CL,  "mulh2 %h,%h" },
  { 0x65, 2, inst,   CL,  "mulh3 %h,%h,%h" },
  { 0x66, 2, inst,   CL,  "divh2 %h,%h" },
  { 0x67, 2, inst,   CL,  "divh3 %h,%h,%h" },
  { 0x68, 2, inst,   CL,  "cvthb %h,%b" },
  { 0x69, 2, inst,   CL,  "cvthw %h,%w" },
  { 0x6a, 2, inst,   CL,  "cvthl %h,%l" },
  { 0x6b, 2, inst,   CL,  "cvtrhl %h,%l" },
  { 0x6c, 2, inst,   CL,  "cvtbh %b,%h" },
  { 0x6d, 2, inst,   CL,  "cvtwh %w,%h" },
  { 0x6e, 2, inst,   CL,  "cvtlh %l,%h" },
  { 0x6f, 2, inst,   CL,  "acbh %h,%h,%h,%2" },
  { 0x70, 2, inst,   CL,  "movh %h,%h" },
  { 0x71, 2, inst,   CL,  "cmph %h,%h" },
  { 0x72, 2, inst,   CL,  "mnegh %h,%h" },
  { 0x73, 2, inst,   CL,  "tsth %h" },
  { 0x74, 2, inst,   CL,  "emodh %h,%w,%h,%l,%h" },
  { 0x75, 2, inst,   CL,  "polyh %h,%w,%?" },
  { 0x76, 2, inst,   CL,  "cvthg %h,%g" },
  { 0x7c, 2, inst,   CL,  "clro %o" },
  { 0x7d, 2, inst,   CL,  "movo %o" },
  { 0x7e, 2, inst,   CL,  "movao %o,%l" },
  { 0x7f, 2, inst,   CL,  "pushao %a" },
  { 0x98, 2, inst,   CL,  "cvtfh %f,%h" },
  { 0x99, 2, inst,   CL,  "cvtfg %f,%g" },
  { 0xf6, 2, inst,   CL,  "cvthf %h,%f" },
  { 0xf7, 2, inst,   CL,  "cvthd %h,%d" },
  { 0x00, 0, arnold, 0,   0 },
};

/**********************************************************************/

/* configurable variables: */

extern int radix;

/*
** Start of our local variables:
*/

static address* touch;

/************************************************************************/

/*
** Print a number in hexadecimal.
*/

static void hex(longword l)
{
  if (l > 9) casestring("^x");
  bufhex(l, 1);
}

/*
** Print a number in octal.
*/

static void octal(longword l)
{
  if (l > 7) bufchar('0');
  bufoctal(l, 1);
}

/*
** Print a number in decimal.
*/

static void decimal(longword l)
{
  bufdecimal(l, 1);
}

/*
** number() prints out a number in the selected radix.
*/

static void number(longword l)
{
  switch(radix) {
    case 8:  octal(l); break;
    case 10: decimal(l); break;
    case 16: hex(l); break;
    default: decimal(l); break;
  }
}

/* output a signed number */

static void signum(long l)
{
  if (l < 0) {
    bufchar('-');
    l = -l;
  }
  number(l);
}

/*
** putreg() outputs the name of a register.
*/

static void putreg(byte reg)
{
  switch (reg) {
    case 0xc: casestring("ap"); break;
    case 0xd: casestring("fp"); break;
    case 0xe: casestring("sp"); break;
    case 0xf: casestring("pc"); break;
    default:  casechar('r'); decimal(reg); break;
  }
}

/*
** printmask() prints a word as a register mask.
*/

static void printmask(word w)
{
  bool virgin = true;
  word bitmask = 0x0001;
  int bitpos = 0;

  casestring("^m<");
  while (bitpos < 15) {
    if (w & bitmask) {
      if (!virgin) {
	bufchar(',');
      }
      putreg(bitpos);
      virgin = false;
    }
    bitmask <<= 1;
    bitpos += 1;
  }
  bufchar('>');
}

/*
** This routine will generate a label on the specified address, unless
** one already exist.
*/

static void genlabel(address* addr)
{
  char work[10];

  if (!l_exist(addr)) {
    sprintf(work, "L%05lx", (a_a2l(addr) & 0xfffff));
    while(l_lookup(work) != nil) {
      sprintf(work, "L%lx", uniq());
    }
    l_insert(addr, work);
  }
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
** domask() will output the current item as a register mask (16 bits).
*/

static void domask(word w)
{
  pb_length = 2;

  if (l_exist(istart)) {
    if (listformat) {		/* Here we do most of startline() */
      starthex(4);		/*  by hand, since we don't want */
      tabto(22);		/*  the label. */
      bufmark();
    }
    casestring(".entry");
    tabdelim();
    reference(istart);
    bufstring(l_find(istart));
    endref();
    bufstring(", ");
  } else {
    startline(true);
    casestring(".word");
    tabdelim();
  }
  printmask(w);
  checkblank();
}
  
/*
** dolong() will output the current item as a longword (32 bits) of data.
*/

static void dolong(longword l)
{
  pb_length = 4;
  startline(true);
  casestring(".long");
  tabdelim();
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

  pb_length = 4;
  a = a_l2a(l);
  if (l_exist(a)) {
    startline(true);
    casestring(".long");
    tabdelim();
    reference(a);
    bufstring(l_find(a));
    endref();
    checkblank();
  } else {
    dolong(l);
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

/*
** doinstr() is the main workhorse.  This is the place you will
** have to write some serious code.
*/

static void disp(long offset, byte reg, bool indirect)
{
  if (indirect) {
    bufchar('@');
  }
  if (reg == 0x0f) {		/* PC? */
    touch = a_offset(pc, offset);
    if (l_exist(touch)) {
      reference(touch);
      bufstring(l_find(touch));
      endref();
      if (indirect) {
	touch = nil;
      }
      return;
    }
    if (updateflag) {
      genlabel(touch);
    }
  }
  if (indirect) {
    touch = nil;
  }
  signum(offset);
  bufchar('(');
  putreg(reg);
  bufchar(')');
}

static void iconst(longword n)
{
  bufchar('#');
  number(n);
}

static void qconst(void)
{
  a_inc(pc, 8);
  bufchar('#');
  bufstring("quad");
}

static void oconst(void)
{
  a_inc(pc, 16);
  bufchar('#');
  bufstring("octa");
}

static void fconst(void)
{
  a_inc(pc, 4);
  bufchar('#');
  bufstring("float");
}

static void dconst(void)
{
  a_inc(pc, 8);
  bufchar('#');
  bufstring("double");
}

static void gconst(void)
{
  a_inc(pc, 8);
  bufchar('#');
  bufstring("gfloat");
}

static void hconst(void)
{
  a_inc(pc, 16);
  bufchar('#');
  bufstring("hfloat");
}

static void refarg(char type)
{
  byte arg;
  byte mode, reg;

  bool indexflag;
  byte indexreg;

  address* target;
  longword l;

  touch = nil;
  indexflag = false;
  arg = getbyte();
  mode = arg >> 4;
  reg = arg & 0x0f;

  if (mode == 4) {
    indexflag = true;
    indexreg = reg;
    arg = getbyte();
    mode = arg >> 4;
    reg = arg & 0x0f;
  }

  if (arg == 0x8f) {
    switch (type) {
      case 'b': iconst(getbyte()); break;
      case 'w': iconst(getword()); break;
      case 'l': iconst(getlong()); break;
      case 'q': qconst(); break;
      case 'o': oconst(); break;
      case 'f': fconst(); break;
      case 'd': dconst(); break;
      case 'g': gconst(); break;
      case 'h': hconst(); break;
      case 'a': a_inc(pc, 4); bufstring("address"); break;
    }
  } else if (arg == 0x9f) {
    bufchar('@');
    l = getlong();
    target = a_l2a(l);
    if (l_exist(target)) {
      reference(target);
      bufstring(l_find(target));
      endref();
    } else {
      bufchar('#');
      number(l);
    }
  } else {
    switch (mode) {
      case 0x0:
      case 0x1:
      case 0x2:
      case 0x3: bufchar('#'); number(arg); break;

      case 0x5: putreg(reg); break;
      case 0x6: bufstring("(");  putreg(reg); bufstring(")");  break;
      case 0x7: bufstring("-("); putreg(reg); bufstring(")");  break;
      case 0x8: bufstring("(");  putreg(reg); bufstring(")+"); break;
      case 0x9: bufstring("@("); putreg(reg); bufstring(")+"); break;
      case 0xa: disp(sextb(getbyte()), reg, false); break;
      case 0xb: disp(sextb(getbyte()), reg, true);  break;
      case 0xc: disp(sextw(getword()), reg, false); break;
      case 0xd: disp(sextw(getword()), reg, true);  break;
      case 0xe: disp(sextl(getlong()), reg, false); break;
      case 0xf: disp(sextl(getlong()), reg, true);  break;
    }
  }
  if (indexflag) {
    bufchar('[');
    putreg(indexreg);
    bufchar(']');
    touch = nil;
  }
  switch (type) {
    case 'b': suggest(touch, st_byte, 1); break;
    case 'w': suggest(touch, st_word, 2); break;
    case 'l': suggest(touch, st_long, 4); break;
    case 'q': suggest(touch, st_quad, 8); break;
    case 'o': suggest(touch, st_octa, 16); break;
    case 'f': suggest(touch, st_float, 4); break;
    case 'd': suggest(touch, st_double, 8); break;
  }
}

static void refdisp(long l)
{
  touch = a_offset(pc, l);
  if (l_exist(touch)) {
    reference(touch);
    bufstring(l_find(touch));
    endref();
  } else {
    if (updateflag) {
      genlabel(touch);
    }
    l += pb_length;		/* Adjust for (pc) - (istart) */
    bufchar('.');
    if (l < 0) {
      bufchar('-'); number(-l);
    }
    if (l > 0) {
      bufchar('+'); number(l);
    }
  }
}

static void skip(int i)
{
  while (i-- > 0) {
    pb_length += 1;
    (void) getbyte();
  }
}

static bool chkarg(char type)
{
  byte arg;
  byte mode, reg;

  arg = getbyte();
  pb_length += 1;

  if (arg <= 0x3f) {		/* Literal? */
    return(true);		/*   Yes, that's OK. */
  }

  mode = arg >> 4;
  reg = arg & 0x0f;

  if (mode == 4) {		/* Index? */
    if (reg == 0x0f) {		/*   Yes, indexing with PC? */
      return(false);		/*     Yes, naugthy. */
    }
    arg = getbyte();
    pb_length += 1;

    if (arg <= 0x5f) {		/* Can't index literal, index or register. */
      return(false);
    }

    mode = arg >> 4;
    reg = arg & 0x0f;
  }

  if (reg == 0x0f) {		/* PC? */
    if (mode <= 7) {
      return(false);
    }
    if (mode == 8) {
      switch (type) {
	case 'b': skip(1); return(true);
	case 'w': skip(2); return(true);
	case 'l': skip(4); return(true);
	case 'q': skip(8); return(true);
	case 'o': skip(16);return(true);
	case 'f': skip(4); return(true);
	case 'd': skip(8); return(true);
	case 'g': skip(8); return(true);
	case 'h': skip(16);return(true);
	case 'a': skip(4); return(true);
      }
    }
    if (mode == 9) {
      skip(4);
      return(true);
    }
  }

  switch (mode) {
    case 0xa: skip(1); return(true);	/* disp(Rn) */
    case 0xb: skip(1); return(true);	/* @disp(Rn) */
    case 0xc: skip(2); return(true);	/* disp(Rn) */
    case 0xd: skip(2); return(true);	/* @disp(Rn) */
    case 0xe: skip(4); return(true);	/* disp(Rn) */
    case 0xf: skip(4); return(true);	/* @disp(Rn) */
  }
  return(true);		/* Can this happen? */
}

static bool computelength(char* expand)
{
  char c;

  while ((c = *(expand++)) != (char) 0) {
    if (c == '%') {
      c = *(expand++);
      switch (c) {
      case '1':
	pb_length += 1;
	break;
      case '2':
	pb_length += 2;
	break;
      default:
	if (!chkarg(c)) {
	  return(false);
	}
	break;
      }
    }
  }
  return(true);
}

static void copytext(char* expand)
{
  char c;

  while ((c = *(expand++)) != (char) 0) {
    if (c == '%') {
      c = *(expand++);
      switch(c) {
	case '1': refdisp(sextb(getbyte())); break;
	case '2': refdisp(sextw(getword())); break;
	default: refarg(c); break;
      }
    } else if (c == ' ') {
      tabdelim();
    } else if (c == ',') {
      argdelim(",");
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
  case pfxFD:
    disp = finddisp(getbyte(), pfxFDdisp);
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

  if (disp->flags & CL) {
    pushpc();
    if (!computelength(disp->expand)) {
      dobyte(opcode);
      return;
    }
    poppc();
  }

  if (overrun()) {
    dobyte(opcode);
    return;
  }

  startline(true);

  copytext(disp->expand);

  if (disp->flags & EM) {
    if (touch != nil) {
      suggest(touch, st_mask, 2);
      a_inc(touch, 2);
    }
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
    bufchar('=');
    number(a_a2l(a));
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

void vax_spec(address* a, int func)
{
  if (func == SPC_BEGIN) {
    casestring(".title  .main");
    bufblankline();
    foreach(checkunmap);
    bufblankline();
  }

  if (func == SPC_ORG) {
    bufblankline();
    casestring(".org");
    spacedelim();
    bufstring(a_a2str(a));
    bufblankline();
  }

  if (func == SPC_END) {
    bufblankline();
    casestring(".end");
  }
}

/*
** the main entry is the peek routine.  This should need a minimum of work.
*/

void vax_peek(stcode prefer)
{
  /* We wait with expanding data, labels etc. until we know length... */

  stddescription();

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
      doptr(getlong());
      break;
    case st_mask:
      domask(getword());
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

/************************************************************************/

/*
** Canonicalize a label.
*/

char* vax_lcan(char* name)
{
  static char work[32];

  return(canonicalize(name, work, 31));
}

/*
** Check a label for valid syntax.
*/

bool vax_lchk(char* name)
{
  return(checkstring(name, ".$_", "0123456789.$_"));
}

/*
** Generate a label at a specified address.
*/

void vax_lgen(address* addr)
{
  genlabel(addr);
}

/*
** Return list of good starting points.
*/

address* vax_auto(void)
{
  return(nil);			/* Nothing known yet. */
}

void vax_init(void)
{
  /* Set up our functions: */
  
  spf_peek(vax_peek);
  spf_spec(vax_spec);
  spf_lcan(vax_lcan);
  spf_lchk(vax_lchk);
  spf_lgen(vax_lgen);
  spf_auto(vax_auto);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line, expanded. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = false;		/* We are little-endian. */
  pv_cstart = ";";		/* Default comment start string. */
}

bool vax_help(int helptype)
{
  switch (helptype) {
  case hty_general:
    bufstring("\
Help string for VAX processors.\n\
");
    return(true);
  }
  return(false);
}
