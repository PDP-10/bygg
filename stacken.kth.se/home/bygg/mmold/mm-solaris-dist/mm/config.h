/*
 * Copyright (c) 1986, 1987, 1988 by The Trustees of Columbia University
 * in the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *config_rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/config.h,v 2.7 1997/10/21 19:33:32 howie Exp $";
#endif
#endif /* RCSID */

/*
 * Configuration file for cunixa, cunixb, cunixd
 */

#define BUGSTO "bug-mm"

#if defined(SOLARIS)
#include "s-sol25.h"
#elif defined(__svr4__)
#include "s-svr4.h"
#elif defined(sun)
#include "s-sun40.h"
#define HAVE_F_SETLK
#elif defined(linux)
#include "s-linux.h"
#endif

