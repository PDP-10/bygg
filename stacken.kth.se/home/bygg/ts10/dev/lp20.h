// lp20.h - LP20/LP05/LP14 - Line Printer Emulation
//
// Written by
//  Timothy Stark <sword7@speakeasy.org>
//
// This file is part of the TS10 Emulator.
// See ReadMe for copyright notice.
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

// Unibus Address Index
#define LPCSRA (000 >> 1) // (R/W) Control and Status A      (CSRA)
#define LPCSRB (002 >> 1) // (R/W) Control and Status B      (CSRB)
#define LPBSAD (004 >> 1) // (R/W) DMA Bus Address Register  (BSAD)
#define LPBCTR (006 >> 1) // (R/W) DMA Byte Count Register   (BCTR)
#define LPPCTR (010 >> 1) // (R/W) Page Count Register       (PCTR)
#define LPRAMD (012 >> 1) // (R/W) RAM Data Register         (RAMD)
#define LPCCTR (014 >> 1) // (R/W) Column Count Register     (CCTR)
#define LPCBUF (014 >> 1) // (R/W) Character Buffer Register (CBUF)
#define LPCKSM (016 >> 1) // (R)   Checksum Register         (CKSM)
#define LPTDAT (016 >> 1) // (R)   Printer Data Register     (TDAT)

// Unibus Address 775400
// LPCSRA - Control and Status Register #A
#define LPCSRA_ERR     0100000 // (R)   Error  ("OR" of all errors)
#define LPCSRA_PAG0    0040000 // (R)   Page Counter Incremented to Zero
#define LPCSRA_ILCHR   0020000 // (R)   Illegal/Undefined Character
#define LPCSRA_VFURY   0010000 // (R)   DAVFU is loaded and ready
#define LPCSRA_ONLIN   0004000 // (R)   Printer is ready and online
#define LPCSRA_DEL     0002000 // (R/W) Last Character Received - Delimiter
#define LPCSRA_INIT    0001000 // (W)   Local Initialization (Unibus Init)
#define LPCSRA_RESET   0000400 // (W)   Reset Errors, Set Done, Reset Go
#define LPCSRA_DONE    0000200 //       Done
#define LPCSRA_INTEN   0000100 // (R/W) Interrupt Enable
#define LPCSRA_A17     0000040 // (R/W) Bus Address Bit 17
#define LPCSRA_A16     0000020 // (R/W) Bus Address Bit 16
#define LPCSRA_MOD     0000014 // (R/W) Mode
#define LPCSRA_PAREN   0000002 // (R/W) Parity Enable
#define LPCSRA_GO      0000001 // (R/W) Go (Start DMA Transfers)

// Unibus Address 775402
// LPCSRB - Control and Status Register #B
#define LPCSRB_VDATA   0100000 // (R/W) Valid Data Flag
#define LPCSRB_LA180   0040000 //       Set if LA180 Type Printer
#define LPCSRB_NRDY    0020000 //       Set if Printer is Not Ready
#define LPCSRB_PAR     0010000 //       Parity Bit is Sent to Printer
#define LPCSRB_OPVFU   0004000 //       Optical VFU
#define LPCSRB_TST     0003400 // (R/W) Test bits
#define LPCSRB_OFLIN   0000200 //       Printer is Offline
#define LPCSRB_VFUER   0000100 // (R)   Vertical Format Unit Error
#define LPCSRB_PARER   0000040 // (R)   Parity Error
#define LPCSRB_MEMER   0000020 // (R)   Memory Parity Error
#define LPCSRB_RAMER   0000010 // (R)   RAM Parity Error
#define LPCSRB_SYNTO   0000004 // (R)   Master Sync Timeout
#define LPCSRB_DEMTO   0000002 // (R)   Demand Timeout
#define LPCSRB_GOERR   0000001 //       Go Set and Error Up or Demand Not Up

// Unibus Address 775404
// LPBSAD - DMA Bus Address Register
#define LPBSAD_ADR     0177777 // (R/W) Address of Printer Data Buffer

// Unibus Address 775406
// LPBCTR - Byte Count Register
#define LPBCTR_SP      0170000
#define LPBCTR_BC      0007777 // (R/W) Byte Count (0 = Done & Interrupt)

// Unibus Address 775410
// LPPCTR - Page Count Register
#define LPPCTR_SP      0170000
#define LPPCTR_PC      0007777 // (R/W) Page Count Limit

// Unibus Address 775412
// LPRAMD - RAM Data Register
#define LPRAMD_SP      0160000
#define LPRAMD_RPAR    0010000 // (R)   RAM Parity Error
#define LPRAMD_RINT    0004000 // (R/W) Interrupt
#define LPRAMD_RDEL    0002000 // (R/W) Delimiter
#define LPRAMD_RTRN    0001000 // (R/W) Translation
#define LPRAMD_RPI     0000400 // (R/W) Paper Instruction
#define LPRAMD_RDAT    0000377 // (R/W) RAM Data Addr (Bit 13 is R-Only)

// Unibus Address 775414
// LPCCTR - Column Count Register     (High Byte)
// LPCBUF - Character Buffer Register (Low Byte)
#define LPCCTR_CLCT    0177400 // (R/W) Column Count
#define LPCBUF_CBUF    0000377 // (R/W) Last Data Byte from Memory

// Unibus Address 775416
// LPCKSM - Checksum Register         (High Byte)
// LPTDAT - Printer Data Register     (Low Byte)
#define LPCKSM_CHK     0177400 // (R)   Checksum
#define LPTDAT_DATA    0000377 // (R)   Data Sent to Printer
