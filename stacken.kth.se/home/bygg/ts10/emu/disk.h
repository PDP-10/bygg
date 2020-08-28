// disk.h - Definitions for the disk emulation
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

#define DISK_UNKNOWN   0
#define DISK_IMAGE     1

#define DISK_WRITABLE  1

typedef struct {
	int	Type;	
	int	Mode;       // Writable, etc.
	int	Error;		// Error report
	int	szBlock;    // Block size, i.e. 256, 512, or 576 bytes per block
	int	fd;         // File Description
} DISK;
