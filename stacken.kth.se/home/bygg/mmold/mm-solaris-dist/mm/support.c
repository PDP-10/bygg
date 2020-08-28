/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/support.c,v 2.1 90/10/04 18:26:48 melissa Exp $";
#endif

#include "mm.h"
#include "ccmd.h"

/*
 * safe_strcat:
 * do a strcat, but malloc up the required space first.
 * possibly put a space in before the string, depending on context.
 * Note: returns dest if src is NULL.
 */

char *malloc(), *realloc();

char *
safe_strcat(dest,src,usespace) 
char *dest, *src;
int usespace;
{
    int space=FALSE;
    char c1,c2;


    if (src == NULL)
        return (dest);
    if (dest) {
	if (usespace) {
	    c1 = dest[strlen(dest)-1];
	    c2 = src[0];
	    if (isalnum(c1)) 
		space = TRUE;
	    if (c2 == '@' || c2 == '>' || c2 == ')')
		space = FALSE;
	    if (c1 == '>' || c1 == ')' || c1 == '\"')
		space = TRUE;
	}
	dest = realloc(dest,strlen(dest)+strlen(src)+1+(space?1:0));
	if (dest == nil) {
	    cmxprintf("?Out of memory");
	    ccmd_errnp(CMxOK);
	}
	if (space) 
	    strcat(dest," ");
	strcat(dest,src);
    }
    else {
	dest = malloc(strlen(src)+1);
	if (dest == nil) {
	    cmxprintf("?Out of memory");
	    ccmd_errnp(CMxOK);
	}
	strcpy(dest,src);
    }
    return(dest);
}



char *
safe_realloc(cp, size)
char *cp;
int size;
{
    char *realloc(), *malloc();

    if (cp)
	return(realloc(cp,size));
    else
	return(malloc(size));
}


/*
 * safe_strcpy:
 * copy str into a newly malloc'ed buffer
 * Note: when str is NULL, returns NULL
 */

char *
safe_strcpy(str) 
char *str;
{
    char *x;
    
    if (str == NULL)
        return (NULL);
    x = malloc(strlen(str)+1);
    if (!x)
	panic ("Out of memory");
    strcpy(x,str);
    return(x);
}



#define min(x,y) (((x) < (y)) ? (x) : (y))


/*
 * safe_strncat:
 * Note: returns dest if src is NULL
 */

char *
safe_strncat(dest,src,n) 
char *src,*dest;
int n;
{
    char *x;
    int srclen, destlen;

    if (src == NULL)
        return(dest);
    srclen = min(strlen(src),n);

    if (dest)
	destlen = strlen(dest);
    else 
	destlen = 0;
	    
    if (dest) 
	x = realloc(dest,srclen + destlen + 1);
    else
	x = malloc(srclen + 1);
    if (!x)
	panic ("Out of memory");
    strncat(x,src,srclen);
    return(x);
}


char *
safe_free(str) char *str; {
    if (str) free(str);
    return((char *)NULL);
}

