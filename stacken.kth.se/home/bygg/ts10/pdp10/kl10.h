// kl10.h - Definitions for the KL10 Processor emulation
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

// KL10 Processor - definitions

// Memory Size and Limitation
#define MAXMEMSIZE  (1 << 23) // 23-bit Addressing   - 4096K words
#define MAXSECSIZE  037       // Maximum Section     - 37 sections
#define INIMEMSIZE  (1 << 20) // Initial memory size - 1024K words

// Arithmetic Processor Identification
//
// Status Register - Read/Write Access
//
// |<-------- Left Halfword ---------->|<-------- Right Halfword --------->|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  Ucode Options  |  Ucode Version  | Hard Opts |     Serial Number     |
// +-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-+
//  0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//
//  Bits    Defintion
//  ----    ---------
//  0:8     Microcode Options
//   0        TOPS-20 Paging System (0 - T10, 1 - T20)
//   1        Extended Addressing
//   2        Exotic Microcode
//  9:17    Microcode Version
// 18:23    Hardware Options
//   18       50 Hz (0 - 60 Hz, 1 - 50 Hz)
//   19       Cache
//   20       Extended KL10 (0 - Single Section, 1 - Extended Section)
//   21       Master Osciiator
// 24:35    Serial Number

#define APRID_UC_OPTS    0777000000000LL // Microcode Options
#define APRID_UO_T20     0400000000000LL //    TOPS-20 Paging System
#define APRID_UO_XADR    0200000000000LL //    Extended Addressing
#define APRID_UO_EXOTIC  0100000000000LL //    Exotic Microcode
#define APRID_UC_VER     0000777000000LL // Microcode Version
#define APRID_HW_OPTS    0000000770000LL // Hardware Options
#define APRID_HO_50HZ    0000000400000LL //    50 Hz
#define APRID_HO_XKL     0000000200000LL //    Extended KL10
#define APRID_HO_MOSC    0000000100000LL //    Master OSC
#define APRID_SN         0000000007777LL // Serial Number

// Arithmetic Processor
//
// Status Register - Read/Write Access
//
// |<-------- Left Halfword ---------->|<-------- Right Halfword --------->|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |0 0 0 0 0 0 0 0 0|F|R|W|U|               Break Address                 |
// +-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-^-+-+-+
//  0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//
//  Bit     Defintion
//  ---     ---------
//   9      Fetch \
//  10      Read   > Reference Type
//  11      Write / 
//  12      User Space
//  13:35   Break Address

#define APR_REF_FETCH  0000400000000LL
#define APR_REF_READ   0000200000000LL
#define APR_REF_WRITE  0000100000000LL
#define APR_REF_USER   0000040000000LL
#define APR_REF_CONDS  0000740000000LL
#define APR_BRK_ADDR   0000037777777LL

// Priority Interrupt System
//
// Status Register - Write Access
//
// |<------------------ Right Halfword ----------------->|
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |PA|PD|PR|00|DR|CS|II|I+|I-|S+|S-|   Selected Levels  |
// +--+--+--^--+--+--^--+--+--^--+--+--^--+--+--^--+--+--+
//  18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35
//
//  Bit     Defintion
//  ---     ---------
//  18      (PA) Addresss   \ 
//  19      (PD) Data        >  Write Even Parity
//  20      (PR) Directory  /
//  22      (DR) Drop Program Requests (on selected Levels)
//  23      (CS) Clear PI System
//  24      (II) Initial Interrupts On \
//  25      (I+) Turn Interrupts On     >(on selected Levels)
//  26      (I-) Turn Interrupts Off   /
//  27      (S+) Turn PI System On
//  28      (S-) Turn PI System Off
//  29:35   Selected Levels
//
// Status Register - Read Access
//
// |<------------------ Left Halfword ------------------>|
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |00 00 00 00 00 00 00 00 00 00 00|  Program Requests  |
// +--+--+--^--+--+--^--+--+--^--+--+--^--+--+--^--+--+--+
//  00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17
//
// |<------------------ Right Halfword ----------------->|
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |PA|PD|PR|    In Progress     |On|     Levels On      |
// +--+--+--^--+--+--^--+--+--^--+--+--^--+--+--^--+--+--+
//  18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35
//
//  Bit     Defintion
//  ---     ---------
//  11:17   Program Requests on Levels
//  18      (PA) Addresss   \ 
//  19      (PD) Data        >  Write Even Parity
//  20      (PR) Directory  /
//  21:27   Interrupt in Progress on Levels
//  28      PI System On
//  29:35   Levels On

// Write Access
#define PIW_WEP_ADDR     0400000 // Address   \
#define PIW_WEP_DATA     0200000 // Data       > Write Even Parity
#define PIW_WEP_DIR      0100000 // Directory /
#define PIW_WEP_MASK     0700000 // Write Even Parity (Mask)
#define PIW_INT_DROP     0020000 // Drop Program Requests
#define PIW_SYS_CLEAR    0010000 // Clear PI System
#define PIW_INT_REQS     0004000 // Initial Interrupts On (Requests)
#define PIW_INT_ON       0002000 // Turn Interrupts On
#define PIW_INT_OFF      0001000 // Turn Interrupts Off 
#define PIW_SYS_ON       0000400 // Turn PI System On
#define PIW_SYS_OFF      0000200 // Turn PI System Off
#define PIW_INT_LEVELS   0000177 // Interrupt Levels Mask

// Read Access
#define PIR_INT_REQS     0000177000000LL // Program Requests
#define PIR_WEP_ADDR     0000000400000LL // Address   \
#define PIR_WEP_DATA     0000000200000LL // Data       > Write Even Parity
#define PIR_WEP_DIR      0000000100000LL // Directory /
#define PIR_WEP_MASK     0000000700000LL // Write Even Parity (Mask)
#define PIR_INT_PROG     0000000077400LL // Interrupts in Progress
#define PIR_SYS_ON       0000000000200LL // PI System On
#define PIR_INT_ON       0000000000177LL // Interrupts On

