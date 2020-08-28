// defs.h - Definitions for the main emulator routines
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

typedef char               int8;
typedef short              int12;
typedef short              int16;
typedef long               int18;
typedef long               int30;
typedef long               int32;
typedef long long          int36;
typedef long long          int64;

typedef unsigned char      uint8;
typedef unsigned short     uint12;
typedef unsigned short     uint16;
typedef unsigned long      uint18;
typedef unsigned long      uint30;
typedef unsigned long      uint32;
typedef unsigned long long uint36;
typedef unsigned long long uint64;

typedef int         boolean;

typedef const char    cchar;
typedef unsigned char uchar;

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <setjmp.h>
#include <errno.h>

#define TRUE  1
#define FALSE 0

// Maximum number of arguments.
#define TS10_MAXARGS 256

#define EMU_OK           0               // Normal
#define EMU_BASE         64              // Base for Messages
#define EMU_FATAL        (EMU_BASE +  0) // Fatal Error
#define EMU_NXM          (EMU_BASE +  1) // Non-existant Memory
#define EMU_MEMERR       (EMU_BASE +  2) // Memory Error
#define EMU_OPENERR      (EMU_BASE +  3) // Open Error
#define EMU_IOERR        (EMU_BASE +  4) // I/O Error
#define EMU_PRESENT      (EMU_BASE +  5) // Unit is already present
#define EMU_NPRESENT     (EMU_BASE +  6) // Unit is not present
#define EMU_DISABLED     (EMU_BASE +  7) // Unit is disabled
#define EMU_NOATT        (EMU_BASE +  8) // Unit is not attachable
#define EMU_ATTACHED     (EMU_BASE +  9) // Unit is already attached
#define EMU_ARG          (EMU_BASE + 10) // Bad Argument
#define EMU_UNKNOWN      (EMU_BASE + 11) // Unknown Command
#define EMU_NOTFOUND     (EMU_BASE + 12) // Not Found
#define EMU_CONFLICT     (EMU_BASE + 13) // Conflict
#define EMU_NOTSUPPORTED (EMU_BASE + 14) // Not supported
#define EMU_NOTBOOTABLE  (EMU_BASE + 15) // Not bootable

#define EMU_ILR      (EMU_BASE + 128)  // Illegal Register
#define EMU_ZWC      (EMU_BASE + 129)  // Zero Word Count
#define EMU_PFAIL    (EMU_BASE + 130)  // Page Failure

#define EMU_QUIT     0
#define EMU_CONSOLE  1
#define EMU_RUN      2
#define EMU_HALT     3

#define EMU_MAXARGS  16

#define emu_Abort(e, x) longjmp(e, x)

extern FILE *debug;
extern int  emu_State;
extern int  emu_logFile;

typedef struct Command   COMMAND;
typedef struct IOCommand IOCOMMAND;
typedef struct dType     DTYPE;
typedef struct Unit      UNIT;
typedef struct Device    DEVICE;
typedef struct MapDevice MAPDEVICE;

// Command table
struct Command {
	char  *nCommand;
	int   (*Execute)(int, char **);
};

// I/O Command table
struct IOCommand {
	char  *Name;   // Name of Command
	char  *Usage;  // Usage information
	int   (*Execute)(UNIT *, int, char **);
};

/* Drive type table - data structure */
struct dType {
	char  *Name;      /* Device Name */
	char  *Desc;      /* Description */
	int32 Flags;      /* Device flags */
	int32 Sectors;    /* Number of Sectors */
	int32 Tracks;     /* Number of Tracks (Heads) */
	int32 Cylinders;  /* Number of Cylinders */
	int32 Blocks;     /* Number of Blocks */
	int32 dType;      /* Device Type ID */
	DTYPE *sTypes;    /* Slave Drive Types */

	int32 uFlags;     /* User-definable Flags */
	void  *uData;     /* User-definable Data */

	// Low-level Routines for disk, tape, ethernet, etc..
	int    (*Open)();        // low-level Open Routine
	int    (*Close)();       // low-level Close Routine
	int    (*Read)();        // low-level Read Routine
	int    (*Write)();       // low-level Write Routine
	int    (*Rewind)();      // low-level Rewind Routine
	int    (*Skip)();        // low-level Skip Routine
	int    (*Seek)();        // low-level Seek Routine
	int    (*GetDiskAddr)(); // low-level GetDiskAddr Routine
};

/* Unit data structure */
struct Unit {
	int32     idUnit;     // Unit #
	int32     tFlags;     // Type flags
	int32     Flags;      // Flags
	char      *devName;   // Device name itself
	char      *FileName;  // Open file name
	void      *FileBuf;   // Memory Buffer
	void      *FileRef;   // File Reference
	DTYPE     *dType;     // Desired drive type
	DEVICE    *Device;    // Device Reference
	UNIT      *pUnit;     // Parent Unit
	UNIT      *sUnits;    // Slave/Child Units
	UNIT      *cpuUnit;   // CPU Unit
	MAPDEVICE *mapDevice; // Device mapping table for CPU use.

	int       nUnits;     // Number of Slave/Child Units
	int       cUnit;      // Current of Unit #
	UNIT      *pcUnit;    // Pointer to Current Unit

	int32     Blocks;     // TEMP: total Blocks
	int32     szBlock;    // TEMP: Block size

	int32     uFlags;     // User-definable Flags
	void      *uData;     // User-definable Data
};

/* Device data structure */
struct Device {
	char      *Name;        /* Device Name */
	char      *Desc;        /* Description */
	char      *Version;     /* Version */
	DTYPE     *dTypes;      /* Listing of Drive Types */
	UNIT      *Units;       /* Units List */
	DEVICE    **Devices;    /* Slave Devices */
	IOCOMMAND *Commands;    /* Commands List */
	IOCOMMAND *SetCommands; /* SET Commands List */
	IOCOMMAND *ShowCommands;/* SHOW Commands List */
	int       nDevices;     /* Number of Devices; */
	int       nUnits;       /* Number of Units; */

	void     (*Initialize)();  /* Initialization Routine */
	void     (*Reset)();       /* Reset Routine */
	int      (*Create)();      /* Create Routine */
	int      (*Delete)();      /* Delete Routine */
	int      (*Configure)();   /* Configure Routine */
	int      (*Enable)();      /* Enable Routine */
	int      (*Disable)();     /* Disable Routine */
	int      (*Attach)();      /* Attach Routine */
	int      (*Detach)();      /* Detach Routine */
	int      (*Format)();      /* Format Routine */
	int      (*ReadIO)();      /* Read Routine */
	int      (*WriteIO)();     /* Write Routine */
	int      (*Process)();     /* Process Routine */
	int      (*Boot)();        /* Boot Routine */
	int      (*Execute)();     /* Execute Routine */
 
	UNIT     *(*SetUnit)();    /* SetUnit Routine */
	void     (*SetATA)();      /* SetATA Routine */
	void     (*ClearATA)();    /* ClearATA Routine */
	int      (*ReadData18)(UNIT *, int18 *);  /* ReadData Routine */
	int      (*ReadData36)(UNIT *, int36 *);  /* ReadData Routine */
	int      (*WriteData18)(); /* WriteData Routine */
	int      (*WriteData36)(); /* WriteData Routine */
	void     (*Ready)();       // Ready (Job Done) Routine
	int      (*CheckWordCount)(); // Check Word Count Routine
};

// Mapping Device definitions
struct MapDevice {
	char   *Name;
	UNIT   *Unit;
};

#define MAX_MAPDEVICES 256  // Up to 256 devices

// Boot type for devices
#define BOOT_UNKNOWN  0
#define BOOT_DISK     1
#define BOOT_MAGTAPE  2
#define BOOT_NETWORK  3

// Unit type for devices
#define UT_UNKNOWN    0 // Unit is unknown
#define UT_PROCESSOR  1 // Unit is CPU/Processor
#define UT_MEMORY     2 // Unit is memory
#define UT_INTERFACE  3 // Unit is bus interface
#define UT_CONTROLLER 4 // Unit is controller
#define UT_NETWORK    5 // Unit is network
#define UT_STORAGE    6 // Unit is storage medium

// Unit flags for devices
#define UNIT_WRLOCKED   010000 // Device is write-locked
#define UNIT_DISABLE    004000 // Device is disable
#define UNIT_DISABLED   002000 // Device is disabled
#define UNIT_REMOVABLE  001000 // Device is removable
#define UNIT_ATTABLE    000400 // Device is attachable
#define UNIT_ATTACHED   000200 // Device is attached
#define UNIT_BUFABLE    000100 // Device is bufferable
#define UNIT_BUFREQ     000040 // Device must have buffer
#define UNIT_BUFFERED   000020 // Device is buffered
#define UNIT_FIXED      000010 // Device capacity is fixed
#define UNIT_SEQ        000004 // Device is sequential
#define UNIT_RO         000002 // Device allows read access only
#define UNIT_PRESENT    000001 // Device is present

#ifdef DEBUG

typedef struct {
	char *Name;
	int  Mode;
} DBG_MODES;

// Debugging Facility Definitions
#define DBG_SOCKETS    01000   // Sockets
#define DBG_INTERRUPT  00400   // Interrupt
#define DBG_CONSOLE    00200   // I/O Console
#define DBG_IODATA     00100   // I/O Data Transfers
#define DBG_IOREGS     00040   // I/O Registers
#define DBG_TABLE      00020   // Instruction Table
#define DBG_DATA       00010   // Data Watch
#define DBG_OPERAND    00004   // Operand Watch
#define DBG_TRACE      00001   // Execution Trace

#endif // DEBUG

// Function definitions (Prototype)
#include "emu/extern.h"
#include "emu/proto.h"
