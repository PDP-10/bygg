// socket.h - Socket definitions
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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define NET_MAXSOCKETS  256  // Maximum number of open sockets.
#define NET_MAXBUF      256  // Manimum number of bytes of buffer.

typedef struct sockaddr    SOCKADDR;
typedef struct sockaddr_in SOCKADDRIN;

typedef struct Socket SOCKET;

struct Socket {
	int         idSocket; // Socket ID (FD number)
	int         Flags;    // Socket Flags
	SOCKET      *Server;  // Parent socket.
	int         maxConns; // Maximum number of opened connections.
	int         nConns;   // Current number of opened connections.
	SOCKADDRIN  locAddr;  // Local Internet Address/Port
	SOCKADDRIN  remAddr;  // Remote Internet Address/Port

	// Callback functions
	void (*Accept)(SOCKET *);
	void (*Eof)(SOCKET *, int, int);
	void (*Process)(SOCKET *, char *, int);
};

// Flag definitions for Socket variable
#define SOCK_OPENED    0x80000000 // Socket is opened or closed.
#define SOCK_SERVER    0x40000000 // Socket is server or client.
#define SOCK_LISTEN    0x20000000 // Socket is listening.
#define SOCK_CONNECT   0x10000000 // Socket is connecting (incoming).

// Socket mode defintions - Server, Client, or Connected.
#define NET_SERVER   1 // Socket's Server role
#define NET_CLIENT   2 // Socket's Client role
#define NET_CONNECT  3 // Incoming socket connection

// Socket error definitions
#define NET_OK            0 // Operation successful
#define NET_OPENERR       1 // Open Error
#define NET_BINDERR       2 // Bind Error
#define NET_FULLSOCKETS   3 // Sockets are full
#define NET_UNKNOWNMODE   4 // Unknown Mode
#define NET_SOCKERR       5 // Socket Error
#define NET_NOTVALID      6 // Not valid socket

// Telnet code definitions
#define TEL_SE    240 // End of subnegotiation parameters
#define TEL_NOP   241 // No operation.
#define TEL_DM    242 // Data Mark
#define TEL_BRK   243 // Break
#define TEL_IP    244 // Interrupt Process
#define TEL_AO    245 // Abort Output
#define TEL_AYT   246 // Are You There?
#define TEL_EC    247 // Erase Character
#define TEL_EL    248 // Erase Line
#define TEL_GA    249 // Go Ahead
#define TEL_SB    250 // Subnegotiation
#define TEL_WILL  251 // Will (Option code)
#define TEL_WONT  252 // Will Not (Option Code)
#define TEL_DO    253 // Do (Option Code)
#define TEL_DONT  254 // Don't (Option Code)
#define TEL_IAC   255 // Interpret as Command

// Protoype definitions
void   sock_Intialize(void);
void   sock_Cleanup(void);
#ifdef DEBUG
void   sock_Dump(int, uchar *, int, char *);
#endif
SOCKET *sock_Open(int, int);
int    sock_Close(SOCKET *);
int    sock_Listen(SOCKET *, int);
SOCKET *sock_Accept(SOCKET *);
int    sock_Send(int, char *, int);
int    sock_ProcessTelnet(uchar *, int);
void   sock_Handler(int);
void   sock_ShowList(void);
