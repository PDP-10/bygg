/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *compat_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/compat.h,v 2.2 90/10/04 18:23:48 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * This file contains definitions needed to resolve incompatibilities
 * between various different C library implementations.  Conditional
 * declarations here depend on macros defined in config.h, and perhaps
 * <sys/param.h> (included by osfiles.h), so this file should be included
 * after them, but before any other other header files or source code
 * which may depend on these declarations.
 */

/*
 * map index & rindex to strchr and strrchr if we don't have them.
 */
#ifndef HAVE_INDEX
#define index strchr
#define rindex strrchr
#endif

/*
 * non-Berkeley C libaries generally don't have bzero and friends
 */
#ifndef HAVE_BSTRING
#define bzero(a,b)	memset((a),0,b)
#define bcopy(a,b,c)	memcpy((b),(a),c)
#define bcmp(a,b,c)	memcmp((a),(b),c)
#endif

/*
 * Some systems don't have rename(2)
 */
#ifndef HAVE_RENAME
#define rename bsdrename
#endif

/*
 * Some systems don't have have vfork(2)
 */
#ifndef HAVE_VFORK
#define vfork fork
#endif

/*
 * Non-berkeley systems generally don't have getwd(2).
 */
#ifndef HAVE_GETWD
#define getwd bsdgetwd
#endif

/*
 * In SVR3, signal(2) is declared void (*signal())() rather than
 * int (*signal())().
 */
#ifdef HAVE_VOIDSIG
#define signalhandler void
#else
#define signalhandler int
#endif

/*
 * Some older systems systems don't have unistd.h.
 */
#if defined(L_SET) && !defined(SEEK_SET)
#define SEEK_SET	L_SET
#define SEEK_CUR	L_INCR
#define SEEK_END	L_XTND
#endif

/*
 * Make sure filename strings are long enough.
 */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/*
 * Define a macro to give a process its own process group.
 */
#ifdef HAVE_BSD_SETPGRP
# define new_process_group() setpgrp(0, getpid ())
#else
# define new_process_group() setpgrp()
#endif


/*
 * Some systems define both, so the distinction isn't useful, and
 * we'll use SIGCHLD internally.
 */
#if defined(SIGCLD) && !defined(SIGCHLD)
#define SIGCHLD SIGCLD
#endif

/*
 * On systems without the Berkeley signal behavior, use special
 * versions of read(2) and write(2)
 */
#ifndef HAVE_BSD_SIGNALS
#define read sys_read
#define write sys_write
#endif

/*
 * Finally, try to ensure that one of BSD or SYSV is #defined, for
 * the benefit of code which assumes that one or the other is true.
 */

#if !defined(BSD) && !defined(SYSV)
#   if hpux || u3b || u3b2 || u3b5 || u3b15 || u3b20 || SVR2 || SVR3 || AIX
#	define SYSV 1
#   else
#	if defined(TIOCNOTTY) || sun || ultrix || accel || pyr || bsd4_2
#	    define BSD 1
#	endif
#   endif /* !SYSV */
#endif
