/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

*/
/*
 * This file contains conditional compilation directives describing the
 * presence or absence of certain common features which are not unique
 * to any one generic environment.
 *
 * Machine dependencies which need to be reflected in the compilation
 * of ccmd applications should go into ccmdmd.h.
 */

/* One of these needs to be defined... */
/* #define DIRLIB */		/* e.g. 4.2bsd */
/* #define NDIRLIB */		/* e.g. hp-ux */
/* #define DIRENTLIB */		/* e.g. posix? */
/* #define NODIRLIB */		/* e.g. v7 */

/* completion is slow when using Sun's YP facility */
/* #define NO_USERNAME_COMPLETION */
/* #define NO_GROUP_COMPLETION */

#include "site.h"

#ifdef BSD
#define HAVE_BSTRING
#endif

#ifdef SYSV
#define bzero(a,b)	memset((a),0,b)
#define bcopy(a,b,c)	memcpy((b),(a),c)
#define bcmp(a,b,c)	memcmp((a),(b),c)
#define HAVE_BSTRING
#endif

#if defined(SVR3) || defined(SVR4)
#if	!defined(HAVE_VOIDSIG)
#	define HAVE_VOIDSIG
#endif	/* HAVE_VOIDSIG */
#if	!defined(DIRENTLIB)
#	define DIRENTLIB
#endif	/* DIRENTLIB */
#endif	/* SVR3 ... */


#if !defined(DIRLIB) && !defined(NDIRLIB) && !defined(DIRENTLIB)
#  ifndef NODIRLIB
#    if BSD
#      define DIRLIB
#    else
#      if hpux
#        define NDIRLIB
#      else
#	 if AIX
#	   define DIRENTLIB
#	 else
#          define NODIRLIB
#	 endif
#      endif
#    endif
#  endif
#endif

#if defined(HAVE_VOIDSIG)
#define SIG void
#else
#define SIG int
#endif
typedef SIG (*sigval)();
