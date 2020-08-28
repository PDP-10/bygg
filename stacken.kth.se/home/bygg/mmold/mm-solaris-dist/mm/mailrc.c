/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/mailrc.c,v 2.2 90/10/04 18:24:39 melissa Exp $";
#endif

#include "mm.h"
#include "address.h"

mailrc()
{
    FILE *fp;
    char fname[BUFSIZ];

    sprintf(fname, "%s/.mailrc", HOME);
    if  ((fp = fopen(fname, "r")) == NULL)
	return;
    rdmailrc(fp);
    fclose(fp);
}
    
rdmailrc(fp) 
FILE *fp;
{
    char line[BUFSIZ];
    char alias[40];
    addresslist al;
    char *cp, *cp1, *gettoken(), *egets();
    addresslist *a, *lookup_alias();

    while(egets(line, BUFSIZ-1, fp) != NULL) {
	al.first = al.last = nil;
	line[BUFSIZ-1] = '\0';
	cp = line;

	cp1 = gettoken(&cp);
	if (cp1 == nil)
	    continue;
	if (ustrncmp(cp1,"alias",5))
	    continue;

	cp1 = gettoken(&cp);
	if (cp1 == nil)
	    continue;
	strcpy(alias, cp1);
	while(cp1 = gettoken(&cp)) {
	    if (a = lookup_alias(cp1))
		merge_addresses(&al, a);
	    else
		add_addresslist(&al, cp1, ADR_ADDRESS);
	}
	if (al.first)
	    set_alias(safe_strcpy(alias), &al, MA_MAILRC);
    }
}


/*
 * get a "token" -- a "word" or a quoted string
 */

char *
gettoken(cp) 
char **cp;
{
    static char buf[BUFSIZ];
    char *cp1;

    for(cp1 = *cp; isspace(*cp1) && *cp1 != '\0'; cp1++);
    *cp = cp1;
    if (*cp1 == '"') {			/* quoted string */
	for(cp1 = *cp+1; *cp1 != '"' && *cp1 != '\0'; cp1++)
	    if ((*cp1 == '\\') && *(cp1+1) != '\0')
		cp1++;			/* ignore " if it's next */
	cp1++;				/* include " */
    }
    else
	for(cp1 = *cp; !isspace(*cp1) && *cp1 != '\0'; cp1++);
    if (*cp == cp1)
	return(nil);
    strncpy(buf,*cp, cp1- *cp);
    buf[cp1- *cp] = '\0';
    *cp = cp1;
    return(buf);
}

/*
 * egets:
 * like fgets, but if line ends in "\", get next line
 */

char *
egets (s, n, stream)
char *s;
int n;					/* max length */
FILE *stream;
{
    int left, len;
    char *str;

    left = n; str = s;			/* haven't read anything yet */

    while (true) {
	if (fgets (str, left, stream) == NULL)
	    return (NULL);		/* -sigh- */
	len = strlen (str);
	if ((len < 2) ||		/* didn't get backslash-lf */
	    (str[len - 1] != '\n') ||	/* no lf -- out of space so done */
	    (str[len - 2] != '\\'))	/* not continued */
	    return (s);
	str[len - 2] = ' ';		/* translate backslash-lf to space */
	str[len - 1] = '\0';		/* just in case... */
	str = &(str[len-1]);		/* continue after the space */
    }
}
