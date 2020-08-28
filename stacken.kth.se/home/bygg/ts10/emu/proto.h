// proto.h - Prototypes for the emulation routines
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

// Console Commands - console.c
int  console_Execute(char *);
void console_ExecuteFile(char *);
void console_Prompt(void);

// Unit Attach/Detach - unit.c
int  unit_mapCreateDevice(char *, UNIT *);
int  unit_mapDeleteDevice(char *);
UNIT *unit_mapFindDevice(char *);

DEVICE *unit_FindDevice(DEVICE **, char *);
int unit_Attach(UNIT *, char *);
int unit_Detach(UNIT *);
int unit_CmdCreate(int, char **);
int unit_CmdDelete(int, char **);
int unit_CmdAttach(int, char **);
int unit_CmdDetach(int, char **);
int unit_CmdFormat(int, char **);
int unit_CmdInit(int, char **);
int unit_CmdBoot(int, char **);
int ts10_CmdUse(int, char **);

// Console TTY - emu/tty.c
void tty_Initialize(void);
void tty_RunState(void);
void tty_CmdState(void);
int tty_GetChar(void);
int tty_GetString(char *, int);
int tty_PutChar(int32);

// Timer - emu/timer.h
int timer_Start(void);
int timer_Stop(void);
int timer_SetAlarm(void (*)(int));

// Emulated Disk routines - emu/disk.c
int disk_Open(UNIT *, char *, int);
int disk_Close(UNIT *);
int disk_Read(UNIT *, uint8 *);
int disk_Write(UNIT *, uint8 *);
int disk_Seek(UNIT *, int32, int);
int disk_GetError(UNIT *);
int disk_GetDiskAddr(UNIT *, int, int, int);

// Emulated Tape routines - emu/tape.c
int tape_Open(UNIT *, char *, int);
int tape_Close(UNIT *);
int tape_Read(UNIT *, uint8 *, int);
int tape_Write(UNIT *, uint8 *, int);
int tape_Rewind(UNIT *);
int tape_Mark(UNIT *);
int tape_SkipRec(UNIT *, int);
int tape_GetError(UNIT *);

// system.c
void  ts10_Initialize(void);
int   ts10_Create(UNIT *, char *, int, char **);
int   ts10_Delete(UNIT *);

// Utilities - emu/utils.c
void  util_RemoveSpaces(register char *);
char  *util_SplitChar(register char **, register char);
char  *util_SplitWord(register char **);
char  *util_SplitQuote(register char **);
void  util_ToUpper(register char *);
int   util_GetInteger(char *, int, int, int *);
int   util_ToInteger(char *, char **, int);
char  *util_ToBase10(uint32);
int36 util_Convert8to36(uchar *);
void  util_Convert36to8(int36, uchar *);
int36 util_PackedASCII6(uchar *);
int36 util_PackedASCII7(uchar *);
char  *emu_nowTime(cchar *);
void  emu_Printf(cchar *, ...);
char  *StrChar(char *, char);

#ifdef DEBUG
// Debugging Facility - emu/debug.c
boolean dbg_Check(int);
void    dbg_SetMode(int);
void    dbg_ClearMode(int);
int     dbg_GetMode(void);
void    dbg_Printf(cchar *, ...);
int     dbg_CmdDebug(int, char **);
int     dbg_CmdTrace(int, char **);
int     dbg_CmdAsm(int, char **);
int     dbg_CmdDisasm(int, char **);
#endif
