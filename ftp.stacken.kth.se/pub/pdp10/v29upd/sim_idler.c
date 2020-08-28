/*
** sim_idler.c: os-dependent routines to idle, null-job style.
*/

/*
** Written 2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

/*
** Revision history:
**
** 2002-04-16    Made into a module.
*/

#include "sim_defs.h"
#include "sim_idler.h"

/*
** Find out what code to generate:
*/

#if defined (IDLER_DUMMY)	/* Force just a dummy module? */
#  define idler_dummy		/*   Yes, set up for later. */

#elif defined (IDLER_BSD)	/* Force BSD code? */
#  include "idl_bsd.h"

/*
** No forced version, try heuristics:
*/

#elif defined (__FreeBSD__) || \
      defined (__NetBSD__) || \
      defined (__OpenBSD__)	/* Known BSD? */
#  include "idl_bsd.h"

#else
#  define idler_dummy		/* No heuristics, default to dummy. */
#endif


/*
** If no good idle code can be found, gen a dummy routine:
*/

#ifdef idler_dummy

int32 sim_idle(int32 cycles)
{
  return 0;			/* Should stop further usage. */
}

#endif
