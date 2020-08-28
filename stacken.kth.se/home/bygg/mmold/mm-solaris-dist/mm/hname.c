/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/hname.c,v 2.1 90/10/04 18:24:31 melissa Exp $";
#endif

/*
 * hname - return the local hostname
 *
 */

#include "mm.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#ifdef MMHOSTNAME
char *
hname ()
{
    return (char *) MMHOSTNAME;
}

isourhost (host)
{
    return (ustrcmp (host, MMHOSTNAME) == 0);
}
#else

#ifdef GETHOSTNAME
#include <netdb.h>

char *
hname ()
{
    char name[MAXHOSTNAMELEN], *result;		
    struct hostent *h, *gethostbyname();

    gethostname(name, sizeof name);
    if (h = gethostbyname(name)) {	/* get host entry */
	result = malloc (strlen (h->h_name) +1);
	if (result)
	    strcpy (result, h->h_name);
	else
	    panic ("hname: no space\n");
    }

    return result;
}

/*
 * This is too expensive--needs to be redone.
 */
isourhost (host)
char *host;
{
    char name[MAXHOSTNAMELEN];
    struct hostent *h, *gethostbyname ();
    char **aliases;

    /* don't bother the nameserver if we don't have to */
    if (ustrcmp (host, ourhostname) == 0)
	return true;

    if (h = gethostbyname (host)) {
	if (ustrcmp (h->h_name, ourhostname) == 0)
	    return true;
    }

    return false;
}
#else
#ifdef DOUNAME
/*
 * XXX - not tested
 */
#include <sys/utsname>

char *
hname ()
{
    char *result;
    struct utsname name;

    uname (&name);
    result = malloc (strlen (name.nodename) + 4);
    if (result)
	strcpy (result, name.nodename);
    else
	panic ("hname: no space\n");
    return result;
}

isourhost (host)
{
    return (ustrcmp (host, ourhostname) == 0);
}
#endif /* !DOUNAME */ */
#endif /* !GETHOSTNAME */
#endif /* !defined (MMHOSTNAME) */
