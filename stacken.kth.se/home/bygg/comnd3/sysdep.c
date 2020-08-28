#include "comnd.h"

/*
** Find out what code to generate:
*/

#if defined (SYS_DUMMY)         /* Force just a dummy module? */
#  include "sys_dummy.h"
#elif defined (SYS_BSD)		/* Force BSD code? */
#  include "sys_bsd.h"
#elif defined (SYS_POSIX)	/* Force POSIX? */
#  include "sys_posix.h"
#elif defined (SYS_LINUX)       /* Thorvald-ux (tux)? */
#  include "sys_linux.h"
#elif defined (SYS_SOLARIS)	/* Force SOLARIS? */
#  include "sys_sol.h"

/*
** No forced version, try heuristics:
*/

#elif defined (__FreeBSD__) || \
      defined (__NetBSD__) || \
      defined (__OpenBSD__)     /* Known BSD? */
#  include "sys_posix.h"	/*   ... should be POSIX. */

#elif defined (__linux__)       /* Thorvald-ux (tux)? */
#  include "sys_posix.h"	/*   ... should be POSIX. */

#elif defined(__sun) && defined(__SVR4) /* Solaris? */
#  include "sys_posix.h"	/*   ... should be POSIX. */

/*
** Can't do anything, have to use a dummy.
*/

#else
#  include "sys_dummy.h"
#endif
