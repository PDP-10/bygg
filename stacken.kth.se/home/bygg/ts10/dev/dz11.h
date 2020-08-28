// dz11.h - DZ11 Terminal communications
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
#define DZCSR  (000 >> 1) // (R/W)  Control and Status   (CSR)
#define DZRBUF (002 >> 1) // (R)    Receiver Buffer      (RBUF)
#define DZLPR  (002 >> 1) // (W)    Line Parameter       (LPR)
#define DZTCR  (004 >> 1) // (R/W)  Transmit Control     (TCR)
#define DZMSR  (006 >> 1) // (R)    Modem Status         (MSR)
#define DZTDR  (006 >> 1) // (W)    Transmit Data        (TDR)

// DZCSR - Control and Status Register
#define DZCSR_TRDY        0100000 // (R)   Transmit Ready
#define DZCSR_TIE         0040000 // (R/W) Transmit Interrupt Enable
#define DZCSR_SA          0020000 // (R)   Silo Alarm
#define DZCSR_SAE         0010000 // (R/W) Silo Alarm Enable
#define DZCSR_TLINE       0003400 // (R)   Transmit Line
#define DZCSR_RDONE       0000200 // (R)   Receiver Done
#define DZCSR_RIE         0000100 // (R/W) Receiver Interrupt Enable
#define DZCSR_MSE         0000040 // (R/W) Master Scan Enable
#define DZCSR_CLR         0000020 // (R/W) Clear
#define DZCSR_MAINT       0000010 // (R/W) Maintenance

// DZRBUF - Receiver Buffer
#define DZRBUF_DATA_VALID 0100000 // (R)   Data Valid
#define SZRBUF_OVRN       0040000 // (R)   Overrun
#define DZRBUF_FRAM_ERR   0020000 // (R)   Framing Error
#define DZRBUF_PAR_ERR    0010000 // (R)   Parity Error
#define DZRBUF_RX_LINE    0003400 // (R)   Line Number
#define DZRBUF_RBUF       0000377 // (R)   Received Character

// DZLPR - Line Parameter Register
#define DZLPR_RX_ON       0010000 // (W)   Receiver On
#define DZLPR_FREQ        0007400 // (W)   Speed Select
#define DZLPR_ODD_PAR     0000200 // (W)   Odd Parity
#define DZLPR_PAR_EN      0000100 // (W)   Parity Enable
#define DZLPR_STOP_CODE   0000040 // (W)   Stop Code
#define DZLPR_CHAR_LEN    0000030 // (W)   Character Length
#define DZLPR_LINE        0000007 // (W)   Line Number

// DZTCR - Transmit Control Register
#define DZTCR_DTR         0177400 // (R/W) Data Terminal Ready
#define DZTCR_LINE_EN     0000377 // (R/W) Line Enable

// DZMSR - Modem Status Register
#define DZMSR_CO          0177400 // (R)   Carrier
#define DZMSR_RI          0000377 // (R)   Ring Indicator

// DZTDR - Transmit Data Register
#define DZTDR_BRK         0177400 // (W)   Break
#define DZTDR_TBUF        0000377 // (W)   Transmit Buffer

//  Baud Rate Selection Chart
// Bits: 11 10 09 08  Baud Rate
//        0  0  0  0      50
//        0  0  0  1      75
//        0  0  1  0     110
//        0  0  1  1     134.5
//        0  1  0  0     150
//        0  1  0  1     300
//        0  1  1  0     600
//        0  1  1  1    1200
//        1  0  0  0    1800
//        1  0  0  1    2000
//        1  0  1  0    2400
//        1  0  1  1    3600
//        1  1  0  0    4800
//        1  1  0  1    7200
//        1  1  1  0    9600
//        1  1  1  1   Not used

#define DZ_B50    0
#define DZ_B75    1
#define DZ_B110   2
#define DZ_B134   3
#define DZ_B150   4
#define DZ_B300   5
#define DZ_B600   6
#define DZ_B1200  7
#define DZ_B1800  8
#define DZ_B2000  9
#define DZ_B2400 10
#define DZ_B3600 11
#define DZ_B4800 12
#define DZ_B7200 13
#define DZ_B9600 14

// Character Length Chart
//
//   Character   Bits
//    Length    04  03
//    ------    ------
//    5 bits     0   0
//    6 bits     0   1
//    7 bits     1   0
//    8 bits     1   1

#define DZ_5BIT  0
#define DZ_6BIT  1
#define DZ_7BIT  2
#define DZ_8BIT  3
