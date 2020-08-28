// ka780.c - VAX-11/780 System Configurations and Routines
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS-10 Emulator.
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

#include "vax/defs.h"

// VAX-11/780 Memory Map
//
//            +-----------------------------------------------------+
// 0000 0000: |                  Installed Memory                   |
//     :      |- - - - - - - - - - - - - - - - - - - - - - - - - - -|
// 1FFF FFFF: |                                                     |
//            +-----------------------------------------------------+   
// 2000 0000: |                  TR0 Adapter Space                  |
//            +-----------------------------------------------------+
// 2000 2000: |                  TR1 Adapter Space                  |
//            |                          :                          |
// 2001 E000: |                  TR15 Adapter Space                 |
//            +-----------------------------------------------------+
// 2002 0000: |                                                     |
//     :      |                      Reserved                       |
// 200F FFFF: |                                                     |
//            +-----------------------------------------------------+
// 2010 0000: |                UNIBUS 0 Address Space               |
//            +-----------------------------------------------------+
// 2014 0000: |                UNIBUS 1 Address Space               |
//            +-----------------------------------------------------+
// 2018 0000: |                UNIBUS 2 Address Space               |
//            +-----------------------------------------------------+
// 201C 0000: |                UNIBUS 3 Address Space               |
//            +-----------------------------------------------------+
// 2020 0000: |                                                     |
//     :      |                      Reserved                       |
// 3FFF FFFF: |                                                     |
//            +-----------------------------------------------------+

// System ID Longword
//
//  31   24 23 22          15 14   12 11             0
// +-------+--+--------------+-------+----------------+
// |   1   |  |   ECO Level  | Plant |  Serial Number |
// +-------+--+--------------+-------+----------------+
//          |
//          +- 0 = VAX-11/780
//             1 = VAX-11/785

#define KA780_ECO      0
#define KA780_PLANT    0
#define KA780_SN       1

#define KA780_SYSID    0x01000000
#define KA780_785      0x00800000

#define KA780_P_ECO    15
#define KA780_P_PLANT  12

// Privileged Register for VAX-11/780 system
#define nKSP    0x00 // Kernel Stack Pointer
#define nESP    0x01 // Executive Stack Pointer
#define nSSP    0x02 // Supervisor Stack Pointer
#define nUSP    0x03 // User Stack Pointer
#define nISP    0x04 // Interrupt Stack Pointer

#define nP0BR   0x08 // P0 Base Register
#define nP0LR   0x09 // P0 Length Register
#define nP1BR   0x0A // P1 Base Register
#define nP1LR   0x0B // P1 Length Register
#define nSBR    0x0C // System Base Register
#define nSLR    0x0D // System Length Register

#define nPCBB   0x10 // Process Control Block Base
#define nSCBB   0x11 // System Control Block Base
#define nIPL    0x12 // Interrupt Priority Level
#define nASTLVL 0x13 // AST Level
#define nSIRR   0x14 // Software Interrupt Request
#define nSISR   0x15 // Software Interrupt Summary

#define nRXCS   0x20 // Console Receive Control and Status Register
#define nRXDB   0x21 // Console Receive Data Buffer Register
#define nTXCS   0x22 // Console Transmit Control and Status Register
#define nTXDB   0x23 // Console Transmit Data Buffer Register
#define nMAPEN  0x38 // Map Enable
#define nSID    0x3E // System Identification

#define SCBB_MBZ   0x000001FF // Must be zeros.
#define BR_MASK    0xFFFFFFFC // xxBR Mask
#define LR_MASK    0x001FFFFF // xxLR Mask

// Console Terminal Registers

// RXCS - Console Receive Control and Status Register
#define RXCS_MBZ   0xFFFFFF3F // Must be zeros.
#define RXCS_RDY   0x00000080 // (R)   Ready
#define RXCS_IE    0x00000040 // (R/W) Interrupt Enable

// RXDB - Console Receive Data Buffer Register
#define RXDB_ERROR 0x00008000 // (R)   Error
#define RXDB_ID    0x00000F00 // (R)   Identification
#define RXDB_DATA  0x000000FF // (R)   Data Received

// TXCS - Console Terminal Control and Status Register
#define TXCS_MBZ   0xFFFFFF3F // Must be zeros.
#define TXCS_RDY   0x00000080 // (R)   Ready
#define TXCS_IE    0x00000040 // (R/W) Interrupt Enable

// TXDB - Console Terminal Data Buffer Register
#define TXDB_MBZ   0xFFFFF000 // Must be zeros
#define TXDB_ID    0x00000F00 // (R/W) Identification
#define TXDB_DATA  0x000000FF // (R/W) Data Transmitted


int32 ka780_prRead(int32 pReg)
{
	int32 data;

	switch (pReg) {
		case nRXDB:
			RXCS &= ~RXCS_RDY;
			data = RXDB;
			break;

		default:
			data = PRN(pReg);
			break;

/*
#ifdef DEBUG
			if (dbg_Check(DBG_TRACE|DBG_DATA)) {
				dbg_Printf("KA780: (R) Unknown privileged register: %08X (%d)\n",
					pReg, pReg);
			}
#endif DEBUG
			RSRVD_OPND_FAULT;
*/
	}

#ifdef DEBUG
//	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("KA780: (R) Register %02X => %08X\n", pReg, data);
#endif DEBUG

	return data;
}

void ka780_prWrite(int32 pReg, int32 data)
{
	switch (pReg) {
		case nKSP:
		case nESP:
		case nSSP:
		case nUSP:
		case nISP:
			PRN(pReg) = data;
			break;

		case nP0BR:
		case nP1BR:
		case nSBR:
			PRN(pReg) = data & BR_MASK;
			break;

		case nP0LR:
		case nP1LR:
		case nSLR:
			PRN(pReg) = data & LR_MASK;
			break;
		
		case nPCBB:
			PCBB = data & LALIGN;
			break;

		case nSCBB:
			if (data & SCBB_MBZ)
				RSRVD_OPND_FAULT;
			SCBB = data;
			break;

		case nIPL:
			PSB.ipl = data;
			IPL = data;
			break;

		case nRXCS:
			if (data & RXCS_MBZ)
				RSRVD_OPND_FAULT;
			RXCS = data;
			break;

		case nRXDB:
			break;

		case nTXCS:
			if (data & TXCS_MBZ)
				RSRVD_OPND_FAULT;
			TXCS = data;
			break;

		case nTXDB:
			putchar(data & 0x7F);
			TXDB = data & ~TXDB_MBZ;
			TXCS |= TXCS_RDY;
			break;

		case nMAPEN:
			MAPEN = data & 1;
			break;

		default:
#ifdef DEBUG
			if (dbg_Check(DBG_TRACE|DBG_DATA)) {
				dbg_Printf("KA780: (W) Unknown privileged register: %08X (%d)\n",
					pReg, pReg);
			}
#endif DEBUG
			RSRVD_OPND_FAULT;
	}

#ifdef DEBUG
//	if (dbg_Check(DBG_TRACE|DBG_DATA))
		dbg_Printf("KA780: (W) Register %02X <= %08X\n", pReg, data);
#endif DEBUG
}

void ka780_Initialize(void)
{
	TXCS = TXCS_RDY;
	SID  = KA780_SYSID;
}
