/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

*/

/*
 * First look for files which can be included unconditionally on any
 * UNIX variant.
 */

#if defined(SOLARIS)
#include <stdio.h>
#define BSD_COMP 1
#endif /* SOLARIS */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
/*
 * Make some initial assumptions.
 *
 * We should be able to configure ourself correctly if running under
 * SunOS, 4.3BSD, HP-UX (s300 or s800), or on a Celerity (?).
 *
 * On 4.2bsd systems, compile with -DBSD
 * On System V Release 3 systems, compile with -DSVR3
 * On System V Release 2 systems, compile with -DSVR2
 * On other System V systems, compile with -DSYSV
 *
 * On 4.1bsd, 32/V, System III, etc, some additional work
 * will probably be necessary.
 */

#if !defined(SYSV) && (SVR3 || SVR2)
#define SYSV 1
#endif

#if unix && !defined(BSD) && !defined(SYSV) && !defined(SOLARIS)
#   if hpux || u3b || u3b2 || u3b5 || u3b15 || u3b20
#	define SYSV 1
#   else
#   if defined(TIOCNOTTY) || sun || ultrix || accel
#	define BSD 1
#   endif
#   endif /* !SYSV */
#endif /* unix && ... */

/*
 * Make a guess as to some os-dependent include files we'll need.
 * These #define's can be overridden in site.h.
 */

#if BSD
#define needSYSTIME
#define needFCNTL
#undef  needUNISTD
#define needSYSFILE
#define STRINGS				/* index, rindex, etc */
#define BSTRING				/* bzero, bcopy, etc */
#endif /* BSD */

#if SYSV
#define needTERMIO
#define needFCNTL
#define needUNISTD			/* you may need to override */
#undef  needSYSFILE			/*  these two in site.h */
#undef  STRINGS
#undef  BSTRING
#endif /* SYSV */

#if defined(SOLARIS)
#define DIRENTLIB
#define needFCNTL
#endif /* SOLARIS */

/*
 * Next, include site-specific configuration information.
 * 
 */

#include "machdep.h"

/*
 * Now include the rest of the header files and add any other appropriate
 * definitions, based on what we know from the header files included so far.
 */

#ifdef needSYSTIME
#include <sys/time.h>
#else
#include <time.h>
#endif

/*
 * We need one or more of unistd.h, fcntl.h, and sys/file.h.
 */

#ifdef needFCNTL
#include <fcntl.h>
#endif
#ifdef needUNISTD
#include <unistd.h>
#endif
#if !defined(O_RDONLY) || !defined(R_OK)
#define needSYSFILE
#endif
#if !defined(SEEK_SET) && !defined(L_SET)
#define needSYSFILE
#endif
#ifdef needSYSFILE
#include <sys/file.h>
#endif
#if defined(L_SET) && !defined(SEEK_SET)
#define SEEK_SET	L_SET
#define SEEK_CUR	L_INCR
#define SEEK_END	L_XTND
#endif

/*
 * tty driver definitions
 */
#ifdef needTERMIO
#include <sys/termio.h>
#else
#include <sgtty.h>
#endif

/*
 * CCMD routines generally assume the presence of
 * the Berkeley strings(3) and bstring(3) routines.
 */
#if !defined(STRINGS)
#define index strchr
#define rindex strrchr
#endif

#if !defined(BSTRING)
#define bzero(a,b)	memset((a),0,b)
#define bcopy(a,b,c)	memcpy((b),(a),c)
#define bcmp(a,b,c)	memcmp((a),(b),c)
#endif

/*
 * We need to define exactly one of DIRLIB, NDIRLIB, DIRENTLIB, or NODIRLIB.
 */

#if !defined(DIRLIB) && !defined(NDIRLIB) && !defined(DIRENTLIB)
#if !defined(NODIRLIB)
#   if u3b || u3b2 || u3b5 || u3b15 || u3b20 || SVR3
#	define DIRENTLIB
#   else
#   if hpux
#	define NDIRLIB
#   else
#   if BSD
#	define DIRLIB
#   else
#	define NODIRLIB
#   endif /* !bsd */
#   endif /* !hpux */
#   endif /* !posix */
#endif
#endif

#ifdef DIRENTLIB
#   include <dirent.h>
#endif
#ifdef NDIRLIB
#   include <ndir.h>
#endif
#ifdef DIRLIB
#   include <sys/dir.h>
#endif

#ifdef NODIRLIB
#   if MSDOS
#	include <dos.h>
#	include <direct.h>
#   endif
#endif

/*
 * Miscellaneous other files we need.
 */

#include <varargs.h>
#include <signal.h>
#include <ctype.h>
#if unix
#include <utmp.h>
#endif

/*
 * On some systems, signal handlers are defined as returning (void).
 */

#ifndef SIGNALHANDLERTYPE
#define SIGNALHANDLERTYPE int
#endif
typedef SIGNALHANDLERTYPE (*signal_handler_t) ();

/*
 * Finally, include ccmd.h, which includes stdio.h and setjmp.h.
 */

#include "ccmd.h"
