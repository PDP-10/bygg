// cpu_kspi.c - KS10 Processor: Priority Interrupt System routines
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS10 Emulator
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

#include "pdp10/defs.h"
#include "pdp10/proto.h"

static int pi_On;      // System On/Off
static int pi_Enables; // Levels - Enables (Levels On/Off)
static int pi_Actives; // Levels - In Progress
static int pi_ioReqs;  // Levels - I/O Interrupt Requests
static int pi_aprReqs; // Levels - APR Interrupt Requests
static int pi_pgmReqs; // Levels - Program Requests

// Select highest priority number from levels
static int pi_Highest[128] = {
	0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static int pi_Mask[8] = {
	PI_M_INT0, // Interrupt Level #0 - No Interrupts
	PI_M_INT1, // Interrupt Level #1 - Highest priority
	PI_M_INT2, // Interrupt Level #2
	PI_M_INT3, // Interrupt Level #3
	PI_M_INT4, // Interrupt Level #4
	PI_M_INT5, // Interrupt Level #5
	PI_M_INT6, // Interrupt Level #6
	PI_M_INT7, // Interrupt Level #7 - Lowest priority
};

// Initialize/Reset priority interrupt system
void p10_ResetPI(void)
{
	pi_On      = 0;
	pi_Enables = 0;
	pi_Actives = 0;
	pi_aprReqs = 0;
	pi_ioReqs  = 0;
	pi_pgmReqs = 0;
}

// 70060 - WRPI Instruction
void p10_ksOpcode_WRPI(void)
{
	if (eAddr & PI_M_CLR_PI_SYS) {
		pi_On      = 0;
		pi_Enables = 0;
		pi_Actives = 0;
		pi_pgmReqs = 0;
	}
	// ACTION: Need implement UUO if all bits are not on.
	if (eAddr & PI_M_PI_SYS_OFF)
		pi_On = 0;
	if (eAddr & PI_M_PI_SYS_ON)
		pi_On = 1;
	if (eAddr & PI_M_LEVELS_ON)
		pi_Enables |= (eAddr & PI_M_LEVELS);
	if (eAddr & PI_M_LEVELS_OFF)
		pi_Enables &= ~(eAddr & PI_M_LEVELS);
	if (eAddr & PI_M_INI_INT_ON)
		pi_pgmReqs |= (eAddr & PI_M_LEVELS);
	if (eAddr & PI_M_DROP_PROG_REQ)
		pi_pgmReqs &= ~(eAddr & PI_M_LEVELS);

	p10_piEvaluate();

#ifdef DEBUG
	if (dbg_Check(DBG_INTERRUPT))
		dbg_Printf("PI: System: %s Levels: %03o Requests: %03o In Progress: %03o\n",
			(pi_On ? "On" : "Off"), pi_Enables, pi_pgmReqs, pi_Actives);
#endif DEBUG
}

// 70064 - RDPI Instruction
void p10_ksOpcode_RDPI(void)
{
	int36 pisr = (pi_pgmReqs << PISR_P_PI_REQUEST) |
		(pi_Actives << PISR_P_PI_HOLD) | pi_Enables |
		(pi_On ? PISR_M_PI_SYS_ON : 0);

	p10_vWrite(eAddr, pisr, NOPXCT);
}

// CONSO PI, Instruction
void p10_ksOpcode_COPI(void)
{
	int18 pisr = (pi_Actives << PISR_P_PI_HOLD) |
		(pi_On ? PISR_M_PI_SYS_ON : 0) | pi_Enables;

	if (eAddr & pisr)
		DO_SKIP;
}

// CONSZ PI, Instruction
void p10_ksOpcode_CZPI(void)
{

	int18 pisr = (pi_Actives << PISR_P_PI_HOLD) |
		(pi_On ? PISR_M_PI_SYS_ON : 0) | pi_Enables;

	if ((eAddr & pisr) == 0)
		DO_SKIP;
}

// Evaluate priority interrupts
inline void p10_piEvaluate(void)
{
	int actlvl, reqlvl;

	if (pi_On) {
		pi_aprReqs = (apr_Flags & apr_Enables) ? pi_Mask[apr_Level] : 0;
//		reqlvl = pi_Highest[((pi_aprReqs | pi_ioReqs) & pi_Enables) | pi_pgmReqs];
		reqlvl = pi_Highest[(pi_aprReqs | pi_ioReqs | pi_pgmReqs) & pi_Enables];
		actlvl = pi_Highest[pi_Actives];
		if ((actlvl == 0) || (reqlvl < actlvl)) {
			cpu_pInterrupt = reqlvl;
			return;
		}
	}
	cpu_pInterrupt = 0;
}

inline void p10_piRequestIO(int pi)
{
	pi_ioReqs |= pi_Mask[pi];

#ifdef DEBUG
	if (dbg_Check(DBG_INTERRUPT))
		dbg_Printf("PI: Channel %d had been requested for I/O.\n", pi);
#endif DEBUG
}

inline void p10_piRequestAPR(int pi)
{
	pi_aprReqs |= pi_Mask[pi];

#ifdef DEBUG
	if (dbg_Check(DBG_INTERRUPT))
		dbg_Printf("PI: Channel %d had been requested for APR.\n", pi);
#endif DEBUG
}

// Dismiss the highest priority channel.
inline void p10_piDismiss(void)
{
#ifdef DEBUG
	if (dbg_Check(DBG_INTERRUPT))
		dbg_Printf("PI: Channel %d had been dismissed.\n",
			pi_Highest[pi_Actives]);
#endif DEBUG

	// Clear the highest priority level bit.
	pi_Actives &= ~(pi_Mask[pi_Highest[pi_Actives]]);
	p10_piEvaluate();
}

void p10_piProcess(void)
{
	int32 device; // UBA Controller #
	int32 vector; // PDP-11 Style Vector Interrupt
	int   i;

#ifdef DEBUG
	if (dbg_Check(DBG_INTERRUPT))
		dbg_Printf("PI: Levels: %03o Requests: %03o In Progress: %03o\n",
			pi_Enables, (pi_aprReqs | pi_ioReqs | pi_pgmReqs), pi_Actives);
#endif DEBUG

	// Check I/O Controllers first
	if (device = ks10uba_CheckInterrupt(&vector, cpu_pInterrupt)) {
		pi_ioReqs &= ~(pi_Mask[pi_Highest[pi_ioReqs]]);

		// Start to execute vector interrupt routines
#ifdef DEBUG
		if (dbg_Check(DBG_INTERRUPT))
			dbg_Printf("PI: UBA = %d Vector = %03o\n", device, vector);
#endif DEBUG
		AR = p10_pRead(eptAddr + EPT_UBA_BASE + device, 0);
		AR += (vector % 0400) >> 2;
		ARX = p10_eRead(AR);
	} else {
		// Start to execute interrupt routines
		AR  = eptAddr + EPT_PI_BASE + (cpu_pInterrupt << 1);
		ARX = p10_pRead(AR, 0);
	}

	if (LH(ARX) == 0254340000000LL) {
		// XPCW Instruction

#ifdef DEBUG
		if (dbg_Check(DBG_INTERRUPT|DBG_DATA)) {
			dbg_Printf("PI: XPCW Instruction\n");
			dbg_Printf("XPCW: Save Flags %06llo PC %06llo\n",
				LHSR(FLAGS), PC);
		}
#endif DEBUG

		ARX = RH(ARX);

		// Save flags and PC to executive memory.
		p10_eWrite(ARX++, FLAGS);
		p10_eWrite(ARX++, RH(PC));
		BR  = p10_eRead(ARX++);
		BRX = p10_eRead(ARX);
		
		// Now load new flags and PC. Update user mode.
		FLAGS = BR;
		DO_JUMP(BRX);

#ifdef DEBUG
		if (dbg_Check(DBG_INTERRUPT|DBG_DATA)) {
			dbg_Printf("XPCW: New Flags %06llo PC %06llo\n",
				LHSR(FLAGS), PC);
		}
#endif DEBUG

#ifdef DEBUG
		// Do not trace during interrupts execpt both DBG_TRACE and
		// DBG_INTERRUPT are enabled.
		if (!dbg_Check(DBG_INTERRUPT|DBG_TRACE)) {
			if (savedMode = dbg_GetMode()) {
				savedMode = ~savedMode;
				dbg_ClearMode(DBG_TRACE|DBG_DATA);
			}
		}
#endif DEBUG
	} else if (LH(ARX) == 0264000000000LL) {
		// JSR Instruction
//		fprintf(debug, "PI: JSR Instruction\n");
		ARX   = RH(ARX);
		p10_eWrite(ARX++, FLAGS | RH(PC));
		FLAGS = 0;
		DO_JUMP(ARX);
	} else {
		// Illegal Interrupt Instruction
		printf("PI: Illegal Interrupt Instruction at PC %06llo\r\n", RH(AR));
		printf("PI: Interrupt Instruction Code: %06llo,,%06llo\r\n",
			LHSR(ARX), RH(ARX));
		T1 = 0101; // Illegal Interrupt Instruction Code
		p10_SetHaltStatus();
		emu_State = EMU_HALT;
		return;
	}

	// Update active levels and do its evaluation.
	pi_Actives |= pi_Mask[cpu_pInterrupt];
	p10_piEvaluate();

	pager_PC = PC;

#ifdef DEBUG
	if (dbg_Check(DBG_INTERRUPT))
		dbg_Printf("PI: Channel %d now is in progress (active).\n",
			pi_Highest[pi_Actives]);
#endif DEBUG
}
