/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/mh.c,v 2.0.1.3 1997/10/21 19:33:32 howie Exp $";
#endif

/* mh.c: routines to handle mh type mail files */

#include "mm.h"


mh_open (mail)
msgvec *mail;
{
    printf ("mh format not supported yet\n");
    return(false);			/* let them know we failed */
}

mh_probe (file)
char *file;
{
    return (false);
}
/* things to be done */

mh_close () {
    return(false);			/* let them know we failed */
}
mh_rdmsg () {
    return(false);			/* let them know we failed */
}
mh_wrmsg () {
    return(false);			/* let them know we failed */
}
