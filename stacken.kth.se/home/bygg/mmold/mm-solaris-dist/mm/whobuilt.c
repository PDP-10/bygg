/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/whobuilt.c,v 2.1 90/10/04 18:27:03 melissa Exp $";
#endif

#ifdef WHOBUILT
char *whobuilt = WHOBUILT;
#else
char *whobuilt = 0;
#endif
