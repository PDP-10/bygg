/*
** template for mips driver module.
*/

#include "disass.h"

evf_init mips_init;
evf_help mips_help;

struct entryvector mips_vector = {
  "mips",			/* Name. */
  "mips (32-bit)",		/* One-liner. */
  mips_init,			/* Init routine. */
  mips_help,			/* Help routine. */
};

evf_init mips64_init;
evf_help mips64_help;

struct entryvector mips64_vector = {
  "mips64",			/* Name. */
  "mips (64-bit)",		/* One-liner. */
  mips64_init,			/* Init routine. */
  mips64_help,			/* Help routine. */
};

evf_init octeon_init;
evf_help octeon_help;

struct entryvector octeon_vector = {
  "octeon",
  "Cavium octeon",
  octeon_init,
  octeon_help,
};

/************************************************************************/

/*
**
** Instruction types:
**
**          3                   2                   1                   0
**        1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
**       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**    I: !   opcode  !    rs   !    rt   !          immediate            !
**       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**    J: !   opcode  !                    target                         !
**       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**    R: !   opcode  !    rs   !    rt   !    rd   ! amount  !  function !
**       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**         fc......   .3e.....  ..1f....  ....f8..  .....7c.   ......3f
**
** str. params:
**
**   $s -- rs decoded.
**   $t -- rt decoded.
**   $d -- rd decoded.
**
**   #m-nc -- numeric field, bits m to n, inclusive, with conversion
**            c, where c can be "u", "s", "o" or "x", for unsigned
**          decimal, signed decimal, octal or hex, respectively.
**
**   %3 -- three bit value at end of word.
**   %5 -- five bit value from "rt" field.
**   %d -- displacement (branch offset)
**   %i -- immed. constant.
**   %t -- target for jump.
**   %a -- amount (five bits).
**
**   %* -- return.
**
*/

typedef struct mipsdisp {
  longword op;
  longword mask;
  char* exp;
  bool (*check)(void);
} mipsdisp;

/*
** fwd declare check functions:
*/

static bool chk_octeon(void);
static bool chk_64(void);

static bool fix_insext(void);

/*
** Opcode table for special instructions.
** opcode == 0, starting with 0x00..0x03
*/

mipsdisp spcdisp[] = {
  { 0x00000000, 0xffffffff, "nop" },
  { 0x00000040, 0xffffffff, "ssnop" },
  { 0x000000c0, 0xffffffff, "ehb" },
  { 0x00000000, 0xffe0003f, "sll $d,$t,%a" },
  /* movci goes here */
  { 0x00000002, 0xffe0003f, "srl $d,$t,%a" },
  { 0x00200002, 0xffe0003f, "rotr $d,$t,%a" },
  { 0x00000003, 0xffe0003f, "sra $d,$t,%a" },
  { 0x00000004, 0xfc0007ff, "sllv $d,$t,$s" },
  { 0x00000006, 0xfc0007ff, "srlv $d,$t,$s" },
  { 0x00000046, 0xfc0007ff, "rotrv $d,$t,$s" },
  { 0x00000007, 0xfc0007ff, "srav $d,$t,$s" },

  { 0x00000008, 0xfc1fffff, "jr $s%*" },
  { 0x0000f809, 0xfc1fffff, "jalr $s" },
  { 0x00000009, 0xfc1f83ff, "jalr $d,$s" },
  { 0x0000000a, 0xfc0007ff, "movz $d,$s,$t" },
  { 0x0000000b, 0xfc0007ff, "movn $d,$s,$t" },
  { 0x0000000c, 0xfc0007ff, "syscall" },
  { 0x0000000d, 0xfc00003f, "break" },
  { 0x0000000f, 0xffffffff, "sync" },

  { 0x00000010, 0xffff07ff, "mfhi $d" },
  { 0x00000011, 0xfc1fffff, "mthi $s" },
  { 0x00000012, 0xffff07ff, "mflo $d" },
  { 0x00000013, 0xfc1fffff, "mtlo $s" },

  { 0x00000014, 0xfc0007ff, "dsllv $d,$t,$s", chk_64 },
  /* special 15 not known to me. */
  { 0x00000016, 0xfc0007ff, "dsrlv $d,$t,$s", chk_64 },
  { 0x00000056, 0xfc0007ff, "drotrv $d,$t,$s", chk_64 },
  { 0x00000017, 0xfc0007ff, "dsrav $d,$t,$s", chk_64 },

  { 0x00000018, 0xfc00ffff, "mult $s,$t" },
  { 0x00000019, 0xfc00ffff, "multu $s,$t" },
  { 0x0000001a, 0xfc00ffff, "div $s,$t" },
  { 0x0000001b, 0xfc00ffff, "divu $s,$t" },
  { 0x0000001c, 0xfc00ffff, "dmult $s,$t", chk_64 },
  { 0x0000001d, 0xfc00ffff, "dmultu $s,$t", chk_64 },
  { 0x0000001e, 0xfc00ffff, "ddiv $s,$t", chk_64 },
  { 0x0000001f, 0xfc00ffff, "ddivu $s,$t", chk_64 },

  { 0x00000020, 0xfc0007ff, "add $d,$t,$s" },
  { 0x00000021, 0xfc0007ff, "addu $d,$t,$s" },
  { 0x00000022, 0xfc0007ff, "sub $d,$t,$s" },
  { 0x00000023, 0xfc0007ff, "subu $d,$t,$s" },
  { 0x00000024, 0xfc0007ff, "and $d,$t,$s" },
  { 0x00000025, 0xfc0007ff, "or $d,$t,$s" },
  { 0x00000026, 0xfc0007ff, "xor $d,$t,$s" },
  { 0x00000027, 0xfc0007ff, "nor $d,$t,$s" },

  /* special 28 not known to me. */
  /* special 29 not known to me. */

  { 0x0000002a, 0xfc0007ff, "slt $d,$t,$s" },
  { 0x0000002b, 0xfc0007ff, "sltu $d,$t,$s" },
  { 0x0000002c, 0xfc0007ff, "dadd $d,$s,$t", chk_64 },
  { 0x0000002d, 0xfc0007ff, "daddu $d,$s,$t", chk_64 },
  { 0x0000002e, 0xfc0007ff, "dsub $d,$t,$s", chk_64 },
  { 0x0000002f, 0xfc0007ff, "dsubu $d,$t,$s", chk_64 },

  { 0x00000030, 0xfc00003f, "tge $s,$t" }, /* What about code field? */
  { 0x00000031, 0xfc00003f, "tgeu $s,$t" },
  { 0x00000032, 0xfc00003f, "tlt $s,$t" },
  { 0x00000033, 0xfc00003f, "tltu $s,$t" },
  { 0x00000034, 0xfc00003f, "teq $s,$t" },

  /* special 35 not known to me. */

  { 0x00000036, 0xfc00003f, "tne $s,$t" },

  /* special 37 not known to me. */

  { 0x00000038, 0xffe0003f, "dsll $d,$t,%a", chk_64 },

  /* special 39 not known to me. */

  { 0x0000003a, 0xffe0003f, "dsrl $d,$t,%a", chk_64 },
  { 0x0020003a, 0xffe0003f, "drotr $d,$t,%a", chk_64 },
  { 0x0000003b, 0xffe0003f, "dsra $d,$t,%a", chk_64 },
  { 0x0000003c, 0xffe0003f, "dsll32 $d,$t,%a", chk_64 },
  { 0x0000003e, 0xffe0003f, "dsrl32 $d,$t,%a", chk_64 },
  { 0x0020003e, 0xffe0003f, "drotr32 $d,$t,%a", chk_64 },
  { 0x0000003f, 0xffe0003f, "dsra32 $d,$t,%a", chk_64 },

  { 0x00000000, 0x00000000, NULL },
};

/*
** Opcode table for regimm instructions.
** opcode == 1, starting with 0x04..0x07
*/

mipsdisp rimdisp[] = {
  { 0x04000000, 0xfc1f0000, "bltz $s,%d" },
  { 0x04010000, 0xfc1f0000, "bgez $s,%d" },
  { 0x04020000, 0xfc1f0000, "bltzl $s,%d" },
  { 0x04030000, 0xfc1f0000, "bgezl $s,%d" },
  { 0x04080000, 0xfc1f0000, "tgei $s,%i" },
  { 0x04090000, 0xfc1f0000, "tgeiu $s,%i" },
  { 0x040a0000, 0xfc1f0000, "tlti $s,%i" },
  { 0x040b0000, 0xfc1f0000, "tltiu $s,%i" },
  { 0x040c0000, 0xfc1f0000, "teqi $s,%i" },
  { 0x040e0000, 0xfc1f0000, "tnei $s,%i" },
  { 0x04100000, 0xfc1f0000, "bltzal $s,%d" },
  { 0x04110000, 0xffff0000, "bal %d" },	/* idiom. */
  { 0x04110000, 0xfc1f0000, "bgezal $s,%d" },
  { 0x04120000, 0xfc1f0000, "bltzlal $s,%d" },
  { 0x04130000, 0xfc1f0000, "bgezlal $s,%d" },
  { 0x041f0000, 0xfc1f0000, "synci %i($s)" },
  { 0x00000000, 0x00000000, NULL },
};

/*
** Opcode table for cop0 instructions.
** opcode == 020, starting with 0x40..0x43
*/

mipsdisp cop0disp[] = {
  { 0x40000000, 0xffe007ff, "mfc0 $t,$d" },
  { 0x40000000, 0xffe007f8, "mfc0 $t,$d,%3" },

  { 0x40200000, 0xffe007ff, "dmfc0 $t,$d", chk_64 },
  { 0x40200000, 0xffe007f8, "dmfc0 $t,$d,%3", chk_64 },

  { 0x40800000, 0xffe007ff, "mtc0 $t,$d" },
  { 0x40800000, 0xffe007f8, "mtc0 $t,$d,%3" },

  { 0x40a00000, 0xffe007ff, "dmtc0 $t,$d", chk_64 },
  { 0x40a00000, 0xffe007f8, "dmtc0 $t,$d,%3", chk_64 },

  { 0x41400000, 0xffe007ff, "rdpgpr $d,$t" },
  { 0x41606000, 0xffffffff, "di" },
  { 0x41606000, 0xffe0ffff, "di $t" },
  { 0x41606020, 0xffffffff, "ei" },
  { 0x41606020, 0xffe0ffff, "ei $t" },
  { 0x41c00000, 0xffe007ff, "wrpgpr $d,$t" },

  { 0x42000001, 0xffffffff, "tlbr" },
  { 0x42000002, 0xffffffff, "tlbwi" },
  { 0x42000006, 0xffffffff, "tlbwr" },
  { 0x42000008, 0xffffffff, "tlbp" },
  { 0x42000018, 0xffffffff, "eret%*" }, /* NO delay slot... */
  { 0x4200001f, 0xffffffff, "deret%*" }, /* NO delay slot... */
  { 0x42000020, 0xfe00003f, "wait" },

  { 0x00000000, 0x00000000, NULL },
};

/*
** Opcode table for cop1 instructions.
** opcode == 021, starting with 0x44..0x47
*/

mipsdisp cop1disp[] = {
  { 0x44000000, 0xffe007ff, "mfc1 $t,$d" },
  { 0x44400000, 0xffe007ff, "cfc1 $t,$d" },
  { 0x44800000, 0xffe007ff, "mtc1 $t,$d" },
  { 0x44c00000, 0xffe007ff, "ctc1 $t,$d" },
  { 0x45000000, 0xffff0000, "bc1f %d" },
  { 0x45010000, 0xffff0000, "bc1t %d" },
  { 0x45020000, 0xffff0000, "bc1fl %d" },
  { 0x45030000, 0xffff0000, "bc1tl %d" },
  //  { 0x46000000, 0xfe000000, "cop1 *****" },

  { 0x00000000, 0x00000000, NULL },
};

/*
** Opcode table for cop2 instructions.
** opcode == 022, starting with 0x48..0x4b
*/

mipsdisp cop2disp[] = {
  { 0x48000000, 0xffe007ff, "mfc2 $t,$d" },
  { 0x48400000, 0xffe007ff, "cfc2 $t,$d" },
  { 0x48800000, 0xffe007ff, "mtc2 $t,$d" },
  { 0x48c00000, 0xffe007ff, "ctc2 $t,$d" },
  { 0x49000000, 0xffff0000, "bc2f %d" },
  { 0x49010000, 0xffff0000, "bc2t %d" },
  { 0x49020000, 0xffff0000, "bc2fl %d" },
  { 0x49030000, 0xffff0000, "bc2tl %d" },
  //  { 0x4a000000, 0xfe000000, "cop2 *****" },

  { 0x00000000, 0x00000000, NULL },
};

/*
** Opcode table for cop1x instructions.
** opcode == 023, starting with 0x4c..0x4f
*/

mipsdisp cop1xdisp[] = {
  { 0x4c000008, 0xfc0007ff, "swxc1 $d,%5($s)" },
  { 0x4c000009, 0xfc0007ff, "sdxc1 $d,%5($s)" },
  { 0x4c00000d, 0xfc0007ff, "suxc1 $d,%5($s)" },

  { 0x00000000, 0x00000000, NULL },
};

/*
** Opcode table for special2 instructions.
** opcode == 034, starting with 0x70..0x73
*/

mipsdisp spc2disp[] = {
  { 0x70000000, 0xfc00ffff, "madd $s,$t" },
  { 0x70000001, 0xfc00ffff, "maddu $s,$t" },
  { 0x70000002, 0xfc0007ff, "mul $d,$s,$t" },
  { 0x70000003, 0xfc0007ff, "dmul $d,$s,$t", chk_octeon },
  { 0x70000004, 0xfc00ffff, "msub $s,$t" },
  { 0x70000005, 0xfc00ffff, "msubu $s,$t" },
  { 0x70000008, 0xfc1fffff, "mtm0 $s", chk_octeon },
  { 0x70000009, 0xfc1fffff, "mtp0 $s", chk_octeon },
  { 0x7000000a, 0xfc1fffff, "mtp1 $s", chk_octeon },
  { 0x7000000b, 0xfc1fffff, "mtp2 $s", chk_octeon },
  { 0x7000000c, 0xfc1fffff, "mtm1 $s", chk_octeon },
  { 0x7000000d, 0xfc1fffff, "mtm2 $s", chk_octeon },
  { 0x7000000f, 0xfc0007ff, "vmulu $d,$s,$t", chk_octeon },

  { 0x70000010, 0xfc0007ff, "vmm0 $d,$s,$t", chk_octeon },
  { 0x70000011, 0xfc0007ff, "v3mulu $d,$s,$t", chk_octeon },

  { 0x70000020, 0xfc0007ff, "clz $d,$s" },
  { 0x70000021, 0xfc0007ff, "clo $d,$s" },
  { 0x70000026, 0xfc0007ff, "dclz $d,$s", chk_64 },
  { 0x70000027, 0xfc0007ff, "dclo $d,$s", chk_64 },
  { 0x7000002a, 0xfc0007ff, "seq $d,$s,$t", chk_octeon },
  { 0x7000002b, 0xfc0007ff, "sne $d,$s,$t", chk_octeon },
  { 0x7000002c, 0xfc1f07ff, "pop $d,$s", chk_octeon },
  { 0x7000002d, 0xfc1f07ff, "dpop $d,$s", chk_octeon },
  { 0x7000002e, 0xfc00003f, "seqi $t,$s,#15-6d", chk_octeon },
  { 0x7000002f, 0xfc00003f, "snei $t,$s,#15-6d", chk_octeon },

  { 0x70000032, 0xfc00003f, "cins $t,$s,%a,#15-11x", chk_octeon },
  { 0x70000033, 0xfc00003f, "cins32 $t,$s,%a,#15-11x", chk_octeon },
  { 0x70000038, 0xfc0007ff, "baddu $d,$s,$t", chk_octeon },
  { 0x7000003a, 0xfc00003f, "exts $t,$s,%a,#15-11x", chk_octeon },
  { 0x7000003b, 0xfc00003f, "exts32 $t,$s,%a,#15-11x", chk_octeon },
  { 0x7000003f, 0xfc0007ff, "sdbbp #25-6x" },

  { 0x00000000, 0x00000000, NULL },
};

/*
** Opcode table for special3 instructions.
** opcode == 037, starting with 0x7c..0x7f
*/

mipsdisp spc3disp[] = {
  { 0x7c000000, 0xfc00003f, "ext $t,$s,%p,%s", fix_insext },
  { 0x7c000001, 0xfc00003f, "dextm $t,$s,%p,%s", fix_insext },
  { 0x7c000002, 0xfc00003f, "dextu $t,$s,%p,%s", fix_insext },
  { 0x7c000003, 0xfc00003f, "dext $t,$s,%p,%s", fix_insext },
  { 0x7c000004, 0xfc00003f, "ins $t,$s,%p,%s", fix_insext },
  { 0x7c000005, 0xfc00003f, "dinsm $t,$s,%p,%s", fix_insext },
  { 0x7c000006, 0xfc00003f, "dinsu $t,$s,%p,%s", fix_insext },
  { 0x7c000007, 0xfc00003f, "dins $t,$s,%p,%s", fix_insext },

  { 0x7c0000a0, 0xffe007ff, "wsbh $d,$t" },
  { 0x7c0000a4, 0xffe007ff, "dsbh $d,$t", chk_64 },
  { 0x7c000164, 0xffe007ff, "dshd $d,$t", chk_64 },
  { 0x7c000420, 0xffe007ff, "seb $d,$t" },
  { 0x7c000620, 0xffe007ff, "seh $d,$t" },

  { 0x7c00003b, 0xffe007ff, "rdhwr $t,$d" },

  { 0x00000000, 0x00000000, NULL },
};

/*
** Main opcode table:
*/

mipsdisp maindisp[] = {
  /* 0x00... special */
  /* 0x04... regimm */
  { 0x08000000, 0xfc000000, "j %t%*" },
  { 0x0c000000, 0xfc000000, "jal %t" },

  { 0x10000000, 0xffff0000, "b %d%*" }, /* idiom. */
  { 0x10000000, 0xfc000000, "beq $s,$t,%d" },
  { 0x14000000, 0xfc000000, "bne $s,$t,%d" },
  { 0x18000000, 0xfc1f0000, "blez $s,%d" },
  { 0x1c000000, 0xfc1f0000, "bgtz $s,%d" },

  { 0x20000000, 0xfc000000, "addi $t,$s,%i" },
  /* { 0x24000000, 0xffe00000, "li $t,%i" }, */ /* idiom. */
  { 0x24000000, 0xfc000000, "addiu $t,$s,%i" },
  { 0x28000000, 0xfc000000, "slti $t,$s,%i" },
  { 0x2c000000, 0xfc000000, "sltiu $t,$s,%i" },

  { 0x30000000, 0xfc000000, "andi $t,$s,%i" },
  { 0x34000000, 0xfc000000, "ori $t,$s,%i" },
  { 0x38000000, 0xfc000000, "xori $t,$s,%i" },
  { 0x3c000000, 0xffe00000, "lui $t,%i" },

  /* 0x40... cop0 (mmu/system) */
  /* 0x44... cop1 (floating point) */
  /* 0x48... cop2 (whatever) */
  /* 0x4c... cop1x (foo) */

  { 0x50000000, 0xfc000000, "beql $t,$s,%d" },
  { 0x54000000, 0xfc000000, "bnel $t,$s,%d" },
  { 0x58000000, 0xfc1f0000, "blezl $s,%d" },
  { 0x5c000000, 0xfc1f0000, "bgtzl $s,%d" },

  { 0x60000000, 0xfc000000, "daddi $t,$s,%i", chk_64 },
  { 0x64000000, 0xfc000000, "daddiu $t,$s,%i", chk_64 },
  { 0x68000000, 0xfc000000, "ldl $t,%i($s)" },
  { 0x6c000000, 0xfc000000, "ldr $t,%i($s)" },

  /* 0x70... special2 */
  /* 0x74... jalx???? */
  /* 0x70... mdmx???? */
  /* 0x7c... special3 */

  { 0x80000000, 0xfc000000, "lb $t,%i($s)" },
  { 0x84000000, 0xfc000000, "lh $t,%i($s)" },
  { 0x88000000, 0xfc000000, "lwl $t,%i($s)" },
  { 0x8c000000, 0xfc000000, "lw $t,%i($s)" },

  { 0x90000000, 0xfc000000, "lbu $t,%i($s)" },
  { 0x94000000, 0xfc000000, "lhu $t,%i($s)" },
  { 0x98000000, 0xfc000000, "lwr $t,%i($s)" },
  { 0x9c000000, 0xfc000000, "lwu $t,%i($s)" },

  { 0xa0000000, 0xfc000000, "sb $t,%i($s)" },
  { 0xa4000000, 0xfc000000, "sh $t,%i($s)" },
  { 0xa8000000, 0xfc000000, "swl $t,%i($s)" },
  { 0xac000000, 0xfc000000, "sw $t,%i($s)" },

  { 0xb0000000, 0xfc000000, "sdl $t,%i($s)" },
  { 0xb4000000, 0xfc000000, "sdr $t,%i($s)" },
  { 0xb8000000, 0xfc000000, "swr $t,%i($s)" },
  { 0xbc000000, 0xfc000000, "cache %5,%i($s)" },

  { 0xc0000000, 0xfc000000, "ll $t,%i($s)" },
  { 0xc4000000, 0xfc000000, "lwc1 $t,%i($s)" },
  { 0xc8000000, 0xfc000000, "bbit0 $s,%5,%d", chk_octeon },
  { 0xc8000000, 0xfc000000, "lwc2 $t,%i($s)" },
  { 0xcc000000, 0xfc000000, "pref %5,%i($s)" },

  { 0xd0000000, 0xfc000000, "lld $t,%i($s)" },
  { 0xd4000000, 0xfc000000, "ldc1 $t,%i($s)" },
  { 0xd8000000, 0xfc000000, "bbit032 $s,%5,%d", chk_octeon },
  { 0xd8000000, 0xfc000000, "ldc2 $t,%i($s)" },
  { 0xdc000000, 0xfc000000, "ld $t,%i($s)" },

  { 0xe0000000, 0xfc000000, "sc $t,%i($s)" },
  { 0xe4000000, 0xfc000000, "swc1 $t,%i($s)" },
  { 0xe8000000, 0xfc000000, "bbit1 $s,%5,%d", chk_octeon },
  { 0xe8000000, 0xfc000000, "swc2 $t,%i($s)" },
  /* 0xec... reserved for future */

  { 0xf0000000, 0xfc000000, "scd $t,%i($s)" },
  { 0xf4000000, 0xfc000000, "sdc1 $t,%i($s)" },
  { 0xf8000000, 0xfc000000, "bbit132 $s,%5,%d", chk_octeon },
  { 0xf8000000, 0xfc000000, "sdc2 $t,%i($s)" },
  { 0xfc000000, 0xfc000000, "sd $t,%i($s)" },

  { 0x00000000, 0x00000000, NULL },
};

/*
** local variables:
*/

static longword opword;
static address* touch;		/* Where in memory this instruction refers */

static enum {
  cpu_mips,			/* Standard, no extensions. */
  cpu_mips64,			/* 64-bit extensions. */
  cpu_octeon,			/* Cavium Octeon. */
} model;

int iepos, iesize;		/* ins/ext pos/size arguments, decoded. */

/************************************************************************/

static bool chk_64(void)
{
  switch (model) {
  case cpu_mips64:
  case cpu_octeon:
    return true;
  default:
    return false;
  }    
}

static bool chk_octeon(void)
{
  if (model == cpu_octeon)
    return true;

  return false;
}

/*
** handle the bit field magic needed for [d]{ins,ext}* instructions:
*/

static bool fix_insext(void)
{
  int msb, lsb;
  int code;

  msb = (opword >> 11) & 0x1f;
  lsb = (opword >> 6) & 0x1f;

  code = opword & 0x3f;

  switch (code) {
  case 0:			/* ext ... */
    iepos = lsb;
    iesize = msb + 1;
    return true;		/* 32-bit, always there. */
  case 4:			/* ins ... */
    iepos = lsb;
    iesize = msb + 1 - lsb;
    return true;		/* 32-bit, always there. */

  /*
   * the rest are 64-bit.
   */

  case 1:			/* dextm ... */
    iepos = lsb;
    iesize = msb + 33;
    break;
  case 2:			/* dextu ... */
    iepos = lsb + 32;
    iesize = msb + 1;
    break;
  case 3:			/* dext ... */
    iepos = lsb;
    iesize = msb + 1;
    break;
  case 5:			/* dinsm ... */
    iepos = lsb;
    iesize = msb + 33 - lsb;
    break;
  case 6:			/* dinsu ... */
    iepos = lsb + 32;
    iesize = msb + 1 - lsb;
    break;
  case 7:			/* dins ... */
    iepos = lsb;
    iesize = msb + 1 - lsb;
    break;
  }

  return chk_64();
}

/************************************************************************/

static void number(longword l)
{
  switch (radix) {
  case 8:
    if (l > 7)
      bufchar('0');
    bufoctal(l, 1);
    break;
  case 10:
    bufdecimal(l, 1);
    break;
  case 16:
  default:
    if (l > 9)
      bufstring("0x");
    bufhex(l, 1);
    break;
  }
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
** dobyte() will output the current item as a byte of data.
*/

static void mips_dobyte(void)
{
  byte b;

  b = getbyte();
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

/*
** dochar() will try to output its argument as a character.  If it can't,
** it will default to dobyte().
*/

static void mips_dochar(void)
{
  byte b;

  b = getbyte();
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();

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

/*
** doword() will output the current item as a word (16 bits) of data.
*/

static void mips_doword(void)
{
  word w;

  w = getword();
  pb_length = 2;
  startline(true);
  casestring(".half");
  tabdelim();
  number(w);
  checkblank();
}

/*
** dolong() will output the current item as a longword (32 bits) of data.
*/

static void mips_dolong(void)
{
  longword l;

  l = getlong();

  pb_length = 4;
  startline(true);
  casestring(".word");
  tabdelim();
  number(l);
  checkblank();
}

/*
** doptr() tries to output its argument as a pointer, i.e. if there is
** a label that corresponds to the argument, it will be used, otherwise
** we will setter for a numeric value.
*/

static void mips_doptr(void)
{
  longword l;
  address* a;
  
  pb_length = 4;		/* Pointers are four bytes here. */
  l = getlong();		/* Get bits. */
  a = a_l2a(l);			/* Get corresponding address. */
  startline(true);		/* Start off line. */
  casestring(".word");		/* Suitable pseudo-op. */
  tabdelim();			/* Tab (or space). */
  reference(a);			/* Note the reference, for highlighting etc. */
  if (l_exist(a)) {		/*   If we have a label, - */
    bufstring(l_find(a));	/*      use it. */
  } else {			/*   If not, - */
    number(l);			/*      just print the numeric value. */
  }
  endref();			/* Stop reference. */
  checkblank();			/* Check for possible blank line here. */
}

/*
** dotext() will try to decode a text constant.
*/

static void mips_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    setpc(istart);
    mips_dochar();
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
** doasciz() will decode an asciz string.
*/

static void mips_doasciz(void)
{
  mips_dotext();		/* ... for now ... */
}

/*
** doinstr() is the main workhorse.  This is the place you will
** have to write some serious code.
*/

static void prreg(int reg)
{
  bufchar('$');
  bufdecimal(reg, 1);
}

static void refdisp(void)
{
  long offset;

  offset = sextw(opword & 0xffff) << 2;
  offset += 4;
  touch = a_touch(a_offset(istart, offset));

  reference(touch);
  if (l_exist(touch)) {
    bufstring(l_find(touch));
  } else {
    if (updateflag) {
      genlabel(touch);
    }
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

static int embed(char* str)
{
  int m, n;
  int nch;
  char c;
  int val;

  if (sscanf(str, "%d-%d%c%n", &m, &n, &c, &nch) == 3) {
    /* mask out bits m..n of opword */

    val = (opword >> n) & ((2 << (m - n)) - 1);

    switch (c) {
    case 'd':
      bufdecimal(val, 1);
      break;
    case 'n':
      number(val);
      break;
    case 'o':
      bufoctal(val, 1);
      break;
    case 'x':
      bufhex(val, 1);
      break;
    }
    return nch;
  }
  return 0;
}

static void copytext(char* str)
{
  char c;

  while ((c = *str++) != 0) {
    if (c == '%') {
      c = *str++;
      switch (c) {
      case '3':			/* Three last bits. */
	number(opword & 7);
	break;
      case '5':			/* five bit value. */
	number((opword >> 16) & 0x1f); /* XXX decimal? */
	break;
      case 'a':			/* five-bit amount field. */
	number((opword >> 6) & 0x1f); /* XXX decimal? */
	break;
      case 'd':			/* displacement. */
	refdisp();
	pb_detour = touch;
	break;
      case 'i':			/* immed. constant. */
	number(opword & 0xffff);
	break;
      case 'p':			/* pos field for ins/ext. */
	bufdecimal(iepos, 0);
	break;
      case 's':			/* size field for ins/ext. */
	bufdecimal(iesize, 0);
	break;
      case 't':			/* jump target. */
	bufstring("%t");	/* FIXME! */
	pb_deadend = true;
	break;
      case '*':			/* signal end-of-instr-stream. */
	pb_deadend = true;
	break;
      }
    } else if (c == '#') {
      str += embed(str);
    } else if (c == '$') {
      c = *str++;
      switch (c) {
      case 's':
	prreg((opword >> 21) & 0x1f);
	break;
      case 't':
	prreg((opword >> 16) & 0x1f);
	break;
      case 'd':
	prreg((opword >> 11) & 0x1f);
	break;
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

static void mips_doinstr(void)
{
  int i;
  mipsdisp* disp;

  if (a_a2w(istart) & 3) {	/* Check alignment. */
    mips_dobyte();
    return;
  }

  opword = getlong();

  pb_length = 4;
  startline(true);

  switch (opword >> 26) {
  case 0: disp = spcdisp; break;
  case 1: disp = rimdisp; break;
  case 020: disp = cop0disp; break;
  case 021: disp = cop1disp; break;
  case 022: disp = cop2disp; break;
  case 023: disp = cop1xdisp; break;
  case 034: disp = spc2disp; break;
  case 037: disp = spc3disp; break;
  default: disp = maindisp; break;
  }

  for (i = 0; disp[i].mask != 0; i += 1) {
    if (disp[i].op == (opword & disp[i].mask)) {
      if (disp[i].check == NULL || (*disp[i].check)()) {
	copytext(disp[i].exp);
	if (updateflag && pb_deadend) {
	  suggest(pc, st_jump, 4);
	}
	return;
      }
    }
  }

  pb_deadend = true;
  pb_status = st_none;

  bufstring(".word");
  tabdelim();
  bufstring("0x");
  bufhex(opword, 8);

  if (!c_exist(istart)) {
    tabto(32);
    bufstring(pv_cstart);
    copytext("#31-26o,$s,$t,$d,%a,#5-0x");
  }
}

static void mips_dodelay(void)
{
  /* do delay slot here. */
  mips_doinstr();
  delayblank = true;
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
    bufstring("==");
    number(a_a2l(a));
    if (c_exist(a)) {
      tabto(32);
      bufchar(';');
      bufstring(c_find(a));
    }
    bufnewline();
  }
}

void mips_begin(void)
{
  bufstring(";Beginning of mips program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

void mips_org(address* a)
{
  bufblankline();
  casestring("org");
  spacedelim();
  bufstring(a_a2str(a));
  bufblankline();
}

void mips_end(void)
{
  bufblankline();
  bufstring(";End of mips program");
}

/************************************************************************/

char* mips_lcan(char* name)
{
  static char work[10];

  return canonicalize(name, work, 8);
}

bool mips_lchk(char* name)
{
  return checkstring(name, ".", "0123456789.");
}

void mips_lgen(address* addr)
{
  genlabel(addr);
}

/**********************************************************************/

void mips_init(void)
{
  /* Set up our functions: */
  
  spf_lcan(mips_lcan);
  spf_lchk(mips_lchk);
  spf_lgen(mips_lgen);

  /* set up our object handlers: */
  
  spf_dodef(mips_dobyte);

  spf_doobj(st_inst, mips_doinstr);
  spf_doobj(st_jump, mips_dodelay);
  spf_doobj(st_ptr,  mips_doptr);
  spf_doobj(st_long, mips_dolong);
  spf_doobj(st_word, mips_doword);
  spf_doobj(st_char, mips_dochar);
  spf_doobj(st_text, mips_dotext);
  spf_doobj(st_asciz, mips_doasciz);
  spf_doobj(st_byte, mips_dobyte);

  spf_begin(mips_begin);
  spf_end(mips_end);
  spf_org(mips_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = true;		/* Work as big-endian */
  pv_cstart = "#";		/* Comment start string. */

  model = cpu_mips;		/* Default cpu model. */
}

/*
** This routine prints a help string for this processor, so that the
** user can ask us what the **** we are.  We should give information on
** what the different assemblers mean and all that jazz.
*/

bool mips_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Missing...\n\
");
    return true;
  }
  return false;
}

void mips64_init(void)
{
  mips_init();

  model = cpu_mips64;
}

bool mips64_help(int helptype)
{
  return mips_help(helptype);
}

void octeon_init(void)
{
  mips_init();
  
  /* set octeon specials here. */

  model = cpu_octeon;
}

bool octeon_help(int helptype)
{
  return mips_help(helptype);
}
