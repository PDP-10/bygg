// tape.h - Definitions for the tape emulation
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

#define MT_UNKNOWN  0
#define MT_TAPE     1
#define MT_WILSON   2   // Wilson's tape format file
#define MT_AWSTAPE  3
#define MT_SOCKET   4
#define MT_TPC      128 // Not supported for emulator.

// Position of tape definitions
#define MT_OK        0 // Normal operation
#define MT_TMARK     0 // Tape Mark encountered
#define MT_BOT      -1 // Bottom of Tape reached
#define MT_EOT      -2 // End of Tape reached
#define MT_ERROR    -3 // Error occured during operation
#define MT_NOTSUP   -4 // Not supported

#define MT_WRITABLE 1
#define MT_SEEKABLE 2

typedef struct {
	int Type;  // tape type: tape, image or socket fd
	int fd;    // tape drive, image, or socket file descriptor
	int bpi;   // tape density
	int mode;  // writable/seek access
	int pos;   // Current position of tape
	int error; // Last error code
} MTAPE;
