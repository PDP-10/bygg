// defs.h - definitions for VAX Processor
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of TS-10 Emulator.
// See README for copyright notice.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "emu/defs.h"

// Phsyical Memory defintions
#define PASIZE   30
#define PAMASK   ((1 << PASIZE) - 1)

// Byte macro definitions
#define BMASK    0x000000FF
#define BSIGN    0x00000080

#define WMASK    0x0000FFFF
#define WSIGN    0x00008000
#define WALIGN   0xFFFFFFFE

#define LMASK    0xFFFFFFFF
#define LSIGN    0x80000000
#define LMAX     0x7FFFFFFF
#define LMIN     0x80000000
#define LALIGN   0xFFFFFFFC

#define REGMASK  0xF

// Signed Extenstion defintions
#define SXTB(x)  (((x) & BSIGN) ? ((x) | ~BMASK) : (x))
#define SXTW(x)  (((x) & WSIGN) ? ((x) | ~WMASK) : (x))

// Result code definitions
#define VAX_OK       EMU_OK
#define VAX_NXM      EMU_NXM
#define VAX_RUN      EMU_RUN
#define VAX_HALT     EMU_HALT

#define MAX_OPREGS   8 //   8 Operand Registers
#define MAX_GREGS   16 //  16 General Registers
#define MAX_PREGS  128 // 128 Processor (Privileges) Registers

// Operand type definitions
#define OP_BYTE    1 // Operand is a byte      (1 byte)
#define OP_WORD    2 // Operand is a word      (2 bytes)
#define OP_LONG    4 // Operand is a longword  (4 bytes)
#define OP_QUAD    8 // Operand is a quadword  (8 bytes)
#define OP_OCTA   16 // Operand is a octaword  (16 bytes)

#define OP_SCALE    0x00FF // Scale Mask
#define OP_WRITE    0x0100 // Operand is writable (otherwise - readable)
#define OP_MODIFIED 0x0200 // Operand is modifiable
#define OP_ADDR     0x0800 // Operand is address
#define OP_VADDR    0x1000 // Operand is variable-length address
#define OP_BRANCH   0x2000 // Operand is a branch displacement
#define OP_FLOAT    0x4000 // Operand is a float (otherwise - integer)

// Read-Only Integer Operands
#define RB    (OP_BYTE)
#define RW    (OP_WORD)
#define RL    (OP_LONG)
#define RQ    (OP_QUAD)
#define RO    (OP_OCTA)

// Read/Write Integer Operands
#define MB    (OP_MODIFIED|OP_BYTE)
#define MW    (OP_MODIFIED|OP_WORD)
#define ML    (OP_MODIFIED|OP_LONG)
#define MQ    (OP_MODIFIED|OP_QUAD)
#define MO    (OP_MODIFIED|OP_OCTA)

// Write-Only Integer Operands
#define WB    (OP_WRITE|OP_BYTE)
#define WW    (OP_WRITE|OP_WORD)
#define WL    (OP_WRITE|OP_LONG)
#define WQ    (OP_WRITE|OP_QUAD)
#define WO    (OP_WRITE|OP_OCTA)

// Read-Only Floating Operands
#define RF    (OP_FLOAT|OP_LONG)
#define RG    (OP_FLOAT|OP_QUAD)
#define RD    (OP_FLOAT|OP_QUAD)
#define RH    (OP_FLOAT|OP_OCTA)

// Read/Write Floating Operands
#define MF    (OP_FLOAT|OP_MODIFIED|OP_LONG)
#define MG    (OP_FLOAT|OP_MODIFIED|OP_QUAD)
#define MD    (OP_FLOAT|OP_MODIFIED|OP_QUAD)
#define MH    (OP_FLOAT|OP_MODIFIED|OP_OCTA)

// Write-Only Floating Operands
#define WF    (OP_FLOAT|OP_WRITE|OP_LONG)
#define WG    (OP_FLOAT|OP_WRITE|OP_QUAD)
#define WD    (OP_FLOAT|OP_WRITE|OP_QUAD)
#define WH    (OP_FLOAT|OP_WRITE|OP_OCTA)

// Address Operands
#define AB    (OP_ADDR|OP_BYTE)
#define AW    (OP_ADDR|OP_WORD)
#define AL    (OP_ADDR|OP_LONG)
#define AQ    (OP_ADDR|OP_QUAD)
#define AO    (OP_ADDR|OP_OCTA)
#define AF    (OP_ADDR|OP_FLOAT|OP_LONG)
#define AG    (OP_ADDR|OP_FLOAT|OP_QUAD)
#define AD    (OP_ADDR|OP_FLOAT|OP_QUAD)
#define AH    (OP_ADDR|OP_FLOAT|OP_OCTA)

// Branch/Misc Operands
#define BB    (OP_BRANCH|OP_BYTE)
#define BW    (OP_BRANCH|OP_WORD)
#define VB    (OP_VADDR|OP_BYTE)

// Operand mode definitions
#define OP_MMASK     0xF0
#define OP_RMASK     0x0F
#define OP_MEM       -1

#define LIT0  0x00 // Short Literal
#define LIT1  0x10 
#define LIT2  0x20 
#define LIT3  0x30 
#define IDX   0x40 // Indexed
#define REG   0x50 // Register
#define REGD  0x60 // Register Deferred
#define ADEC  0x70 // Autodecrement
#define AINC  0x80 // Autoincrement
#define AINCD 0x90 // Autoincrement Deferred
#define BDP   0xA0 // Byte Displacement
#define BDPD  0xB0 // Byte Displacement Deferred
#define WDP   0xC0 // Word Displacement
#define WDPD  0xD0 // Word Displacement Deferred
#define LDP   0xE0 // Longword Displacement
#define LDPD  0xF0 // Longword Displacement Deferred

// Access Mode Definitions for PSL, page tables, etc..
#define AM_KERNEL     0 // Kernel Mode
#define AM_EXECUTIVE  1 // Executive Mode
#define AM_SUPERVISOR 2 // Supervisor Mode
#define AM_USER       3 // User Mode
#define AM_INTERRUPT  4 // Interrupt Mode

// System Control Block Vectors
#define SCB_PASSIVE    0x00  // Passive Release
#define SCB_CHECK      0x04  // Machine Check
#define SCB_KSNV       0x08  // Kernel Stack Not Valid
#define SCB_POWER      0x0C  // Power Fail
#define SCB_RESIN      0x10  // Reserved or Privileged Instruction
#define SCB_XFC        0x14  // Customer Reserved Instruction (XFC)
#define SCB_RESOP      0x18  // Reserved Operand
#define SCB_RESAD      0x1C  // Reserved Address Mode
#define SCB_ACCVIO     0x20  // Access-Control Violation
#define SCB_TNV        0x24  // Translation Not Valid
#define SCB_TP         0x28  // Trace Pending
#define SCB_BPT        0x2C  // Breakpoint Instruction
#define SCB_COMPAT     0x30  // Compatilbility
#define SCB_ARITH      0x34  // Arithmetic
#define SCB_CHMK       0x40  // Change Mode to Kernel
#define SCB_CHME       0x44  // Change Mode to Executive
#define SCB_CHMS       0x48  // Change Mode to Supervisor
#define SCB_CHMU       0x4C  // Change Mode to User
#define SCB_SOFTWARE1  0x84  // Software Level 1
#define SCB_SOFTWARE2  0x88  // Software Level 2
#define SCB_SOFTWARE3  0x8C  // Software Level 3
#define SCB_SOFTWARE4  0x90  // Software Level 4
#define SCB_SOFTWARE5  0x94  // Software Level 5
#define SCB_SOFTWARE6  0x98  // Software Level 6
#define SCB_SOFTWARE7  0x9C  // Software Level 7
#define SCB_SOFTWARE8  0xA0  // Software Level 8
#define SCB_SOFTWARE9  0xA4  // Software Level 9
#define SCB_SOFTWARE10 0xA8  // Software Level A
#define SCB_SOFTWARE11 0xAC  // Software Level B
#define SCB_SOFTWARE12 0xB0  // Software Level C
#define SCB_SOFTWARE13 0xB4  // Software Level D
#define SCB_SOFTWARE14 0xB8  // Software Level E
#define SCB_SOFTWARE15 0xBC  // Software Level F
#define SCB_TIMERVAL   0xC0  // Interval Timer
#define SCB_EMULATE    0xC8  // Subset Emulation
#define SCB_EMULFPD    0xCC  // Subset Emulation with FPD flag
#define SCB_CSREAD     0xF0  // Console Storage Read
#define SCB_CSWRITE    0xF4  // Console Storage Write
#define SCB_CTREAD     0xF8  // Console Terminal Read
#define SCB_CTWRITE    0xFC  // COnsole Terminal Write

// Arithmetic Exception Type Codes
#define TRAP_INTOVF   1 // Integer Overflow Trap
#define TRAP_INTDIV   2 // Integer Divide-By-Zero Trap
#define TRAP_FLTOVF   3 // Floating Overflow Trap
#define TRAP_FLTDIV   4 // Floating or Decimal Divide-By-Zero Trap
#define TRAP_FLTUND   5 // Floating Underflow Trap
#define TRAP_DECOVF   6 // Decimal Overflow Trap
#define TRAP_SUBRNG   7 // Subscript Range Trap
#define FAULT_FLTOVF  8 // Floating Overflow Fault
#define FAULT_FLTDIV  9 // Floating Divide-By-Zero Fault
#define FAULT_FLTUND 10 // Floating Underflow Fault

// General Registers Definition for VAX Processor

#define RN(n)  vax->gRegs[n]

#define RN0(n) RN(n)
#define RN1(n) RN((n+1) & REGMASK)

#define R0  RN(0)  // General Register #0
#define R1  RN(1)  // General Register #1
#define R2  RN(2)  // General Register #2
#define R3  RN(3)  // General Register #3
#define R4  RN(4)  // General Register #4
#define R5  RN(5)  // General Register #5
#define R6  RN(6)  // General Register #6
#define R7  RN(7)  // General Register #7
#define R8  RN(8)  // General Register #8
#define R9  RN(9)  // General Register #9
#define R10 RN(10) // General Register #10
#define R11 RN(11) // General Register #11
#define AP  RN(12) // Argument Pointer Register (R12)
#define FP  RN(13) // Frame Pointer Register    (R13)
#define SP  RN(14) // Stack Pointer Register    (R14)
#define PC  RN(15) // Program Counter Register  (R15)
#define nPC 15

// Operand Registers for up to 8 operands

#define OPN(n) vax->opRegs[n]

#define OP0 OPN(0) // Operand Register #0
#define OP1 OPN(1) // Operand Register #1
#define OP2 OPN(2) // Operand Register #2
#define OP3 OPN(3) // Operand Register #3
#define OP4 OPN(4) // Operand Register #4
#define OP5 OPN(5) // Operand Register #5
#define OP6 OPN(6) // Operand Register #6
#define OP7 OPN(7) // Operand Register #7

// Processor (Privileged) Registers Definition for VAX Processor
#define PRN(n) vax->pRegs[n]   // Processor Register #n

#define KSP    PRN(0)   // (R/W) Kernel Stack Pointer
#define ESP    PRN(1)   // (R/W) Executive Stack Pointer
#define SSP    PRN(2)   // (R/W) Supervisor Stack Pointer
#define USP    PRN(3)   // (R/W) User Stack Pointer
#define ISP    PRN(4)   // (R/W) Interrupt Stack Pointer

#define P0BR   PRN(8)   // (R/W) P0 Base Register
#define POLR   PRN(9)   // (R/W) P0 Length Register
#define P1BR   PRN(10)  // (R/W) P1 Base Register
#define P1LR   PRN(11)  // (R/W) P1 Length Register
#define SBR    PRN(12)  // (R/W) System Base Register
#define SLR    PRN(13)  // (R/W) System Limit Register

#define PCBB   PRN(16)  // (R/W) Process Control Block Base
#define SCBB   PRN(17)  // (R/W) System Control Block Base
#define IPL    PRN(18)  // (R/W) Interrupt Priority Level
#define ASTLVL PRN(19)  // (R/W) AST Level
#define SIRR   PRN(20)  // (W)   Software Interrupt Request Register
#define SISR   PRN(21)  // (R/W) Software Interrupt Summary Register

#define ICCS   PRN(24)  // (R/W) Internal Clock Control Status
#define NICR   PRN(25)  // (W)   Next Interval Count Register
#define ICR    PRN(26)  // (R)   Interval Count Register
#define TODR   PRN(27)  // (R/W) Time of Year Register

#define RXCS   PRN(32)  // (R/W) Console Receiver Status
#define RXDB   PRN(33)  // (R)   Console Receiver Data Buffer
#define TXCS   PRN(34)  // (R/W) Console Transmit Status
#define TXDB   PRN(35)  // (W)   Console Transmit Data Buffer

#define MAPEN  PRN(56)  // (R/W) Map Enable
#define TBIA   PRN(57)  // (W)   Translation Buffer Invalidate All
#define TBIS   PRN(58)  // (W)   Trabslation Buffer Invalidate Single

#define PME    PRN(61)  // (R/W) Performance Monitor Enable
#define SID    PRN(62)  // (R)   System Identification
#define TBCHK  PRN(63)  // (W)   Translation Buffer Check

// Processor Status Register Definitions
//
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// |CM |TP | 0 | 0 |FPD|IS |CUR_MOD|PRV_MOD| 0 |        IPL        |
// +---+---+---+---^---+---+---+---^---+---+---+---^---+---+---+---+
//   3   3   2   2   2   2   2   2   2   2   2   2   1   1   1   1
//   1   0   9   8   7   6   5   4   3   2   1   0   9   8   7   6
//
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |DV |FU |IV | T | N | Z | V | C |
// +---+---+---+---^---+---+---+---^---+---+---+---^---+---+---+---+
//   1   1   1   1   1   1   0   0   0   0   0   0   0   0   0   0
//   5   4   3   2   1   0   9   8   7   6   5   4   3   2   1   0

// Processor Status macro definitions
#define PSL vax->pStatus.Register
#define PSW vax->pStatus.Register
#define PSB vax->pStatus.Bit

// Processor Status Long Register (32-bit word)
#define PSL_MBZ  0x3020FF00 // Must Be Zeros
#define PSL_CM   0x80000000 // Compatibility Mode (PDP-11 Mode)
#define PSL_TP   0x40000000 // Trace Pending
#define PSL_FPD  0x08000000 // First Part Done
#define PSL_IS   0x04000000 // Interrupt Stack
#define PSL_CUR  0x03000000 // Current Mode
#define PSL_PRV  0x00C00000 // Previous Mode
#define PSL_IPL  0x001F0000 // Interrupt Priority Level

// Processor Status Word Register (16-bit word)
#define PSW_MASK 0xFFFF // PSW Mask
#define PSW_MBZ  0xFF00 // Must Be Zeros
#define PSW_DV   0x0080 // Decimal Overflow Enable
#define PSW_FU   0x0040 // Floating Underflow Enable
#define PSW_IV   0x0020 // Integer Overflow Enable
#define PSW_T    0x0010 // Trace Enable
#define PSW_CC   0x000F // Condition Codes
#define PSW_N    0x0008 // Negative Result
#define PSW_Z    0x0004 // Zero Result
#define PSW_V    0x0002 // Overflow Result
#define PSW_C    0x0001 // Carry Bit Result

typedef struct {
	uint c:1;     // Carry Bit Result
	uint v:1;     // Overflow Result
	uint z:1;     // Zero Result
	uint n:1;     // Negative Result
	uint t:1;     // Trace Enable
	uint iv:1;    // Integer Overflow Enable
	uint fu:1;    // Floating Underflow Enable
	uint dv:1;    // Decimal Overflow Enable
	uint mbz1:8;  // Must Be Zeros

	uint ipl:5;   // Interrupt Priority Level
	uint mbz2:1;  // Must Be Zeros
	uint pmod:2;  // Pervious Mode
	uint cmod:2;  // Current Mode
	uint is:1;    // Interrupt Stack
	uint fpd:1;   // First Part Done
	uint mbz3:2;  // Must Be Zeros
	uint tp:1;    // Trace Pending
	uint cm:1;    // Compatibility Mode (PDP-11 Mode)
} PSLBITS;

// Instruction Data Structure Definitions
// Instruction Table for Assembler, Disassembler, and Execution

typedef struct {
	char   *Name;            // Name of the Instruction
	char   *Desc;            // Description of the Instruction
	uint8  Extended;         // MSB of Instruction (Normally Zero)
	uint8  Opcode;           // Opcode Value
	uint8  nOperands;        // Number of Operands
	uint16 opMode[6];        // Attributes/Scales for Each Operand
	int    useCount;         // Instruction Profile Data
	void   (*Execute)(void); // Execute Routine
} INSTRUCTION;

typedef struct {
	union {
		PSLBITS Bit;
		uint32  Register;
	} pStatus;

	int32 opRegs[MAX_OPREGS]; // Operand Registers
	int32 gRegs[MAX_GREGS];   // General Registers
	int32 pRegs[MAX_PREGS];   // Processor (Privileged) Registers

	// RAM (Physical Memory) for VAX series
	uint8 *RAM;       // Address of physical memory
	int32 baseRAM;    // Starting address of physical memory
	int32 endRAM;     // Ending address of physical memory
	int32 sizeRAM;    // Size of physical memory

	// ROM Image for MicroVAX series
	uint8 *ROM;
	int32 baseROM;       // Starting Address of ROM
	int32 endROM;        // Ending Address of ROM
	int32 sizeROM;       // Size of ROM Area
	int32 maskROM;       // ROM Address Mask

	// NVRAM Image for MicroVAX series
	uint8 *NVRAM;        // Address of physical memory
	int32 baseNVRAM;     // Beginning of NVRAM Area
	int32 endNVRAM;      // End of NVRAM Area
	int32 sizeNVRAM;     // Size of NVRAM Area

	// Debug Facility Area
	int   ips;               // Instructions Per Second Meter
} VAXUNIT;

#define FAULT(x) vax_Abort(-(x))

#define RSRVD_INST_FAULT
#define RSRVD_ADDR_FAULT
#define RSRVD_OPND_FAULT
#define PRIV_INST_FAULT
#define XFC_FAULT
#define SET_TRAP(trap)

// Procedure/Function prototype definitions
#include "vax/extern.h"
#include "vax/proto.h"
