/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/version.c,v 2.3 90/10/04 18:26:59 melissa Exp $";
#endif

/*
 * Dump all of the rcs-id's from MM's header files.
 */

#define RCSID
#include "mm.h"
#include "parse.h"
#include "cmds.h"
#include "message.h"
#include "babyl.h"
#include "rd.h"
#include "set.h"
#include "help.h"
#undef RCSID

/*
 * Pull in the version string, major and minor release numbers, the edit
 * number, and the who-compiled-mm string, and make them available in global
 * variables.  This is ugly but it obviates the need to recompile all the
 * modules that might want to reference the edit numbers every time version.h
 * changes.
 */

#include "version.h"

char *mm_version = MM_VERSION;
char *mm_compiled = MM_COMPILED;
int mm_major_version = MM_MAJOR;
int mm_minor_version = MM_MINOR;
int mm_patch_level = MM_PATCH;
int mm_edit_number = MM_EDIT;

/*
 * Make a guess at the operating system type, so we can include that
 * info in the headers of bug reports sent with the "bug" command.
 */

#if AIX
char *OStype = "AIX";
#else
#if hpux
char *OStype = "hpux";
#else
#ifdef pyr
char *OStype = "Pyramid";
#else
#if accel
char *OStype = "Accel";
#else
#if ultrix
char *OStype = "Ultrix";
#else
#if sun
char *OStype = "SunOS";
#else
#if BSD
char *OStype = "BSD";
#else
#if SYSV
#if SVR3
char *OStype = "SVR3";
#else
#if SVR2
char *OStype = "SVR2";
#else
char *OStype = "SYSV";
#endif
#endif
#else
#if MSDOS
char *OStype = "MS-DOS";
#else
char *OStype = "unknown";
#endif /* MSDOS */
#endif /* SYSV */
#endif /* BSD */
#endif /* SUN */
#endif /* Ultrix */
#endif /* accel */
#endif /* pyramid */
#endif /* hpux */
#endif /* AIX */
