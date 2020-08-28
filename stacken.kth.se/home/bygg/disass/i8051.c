/*
** This module implements driver code for the Intel 8051 (and friends)
** microcontrollers.
*/

#include "disass.h"

/**********************************************************************/

evf_init i8051_init;
evf_help i8051_help;

struct entryvector i8051_vector = {
  "8051",			/* Name */
  "Intel 8051",			/* One-liner. */
  i8051_init,			/* Init routine */
  i8051_help,			/* Help routine */
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

/*
** escape codes:
**
**  %1 - 8-bit inline constant.
**  %2 - 16-bit inline constant.
**  %a - 16-bit address.
**  %b - 8-bit bit number in bit space.
**  %d - 8-bit relative dispacement.
**  %i - 8-bit offset into i-ram.
**  %p - 11-bit abs. address, top 5 from pc.
*/

static dispblock i8051disp[256] = {
  { 0x00, 1, inst,  0, "nop" },
  { 0x01, 2, jrst,  0, "ajmp %p" },
  { 0x02, 3, jrst,  0, "ljmp %a" },
  { 0x03, 1, inst,  0, "rr a" },
  { 0x04, 1, inst,  0, "inc a" },
  { 0x05, 2, inst,  0, "inc %i" },
  { 0x06, 1, inst,  0, "inc @r0" },
  { 0x07, 1, inst,  0, "inc @r1" },
  { 0x08, 1, inst,  0, "inc r0" },
  { 0x09, 1, inst,  0, "inc r1" },
  { 0x0a, 1, inst,  0, "inc r2" },
  { 0x0b, 1, inst,  0, "inc r3" },
  { 0x0c, 1, inst,  0, "inc r4" },
  { 0x0d, 1, inst,  0, "inc r5" },
  { 0x0e, 1, inst,  0, "inc r6" },
  { 0x0f, 1, inst,  0, "inc r7" },
  { 0x10, 3, pushj, 0, "jbc %b,%d" },
  { 0x11, 2, pushj, 0, "acall %p" },
  { 0x12, 3, pushj, 0, "lcall %a" },
  { 0x13, 1, inst,  0, "rrc a" },
  { 0x14, 1, inst,  0, "dec a" },
  { 0x15, 2, inst,  0, "dec %i" },
  { 0x16, 1, inst,  0, "dec @r0" },
  { 0x17, 1, inst,  0, "dec @r1" },
  { 0x18, 1, inst,  0, "dec r0" },
  { 0x19, 1, inst,  0, "dec r1" },
  { 0x1a, 1, inst,  0, "dec r2" },
  { 0x1b, 1, inst,  0, "dec r3" },
  { 0x1c, 1, inst,  0, "dec r4" },
  { 0x1d, 1, inst,  0, "dec r5" },
  { 0x1e, 1, inst,  0, "dec r6" },
  { 0x1f, 1, inst,  0, "dec r7" },
  { 0x20, 3, pushj, 0, "jb %b,%d" },
  { 0x21, 2, jrst,  0, "ajmp %p" },
  { 0x22, 1, popj,  0, "ret" },
  { 0x23, 1, inst,  0, "rl a" },
  { 0x24, 2, inst,  0, "add a,%1" },
  { 0x25, 2, inst,  0, "add a,%i" },
  { 0x26, 1, inst,  0, "add a,@r0" },
  { 0x27, 1, inst,  0, "add a,@r1" },
  { 0x28, 1, inst,  0, "add a,r0" },
  { 0x29, 1, inst,  0, "add a,r1" },
  { 0x2a, 1, inst,  0, "add a,r2" },
  { 0x2b, 1, inst,  0, "add a,r3" },
  { 0x2c, 1, inst,  0, "add a,r4" },
  { 0x2d, 1, inst,  0, "add a,r5" },
  { 0x2e, 1, inst,  0, "add a,r6" },
  { 0x2f, 1, inst,  0, "add a,r7" },
  { 0x30, 3, pushj, 0, "jnb %b,%d" },
  { 0x31, 2, pushj, 0, "acall %p" },
  { 0x32, 1, popj,  0, "reti" },
  { 0x33, 1, inst,  0, "rlc a" },
  { 0x34, 2, inst,  0, "addc a,%1" },
  { 0x35, 2, inst,  0, "addc a,%i" },
  { 0x36, 1, inst,  0, "addc a,@r0" },
  { 0x37, 1, inst,  0, "addc a,@r1" },
  { 0x38, 1, inst,  0, "addc a,r0" },
  { 0x39, 1, inst,  0, "addc a,r1" },
  { 0x3a, 1, inst,  0, "addc a,r2" },
  { 0x3b, 1, inst,  0, "addc a,r3" },
  { 0x3c, 1, inst,  0, "addc a,r4" },
  { 0x3d, 1, inst,  0, "addc a,r5" },
  { 0x3e, 1, inst,  0, "addc a,r6" },
  { 0x3f, 1, inst,  0, "addc a,r7" },
  { 0x40, 2, pushj, 0, "jc %d" },
  { 0x41, 2, jrst,  0, "ajmp %p" },
  { 0x42, 2, inst,  0, "orl %i,a" },
  { 0x43, 3, inst,  0, "orl %i,%1" },
  { 0x44, 2, inst,  0, "orl a,%1" },
  { 0x45, 2, inst,  0, "orl a,%i" },
  { 0x46, 1, inst,  0, "orl a,@r0" },
  { 0x47, 1, inst,  0, "orl a,@r1" },
  { 0x48, 1, inst,  0, "orl a,r0" },
  { 0x49, 1, inst,  0, "orl a,r1" },
  { 0x4a, 1, inst,  0, "orl a,r2" },
  { 0x4b, 1, inst,  0, "orl a,r3" },
  { 0x4c, 1, inst,  0, "orl a,r4" },
  { 0x4d, 1, inst,  0, "orl a,r5" },
  { 0x4e, 1, inst,  0, "orl a,r6" },
  { 0x4f, 1, inst,  0, "orl a,r7" },
  { 0x50, 2, pushj, 0, "jnc %d" },
  { 0x51, 2, pushj, 0, "acall %p" },
  { 0x52, 2, inst,  0, "anl %i,a" },
  { 0x53, 3, inst,  0, "anl %i,%1" },
  { 0x54, 2, inst,  0, "anl a,%1" },
  { 0x55, 2, inst,  0, "anl a,%i" },
  { 0x56, 1, inst,  0, "anl a,@r0" },
  { 0x57, 1, inst,  0, "anl a,@r1" },
  { 0x58, 1, inst,  0, "anl a,r0" },
  { 0x59, 1, inst,  0, "anl a,r1" },
  { 0x5a, 1, inst,  0, "anl a,r2" },
  { 0x5b, 1, inst,  0, "anl a,r3" },
  { 0x5c, 1, inst,  0, "anl a,r4" },
  { 0x5d, 1, inst,  0, "anl a,r5" },
  { 0x5e, 1, inst,  0, "anl a,r6" },
  { 0x5f, 1, inst,  0, "anl a,r7" },
  { 0x60, 2, pushj, 0, "jz %d" },
  { 0x61, 2, jrst,  0, "ajmp %p" },
  { 0x62, 2, inst,  0, "xrl %i,a" },
  { 0x63, 3, inst,  0, "xrl %i,%1" },
  { 0x64, 2, inst,  0, "xrl a,%1" },
  { 0x65, 2, inst,  0, "xrl a,%i" },
  { 0x66, 1, inst,  0, "xrl a,@r0" },
  { 0x67, 1, inst,  0, "xrl a,@r1" },
  { 0x68, 1, inst,  0, "xrl a,r0" },
  { 0x69, 1, inst,  0, "xrl a,r1" },
  { 0x6a, 1, inst,  0, "xrl a,r2" },
  { 0x6b, 1, inst,  0, "xrl a,r3" },
  { 0x6c, 1, inst,  0, "xrl a,r4" },
  { 0x6d, 1, inst,  0, "xrl a,r5" },
  { 0x6e, 1, inst,  0, "xrl a,r6" },
  { 0x6f, 1, inst,  0, "xrl a,r7" },
  { 0x70, 2, pushj, 0, "jnz %d" },
  { 0x71, 2, pushj, 0, "acall %p" },
  { 0x72, 2, inst,  0, "orl c,%b" },
  { 0x73, 1, jrst,  0, "jmp @a+dptr" },
  { 0x74, 2, inst,  0, "mov a,%1" },
  { 0x75, 3, inst,  0, "mov %i,%1" },
  { 0x76, 2, inst,  0, "mov @r0,%1" },
  { 0x77, 2, inst,  0, "mov @r1,%1" },
  { 0x78, 2, inst,  0, "mov r0,%1" },
  { 0x79, 2, inst,  0, "mov r1,%1" },
  { 0x7a, 2, inst,  0, "mov r2,%1" },
  { 0x7b, 2, inst,  0, "mov r3,%1" },
  { 0x7c, 2, inst,  0, "mov r4,%1" },
  { 0x7d, 2, inst,  0, "mov r5,%1" },
  { 0x7e, 2, inst,  0, "mov r6,%1" },
  { 0x7f, 2, inst,  0, "mov r7,%1" },
  { 0x80, 2, jrst,  0, "sjmp %d" },
  { 0x81, 2, jrst,  0, "ajmp %p" },
  { 0x82, 2, inst,  0, "anl c,%b" },
  { 0x83, 1, inst,  0, "movc a,@a+pc" },
  { 0x84, 1, inst,  0, "div ab" },
  { 0x85, 3, inst,  0, "mov %i,%i" },
  { 0x86, 2, inst,  0, "mov %i,@r0" },
  { 0x87, 2, inst,  0, "mov %i,@r1" },
  { 0x88, 2, inst,  0, "mov %i,r0" },
  { 0x89, 2, inst,  0, "mov %i,r1" },
  { 0x8a, 2, inst,  0, "mov %i,r2" },
  { 0x8b, 2, inst,  0, "mov %i,r3" },
  { 0x8c, 2, inst,  0, "mov %i,r4" },
  { 0x8d, 2, inst,  0, "mov %i,r5" },
  { 0x8e, 2, inst,  0, "mov %i,r6" },
  { 0x8f, 2, inst,  0, "mov %i,r7" },
  { 0x90, 3, inst,  0, "mov dptr,%2" },
  { 0x91, 2, pushj, 0, "acall %p" },
  { 0x92, 2, inst,  0, "mov %b,c" },
  { 0x93, 1, inst,  0, "movc a,@a+dptr" },
  { 0x94, 2, inst,  0, "subb a,%1" },
  { 0x95, 2, inst,  0, "subb a,%i" },
  { 0x96, 1, inst,  0, "subb a,@r0" },
  { 0x97, 1, inst,  0, "subb a,@r1" },
  { 0x98, 1, inst,  0, "subb a,r0" },
  { 0x99, 1, inst,  0, "subb a,r1" },
  { 0x9a, 1, inst,  0, "subb a,r2" },
  { 0x9b, 1, inst,  0, "subb a,r3" },
  { 0x9c, 1, inst,  0, "subb a,r4" },
  { 0x9d, 1, inst,  0, "subb a,r5" },
  { 0x9e, 1, inst,  0, "subb a,r6" },
  { 0x9f, 1, inst,  0, "subb a,r7" },
  { 0xa0, 2, inst,  0, "orl c,/%b" },
  { 0xa1, 2, jrst,  0, "ajmp %p" },
  { 0xa2, 2, inst,  0, "mov c,%b" },
  { 0xa3, 1, inst,  0, "inc dptr" },
  { 0xa4, 1, inst,  0, "mul ab" },
  { 0xa5, 1, unused, 0, 0 },	/* DEC DPTR in winbond? */
  { 0xa6, 2, inst,  0, "mov @r0,%i" },
  { 0xa7, 2, inst,  0, "mov @r1,%i" },
  { 0xa8, 2, inst,  0, "mov r0,%i" },
  { 0xa9, 2, inst,  0, "mov r1,%i" },
  { 0xaa, 2, inst,  0, "mov r2,%i" },
  { 0xab, 2, inst,  0, "mov r3,%i" },
  { 0xac, 2, inst,  0, "mov r4,%i" },
  { 0xad, 2, inst,  0, "mov r5,%i" },
  { 0xae, 2, inst,  0, "mov r6,%i" },
  { 0xaf, 2, inst,  0, "mov r7,%i" },
  { 0xb0, 2, inst,  0, "anl c,/%b" },
  { 0xb1, 2, pushj, 0, "acall %p" },
  { 0xb2, 2, inst,  0, "cpl %b" },
  { 0xb3, 1, inst,  0, "cpl c" },
  { 0xb4, 3, pushj, 0, "cjne a,%1,%d" },
  { 0xb5, 3, pushj, 0, "cjne a,%i,%d" },
  { 0xb6, 3, pushj, 0, "cjne @r0,%1,%d" },
  { 0xb7, 3, pushj, 0, "cjne @r1,%1,%d" },
  { 0xb8, 3, pushj, 0, "cjne r0,%1,%d" },
  { 0xb9, 3, pushj, 0, "cjne r1,%1,%d" },
  { 0xba, 3, pushj, 0, "cjne r2,%1,%d" },
  { 0xbb, 3, pushj, 0, "cjne r3,%1,%d" },
  { 0xbc, 3, pushj, 0, "cjne r4,%1,%d" },
  { 0xbd, 3, pushj, 0, "cjne r5,%1,%d" },
  { 0xbe, 3, pushj, 0, "cjne r6,%1,%d" },
  { 0xbf, 3, pushj, 0, "cjne r7,%1,%d" },
  { 0xc0, 2, inst,  0, "push %i" },
  { 0xc1, 2, jrst,  0, "ajmp %p" },
  { 0xc2, 2, inst,  0, "clr %b" },
  { 0xc3, 1, inst,  0, "clr c" },
  { 0xc4, 1, inst,  0, "swap a" },
  { 0xc5, 2, inst,  0, "xch a,%i" },
  { 0xc6, 1, inst,  0, "xch a,@r0" },
  { 0xc7, 1, inst,  0, "xch a,@r1" },
  { 0xc8, 1, inst,  0, "xch a,r0" },
  { 0xc9, 1, inst,  0, "xch a,r1" },
  { 0xca, 1, inst,  0, "xch a,r2" },
  { 0xcb, 1, inst,  0, "xch a,r3" },
  { 0xcc, 1, inst,  0, "xch a,r4" },
  { 0xcd, 1, inst,  0, "xch a,r5" },
  { 0xce, 1, inst,  0, "xch a,r6" },
  { 0xcf, 1, inst,  0, "xch a,r7" },
  { 0xd0, 2, inst,  0, "pop %i" },
  { 0xd1, 2, pushj, 0, "acall %p" },
  { 0xd2, 2, inst,  0, "setb %b" },
  { 0xd3, 1, inst,  0, "setb c" },
  { 0xd4, 1, inst,  0, "da" },
  { 0xd5, 3, pushj, 0, "djnz %i,%d" },
  { 0xd6, 1, inst,  0, "xchd a,@r0" },
  { 0xd7, 1, inst,  0, "xchd a,@r1" },
  { 0xd8, 2, pushj, 0, "djnz r0,%d" },
  { 0xd9, 2, pushj, 0, "djnz r1,%d" },
  { 0xda, 2, pushj, 0, "djnz r2,%d" },
  { 0xdb, 2, pushj, 0, "djnz r3,%d" },
  { 0xdc, 2, pushj, 0, "djnz r4,%d" },
  { 0xdd, 2, pushj, 0, "djnz r5,%d" },
  { 0xde, 2, pushj, 0, "djnz r6,%d" },
  { 0xdf, 2, pushj, 0, "djnz r7,%d" },
  { 0xe0, 1, inst,  0, "movx a,@dptr" },
  { 0xe1, 2, jrst,  0, "ajmp %p" },
  { 0xe2, 1, inst,  0, "movx a,@r0" },
  { 0xe3, 1, inst,  0, "movx a,@r1" },
  { 0xe4, 1, inst,  0, "clr a" },
  { 0xe5, 2, inst,  0, "mov a,%i" },
  { 0xe6, 1, inst,  0, "mov a,@r0" },
  { 0xe7, 1, inst,  0, "mov a,@r1" },
  { 0xe8, 1, inst,  0, "mov a,r0" },
  { 0xe9, 1, inst,  0, "mov a,r1" },
  { 0xea, 1, inst,  0, "mov a,r2" },
  { 0xeb, 1, inst,  0, "mov a,r3" },
  { 0xec, 1, inst,  0, "mov a,r4" },
  { 0xed, 1, inst,  0, "mov a,r5" },
  { 0xee, 1, inst,  0, "mov a,r6" },
  { 0xef, 1, inst,  0, "mov a,r7" },
  { 0xf0, 1, inst,  0, "movx @dptr,a" },
  { 0xf1, 2, pushj, 0, "acall %p" },
  { 0xf2, 1, inst,  0, "movx @r0,a" },
  { 0xf3, 1, inst,  0, "movx @r1,a" },
  { 0xf4, 1, inst,  0, "cpl a" },
  { 0xf5, 2, inst,  0, "mov %i,a" },
  { 0xf6, 1, inst,  0, "mov @r0,a" },
  { 0xf7, 1, inst,  0, "mov @r1,a" },
  { 0xf8, 1, inst,  0, "mov r0,a" },
  { 0xf9, 1, inst,  0, "mov r1,a" },
  { 0xfa, 1, inst,  0, "mov r2,a" },
  { 0xfb, 1, inst,  0, "mov r3,a" },
  { 0xfc, 1, inst,  0, "mov r4,a" },
  { 0xfd, 1, inst,  0, "mov r5,a" },
  { 0xfe, 1, inst,  0, "mov r6,a" },
  { 0xff, 1, inst,  0, "mov r7,a" },
};

/*
** Known registers in upper half of i-space:
*/

static dispblock i8051regs[] = {
  { 0x80, 0, 0, 0, "p0" },	/* port 0 */
  { 0x81, 0, 0, 0, "sp" },	/* stack pointer */
  { 0x82, 0, 0, 0, "dpl" },	/* data pointer low */
  { 0x83, 0, 0, 0, "dph" },	/* data pointer high */
  { 0x87, 0, 0, 0, "pcon" },	/* power control */
  { 0x88, 0, 0, 0, "tcon" },	/* timer control */
  { 0x89, 0, 0, 0, "tmod" },	/* timer mode */
  { 0x8a, 0, 0, 0, "tl0" },	/* timer low 0 */
  { 0x8b, 0, 0, 0, "tl1" },	/* timer low 1 */
  { 0x8c, 0, 0, 0, "th0" },	/* timer high 0 */
  { 0x8d, 0, 0, 0, "th1" },	/* timer high 1 */
  { 0x90, 0, 0, 0, "p1" },	/* port 1 */
  { 0x98, 0, 0, 0, "scon" },	/* serial controller */
  { 0x99, 0, 0, 0, "sbuf" },	/* serial data buffer */
  { 0xa0, 0, 0, 0, "p2" },	/* port 2 */
  { 0xa8, 0, 0, 0, "ie" },	/* interrupt enable */
  { 0xb0, 0, 0, 0, "p3" },	/* port 3 */
  { 0xb8, 0, 0, 0, "ip" },	/* interrupt priority */
  { 0xd0, 0, 0, 0, "psw" },	/* program status word */
  { 0xe0, 0, 0, 0, "acc" },	/* accumulator */
  { 0xf0, 0, 0, 0, "b" },	/* b register */
  { 0, 0, arnold, 0, NULL },
};

/*
  Winbond extensions:

  84 -- dpl1
  85 -- dph1
  86 -- dps            ; data pointer select
  8e -- ckcon          ; clock control
  91 -- exif           ; external interrupt flag
  a5 -- p4
  a9 -- saddr          ; slave addr (port 0)
  aa -- addr1          ; slave addr (port 1)
  b9 -- saden          ; slave addr enable (port 0)
  ba -- saden1         ; slave addr enable (port 1)
  c0 -- scon1          ; serial port control (port 1)
  c1 -- sbuf1          ; serial data buffer (port 1)
  c2 -- rommap         ; huh?
  c4 -- pmr            ; power management register
  c5 -- status         ; status register
  c7 -- ta             ; timed access (for protected bits)
  c8 -- t2con          ; timer 2 control
  c9 -- t2mod          ; timer 2 mode control
  ca -- rcap2l         ; timer 2 capture lsb
  cb -- rcap2h         ; timer 2 capture msb
  cc -- tl2            ; timer 2 lsb
  cd -- th2            ; timer 2 msb
  d8 -- wdcon          ; watchdog control
  e8 -- eie            ; extended interrupt enable
  f8 -- eip            ; extended interrupt priority
*/

/*
** Known bits in upper half of i-space:
*/

static dispblock i8085bits[] = {
  { 0x88, 0, 0, 0, "it0" },
  { 0x89, 0, 0, 0, "ie0" },
  { 0x8a, 0, 0, 0, "it1" },
  { 0x8b, 0, 0, 0, "ie1" },
  { 0x8c, 0, 0, 0, "tr0" },
  { 0x8d, 0, 0, 0, "tf0" },
  { 0x8e, 0, 0, 0, "tr1" },
  { 0x8f, 0, 0, 0, "tf1" },
  { 0x90, 0, 0, 0, "t2" },
  { 0x91, 0, 0, 0, "t2ex" },
  { 0x98, 0, 0, 0, "ri" },
  { 0x90, 0, 0, 0, "ti" },
  { 0x9a, 0, 0, 0, "rb8" },
  { 0x9b, 0, 0, 0, "tb8" },
  { 0x9c, 0, 0, 0, "ren" },
  { 0x9d, 0, 0, 0, "sm2" },
  { 0x9e, 0, 0, 0, "sm1" },
  { 0x9f, 0, 0, 0, "sm0" },
  { 0xa8, 0, 0, 0, "ex0" },
  { 0xa9, 0, 0, 0, "et0" },
  { 0xaa, 0, 0, 0, "ex1" },
  { 0xab, 0, 0, 0, "et1" },
  { 0xac, 0, 0, 0, "es" },
  { 0xaf, 0, 0, 0, "ea" },
  { 0xb0, 0, 0, 0, "rxd" },
  { 0xb1, 0, 0, 0, "txd" },
  { 0xb2, 0, 0, 0, "int0" },
  { 0xb3, 0, 0, 0, "int1" },
  { 0xb4, 0, 0, 0, "t0" },
  { 0xb5, 0, 0, 0, "t1" },
  { 0xb6, 0, 0, 0, "wr" },
  { 0xb7, 0, 0, 0, "rd" },
  { 0xb8, 0, 0, 0, "px0" },
  { 0xb9, 0, 0, 0, "pt0" },
  { 0xba, 0, 0, 0, "px1" },
  { 0xbb, 0, 0, 0, "pt1" },
  { 0xbc, 0, 0, 0, "ps" },
  { 0xd0, 0, 0, 0, "p" },
  { 0xd2, 0, 0, 0, "ov" },
  { 0xd3, 0, 0, 0, "rs0" },
  { 0xd4, 0, 0, 0, "rs1" },
  { 0xd5, 0, 0, 0, "f0" },
  { 0xd6, 0, 0, 0, "ac" },
  { 0xd7, 0, 0, 0, "cy" },
  { 0, 0, arnold, 0, NULL },
};

/*
** Start of our local variables:
*/

static address* touch;		/* Where in memory this instruction refers */
static byte opcode;		/* Well, what do you think? */

/**********************************************************************/

static void number(word w)
{
  switch (radix) {
    //  case 2:
    //  case 8:
  case 10:
    bufdecimal(w, 1);
    break;
  case 16:
  default:
    bufhex(w, 0);
    if (w > 9) {
      casechar('h');
    }
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

static void i8051_dobyte(void)
{
  defb(getbyte());
}

static void i8051_dochar(void)
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

static void i8051_doword(void)
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

static void i8051_doptr(void)
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

static void i8051_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    i8051_dochar();
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
    i = w - a_a2l(istart);
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

static void refbit(byte b)
{
  dispblock* d;

  if (b >= 0x80) {
    d = finddisp(b, i8085bits, NULL);
    if (d != NULL) {
      bufstring(d->expand);
      return;
    }
    d = finddisp(b & 0xf8, i8051regs, NULL);
    if (d != NULL) {
      bufstring(d->expand);
      bufchar('.');
      bufnumber(b & 0x07);
      return;
    }
  }
  bufchar('#');
  number(b);
}

static void refiram(byte b)
{
  dispblock* d;

  d = finddisp(b, i8051regs, NULL);

  if (d != NULL) {
    bufstring(d->expand);
  } else {
    number(b);
  }
}

static void refpage(byte b)
{
  word w;

  w = (a_a2w(pc) & 0xf800) + ((opcode << 3) & 0x0700) + b;

  refaddr(w);
}

/*
** escape codes:
**
**  %1 - 8-bit inline constant.
**  %2 - 16-bit inline constant.
**  %a - 16-bit address.
**  %b - 8-bit bit number in bit space.
**  %d - 8-bit relative dispacement.
**  %i - 8-bit offset into i-ram.
**  %p - 11-bit abs. address, top 5 from pc.
*/

static void copytext(char* p)
{
  char c;

  while ((c = *(p++)) != (char) 0) {
    if (c == '%') {
      c = *(p++);
      switch (c) {
      case '1':			/* One byte of data. */
	bufchar('#');
	number(getbyte());
	break;
      case '2':			/* One word of data. */
	if (argstatus == st_ptr) {
	  refaddr(getword());
	} else {
	  bufchar('#');
	  number(getword());
	}
	break;
      case 'a':			/* Absolute address. */
	refaddr(getword());
	break;
      case 'b':			/* bit number. */
	refbit(getbyte());
	break;
      case 'd':			/* Relative address. */
	refdisp(getdisp());
	break;
      case 'i':			/* i-ram offset. */
	refiram(getbyte());
	break;
      case 'p':			/* short-format abs addr. */
	refpage(getbyte());
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

/*
** instruction decoding:
*/

static void i8051_doinst(void)
{
  dispblock* disp;

  opcode = getbyte();
  disp = &i8051disp[opcode];
  pb_length = disp->length;

  if (disp->itype == unused) {
    defb(opcode);
    return;
  }

  if (overrun()) {
    defb(opcode);
    return;
  }

  startline(true);

  copytext(disp->expand);

  switch (disp->itype) {
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

void i8051_begin(void)
{
  bufstring(";Beginning of program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

void i8051_org(address* a)
{
  bufblankline();
  casestring("org");
  spacedelim();
  bufstring(a_a2str(a));
  bufblankline();
}

void i8051_end(void)
{
  bufblankline();
  bufstring(";End of program");
}

char* i8051_lcan(char* name)
{
  static char work[10];

  return canonicalize(name, work, 6);
}

bool i8051_lchk(char* name)
{
  return checkstring(name, "", "0123456789_?");
}

void i8051_lgen(address* addr)
{
  genlabel(a_a2w(addr));
}

/*
** Return list of good starting points.
*/

address* i8051_auto(void)
{
  return a_zero();		/* Power-on starts here. */
}

/**********************************************************************/

/*
** i8051 -- Intel 8051 microcontroller and friends.
*/

void i8051_init(void)
{
  spf_lcan(i8051_lcan);
  spf_lchk(i8051_lchk);
  spf_lgen(i8051_lgen);
  spf_auto(i8051_auto);

  /* set up our object handlers: */
  
  spf_dodef(i8051_dobyte);

  spf_doobj(st_inst, i8051_doinst);
  spf_doobj(st_byte, i8051_dobyte);
  spf_doobj(st_word, i8051_doword);
  spf_doobj(st_ptr,  i8051_doptr);
  spf_doobj(st_char, i8051_dochar);
  spf_doobj(st_text, i8051_dotext);
  
  spf_begin(i8051_begin);
  spf_end(i8051_end);
  spf_org(i8051_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line, expanded. */
  pv_abits = 16;		/* Number of address bits. */
  pv_bigendian = true;
}

bool i8051_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for Intel 8051 processor.\n\
");
    return true;
  }
  return false;
}
