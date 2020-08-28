/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *help_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/help.h,v 2.1 90/10/04 18:24:29 melissa Exp $";
#endif
#endif /* RCSID */

/* help file support */

#ifndef FALSE
#define FALSE (0)
#define TRUE (!FALSE)
#endif /* FALSE */

#define DEF_SRCFILE "./help.cpp"	/* defaults in case not in */
#define DEF_HLPFILE "./mm.help"		/* arguments */

#define HELPLEVEL 5			/* max number of strings per cmd */
#define HELP_TOP 0
#define HELP_READ 1
#define HELP_SEND 2
#define HELP_TOPIC 3
#define HELP_VARS 4
#define HELP_ALL HELP_TOP HELP_READ HELP_SEND

#define INTERLEN 3			/* XXX length of "@@\n" */

typedef struct hlp_offset {
    long offset;
    int length;
} hlp_offset;
