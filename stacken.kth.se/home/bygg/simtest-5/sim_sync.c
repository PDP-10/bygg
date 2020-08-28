/*
** sim_sync.c: os-dependent routines to access sync ports.
*/

/*
** Written 2001-2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

/*
** Revision history:
**
** 2002-04-15    Adopted from previous code.
** 2002-01-23    Modify for version 2.9.
** 2002-01-22    First attempt.
*/

#include "sim_defs.h"
#include "sim_sync.h"

/*
** Find out what code to generate:
*/

#if defined (SYNC_DUMMY)	/* Force just a dummy module? */
#  include "sync_dummy.h"

#elif defined (SYNC_BSD)	/* Force BSD code? */
#  include "sync_bsd.h"

/*
** No forced version, try heuristics:
*/

#elif defined (__FreeBSD__) || \
      defined (__NetBSD__) || \
      defined (__OpenBSD__)	/* Known BSD? */
#  include "sync_bsd.h"

#elif defined (__linux__)	/* Thorvald-ux (tux)? */
#  include "sync_linux.h"

/*
** Can't do anything, have to use a dummy.
*/

#else
#  include "sync_dummy.h"
#endif
