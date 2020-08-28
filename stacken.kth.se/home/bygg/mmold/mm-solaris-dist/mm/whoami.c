/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */


#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/whoami.c,v 2.1 90/10/04 18:27:01 melissa Exp $";
#endif

/* 
 * whoami - return the user's login name
 * 
 * In order to handle the situation where several login names map to the
 * same uid, the USER and LOGNAME environment variables, and the
 * /etc/utmp file are consulted for a name with a uid matching that
 * returned by getuid().  If that fails, the first entry in the passwd
 * database with a matching uid is returned.
 * 
 * If no name matching the current uid is found, NULL is returned.
 */

#include "config.h"
#include "osfiles.h"
#include "compat.h"
#include <pwd.h>

char *getenv (), *getlogin ();
struct passwd *getpwnam (), *getpwuid ();

static char *envariables[] = { "USER", "LOGNAME" };

char *
whoami ()
{
    int i;
    char *cp;
    struct passwd *pw;
    static int realuid = -1;		/* user's real uid */
    static char realname[64];		/* user's name */
    char loginname[256], envname[256];

    if (realuid != -1)
	return realname;

    realuid = getuid ();	       /* get our uid */

    /*
     * Check the conventional environment variables...
     */
    for (i = 0; i < sizeof envariables / sizeof envariables[0]; i++)
	if (cp = getenv (envariables[i]))
	    if (pw = getpwnam (cp))
		if (pw->pw_uid == realuid) {
		    strcpy (realname, cp);
		    return realname;
		}

    /*
     * See if there is an entry in the utmp file...
     */
    if (cp = getlogin ())
	if (pw = getpwnam (cp))
	    if (pw->pw_uid == realuid) {
		strcpy (realname, cp);
		return (realname);
	    }

    /*
     * Finally, look for the first maching entry in the passwd file.
     */
    if (pw = getpwuid (realuid)) {
	strcpy (realname, pw->pw_name);
	return realname;
    }

    realuid = -1;
    return NULL;
}
