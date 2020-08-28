/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/pop3.c,v 2.0.1.2 1997/10/21 19:33:32 howie Exp $";
#endif

/* pop3.c: read Columbia's special pop-3 format remote mail */

#include "mm.h"				/* need defn of FILE */

pop3_open (mail)
msgvec *mail;
{
    printf ("pop2 format not supported yet\n");
    return(false);			/* let them know we failed */
}

pop3_close (fp)
FILE *fp;
{
    return(false);			/* let them know we failed */
}

pop3_rdmsg () {
    return(false);			/* let them know we failed */
}

pop3_wrmsg () {
    return(false);			/* let them know we failed */
}

/* 
 * probe file to see if it's in pop3 format 
 * I don't know what this means yet...
 */
pop3_probe (file)
char *file;
{
    return (false);			/* naah... */
}
