/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Andrew Lowry
*/
/* ccmdst.c
**
** This module exists only for allocating a few global tables that are
** declared via the cmfnc.h header file.  Since this header is likely
** to change frequently, isolating the allocation in this small module
** should make recompilation requirements minimal.
**/

#define	STORAGE			/* this triggers allocation */
#define	GENERR			/* generic error table goes here too */
#include "ccmd.h"		/* load symbols and get declarations */
#include "cmfncs.h"		/* and internal symbols */
