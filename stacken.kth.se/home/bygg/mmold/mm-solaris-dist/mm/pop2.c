/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/pop2.c,v 2.0.1.2 1997/10/21 19:33:32 howie Exp $";
#endif

/* pop2.c: read standard POP format remote mail */

#include "mm.h"				/* need defn of FILE */

pop2_open (mail)
msgvec *mail;
{
    printf ("pop2 format not supported yet\n");
    return(false);			/* let them know we failed */
}

pop2_close (fp)
FILE *fp;
{
    return(false);			/* let them know we failed */
}

pop2_rdmsg () {
    return(false);			/* let them know we failed */
}

pop2_wrmsg () {
    return(false);			/* let them know we failed */
}

/* 
 * probe file to see if it's in pop2 format 
 * I don't know what this means yet...
 */
pop2_probe (file)
char *file;
{
    return (false);			/* naah... */
}
