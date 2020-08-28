/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/headers.c,v 2.1 90/10/04 18:24:24 melissa Exp $";
#endif

/*
 * headers.c - header manipulation stuff
 *
 */

#include "mm.h"
#include "parse.h"

/*
 * compare two strings, ignoring case.  returns true if the strings match
 * or if s1 is a leading substring of s2 and that substring in s2 ends with 
 * a ':'.  Note, s1 may or may not be colon terminated, so check both cases.
 */

hdrcmp (s1, s2)
register char *s1, *s2;
{
    register char c1, c2;

    while (1) {
	c1 = *s1++;
	c2 = *s2++;
	if (c2 == '\0') return false;	/* header line shorter than name */
	if (c1 == '\0' && c2 == ':') return true; /* a match w/o ':' */
	if (c1 == ':' && c2 == ':') return true; /* a match w/ ':'  */
	if (((isascii (c1) && isupper (c1)) ? tolower (c1) : c1) !=
	    ((isascii (c2) && isupper (c2)) ? tolower (c2) : c2))
	    return false;
    }
}


/*
 * return a pointer to the first char following a (possibly folded) line.
 */
char *
skipheader (s)
register char *s;
{
    for (;;) {
	while (*s && (*s != '\n'))
	    ++s;
	if (*s == '\0')
	    return s;
	/* s now points to a linefeed */
	++s;
	if (isblank (*s))
	    continue;
	return s;
    }
}

char *
next_header(s,next) 
register char **s;
int *next;
{
    char *s1, *s2;
    int len;
    
    s1 = skipheader(*s);
    if (s1) {
	len = s1 - *s;
	s2 = malloc(len+1);
	bcopy(*s,s2,len);
	s2[len] = '\0';
	*s = s1;
    }
    else 
	return(NULL);

    if (*s1 == '\n') {
	*next = false;
    }
    else 
	*next = true;
    return(s2);
}

/*
 * htext:
 * Return the body of the first header "h" in string at "s".  The return
 * value is allocated with malloc and should be freed when no longer needed.
 */
char *
htext(h, s)
char *h, *s;
{
    char *findheader();
    return (findheader (h, s, true));	/* malloc the text string */
}

/*
 * hfind:
 * Return a pointer to the first header "h" in message text "s".  The
 * return value is a pointer right into the message text.
 */
char *
hfind(h, s)
char *h, *s;
{
    return (findheader (h, s, false));	/* don't malloc it */
}

/*
 * findheader:
 * do the real work of htext and hfind
 * Don't call this routine, okay?  Just call one of those up there ^^^.
 */
char *
findheader(h, s, mallocp)
char *h, *s;
int mallocp;				/* should we malloc up the space? */
{
    int size;
    char *head;
    int len;

    while (1) {
	if (s == nil || *s == '\0' || *s == '\n')
	    return nil;
	if (hdrcmp (h, s)) {
	    if (!mallocp)
		return (s);		/* here it is! */
	    len = strlen(h);
	    if (h[len-1] == ':')
		s += len;
	    else
		s += len+1;
	    for (;;) {
		switch (*s) {		/* skip past whitespace, ':' etc */
		  case 0:
		    return nil;
		  case ' ':
		  case '\t':
		    ++s;
		    break;
		  case '\r':
		  case '\n':
		    ++s;
		    if (!isblank(*s))
		      return(nil);
		    break;
		  default:
		    /* s now points to the beginning of the header text,
		       so find the end and copy it */
		    size = (int) (skipheader (s) - s - 1); /* -1 for lf */
		    head = malloc (size + 1);
		    if (head == nil)
			return nil;
		    strncpy(head, s, size);
		    head[size] = '\0'; 	/* make sure it's null-terminated */
		    return head;
		}
	    }
	}
	else
	    s = skipheader (s);
    }
}    

/*
 * strip out multiple spaces and folded lines in place.  any substring that
 * can be interpreted as a run of whitespace (tabs, spaces, or line-folding
 * sequences) is collapsed to a single space.
 */
char *
stripspaces(s)
register char *s;
{
    register char *cp = s, *p = s;

    while (*cp) {
	if (isascii(*cp) && isspace(*cp)) {
	    if (cp != s)
		*p++ = ' ';
	    while (isascii(*cp) && isspace(*++cp))
		;
	}
	else
	    *p++ = *cp++;
    }
    *p++ = '\0';
    return s;
}

/*
 * header_summary:
 * print out a little summary of this message, with the author and subject
 * assume we have 79 columns to print:
NFADK123) 21-Aug Melissa Metz    12345678901234567890123456789 (10000000 chars)
 */
#define BIGNUM 10000000			/* 8 characters worth */
#define BIGNAME "10M"			/* a short string to name that */
int
header_summary (n, fp, longfmt)
int n, longfmt;
FILE *fp;
{
    buffer line;
    message *m;
    char *cp = line;
    char *p;
    char *freeme;			/* somebody forgot to free this! */
    char *angle;			/* location of angle bracket */

    if (fp == NULL)			/* print to stdout if output */
	fp = stdout;			/* descriptor is NULL */

    m = &cf->msgs[n];

    if (!(m->flags & M_SEEN))
	if (m->flags & M_RECENT)
	    *cp++ = 'N';
	else
	    *cp++ = 'U';
    else
	if (m->flags & M_RECENT)
	    *cp++ = 'R';
	else
	    *cp++ = ' ';
    *cp++ = (m->flags & M_FLAGGED) ? 'F' : ' ';
    *cp++ = (m->flags & M_ANSWERED) ? 'A' : ' ';
    *cp++ = (m->flags & M_DELETED) ? 'D' : ' ';
    *cp++ = (m->keywords) ? 'K' : ' ';
    (void) sprintf (cp, "%3d) %s ", n, hdate (m->date));
    cp += strlen (cp);
    freeme = htext ("from", m->text);
    if (freeme) {
	char *b, *z;
	if (angle = index(freeme, '<')) {
	    if (angle != freeme) /* some text before <foo>? */
		*angle = '\0';		/* print "foo" not "foo <foo>" */
	    sprintf (cp, "%-15.15s ", freeme); /* XXX should be smarter */
	}
	else if ((b = index(freeme, '(')) && (z = index(b + 1, ')'))) {
	    ++b;
	    *z = 0;
	    sprintf (cp, "%-15.15s ", b);
	}
	else
	    sprintf (cp, "%-15.15s ", freeme);

	safe_free (freeme);
    }
    else
	sprintf (cp, "%-15.15s ", "???");

    cp += strlen (cp);

    freeme = htext ("subject", m->text);
    if (longfmt) {
	/* use the rest of the line for the Subject: header */
	if (freeme)
	    sprintf (cp, "%.*s", cmcsb._cmcmx - (cp - line), freeme);
	else
	    sprintf (cp, "(No subject)");
    }
    else {
	if (freeme) {
	    sprintf (cp, "%.*s", cmcsb._cmcmx - 1 - 50, freeme); /* XXX */
	    cp += strlen (cp);
	}
	if (m->size > BIGNUM)
	    sprintf (cp, " (>%s chars)", BIGNAME);
	else
	    sprintf (cp, " (%ld chars)", m->size);
	cp += strlen (cp);
    }
    safe_free (freeme);
    for (p = line; p && *p != NULL; p++) /* convert newlines to spaces */
	if (*p == '\n')			/* to make sure things stay on one */
	    *p = ' ';			/* line.  could be done more */
    fprintf (fp, "%s\n", line);		/* elegantly... */
}

/*
 * search:
 * find string "s" in string "t", return pointer to where it starts, or nil
 */
char *
search (s, t)
char *s, *t;
{
    register char c = s[0];

    if (isupper (c))
	c = tolower (c);

    do {
	while (*t && (c != (isupper (*t) ? tolower (*t) : *t))) /* XXX ugh */
	    ++t;
	if (*t && !ustrncmp(s, t, strlen(s)))
		return t;
    } while (*t++);
    return nil;
}

/*
 * is the string "s" in the "h" header of this message (m)?
 * returns a boolean (which only superficially resembles a pointer)
 */
search_header (h, s, m)
char *h, *s;
message *m;
{
    int q;
    char *p = htext (h, m->text);
    if (p == nil)
	return (nil);
    q = (int) search (s, p);
    safe_free (p);
    return (q);
}

/*
 * is the string "s" anywhere (including the headers) in message m?  
 * this too is a boolean
 */
search_text (s, m)
char *s;
message *m;
{
    return ((int) search (s, m->text));
}
