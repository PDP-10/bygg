/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/debug.c,v 2.4 90/10/04 18:23:55 melissa Exp $";
#endif

#include <stdio.h>

/*
 * memdebug:
 * variable to control memory debugging.  It must be declared in the user 
 * program.
 * if memdebug == 1, then action is always taken.
 * if memdebug == 0, then no action is taken.
 * if memdebug == -1, then the user is asked.
 */
extern int memdebug;


#ifdef MDEBUG

char *malloc(), *realloc();
char *set_range_check(), *check_range();

#define min(x,y) ((x) < (y) ? (x) : (y))
#define RANGE	 "ABCDEFGHIJKLMNOP"
#define INTSIZE  sizeof(int)
#define LONGSIZE sizeof(long)
#define RSIZE    sizeof(RANGE)
#define RFRONT   min((RSIZE/2),LONGSIZE)
#define RBACK    min((RSIZE-RFRONT),LONGSIZE)

char *
dmalloc(size)
int size;
{
    char *cp;

    cp = malloc(size + RSIZE + INTSIZE);
    if (cp) {
	cp = set_range_check(cp, size);
	m_insert(cp);
    }
    return(cp);
}

char *
dcalloc(nelem, elsize)
int nelem, elsize;
{
    char *cp;

    cp = dmalloc(nelem * elsize);
    if (cp)
	bzero(cp, nelem * elsize);
    return(cp);
}

char *
drealloc(bp,size)
char *bp;
int size;
{
    char *cp;

    if (bp == NULL) {
	maybe_abort("Freeing NULL pointer");
    }
    else {
	m_delete(bp);
	cp = check_range(bp);
    }
    cp = realloc(cp, size + RSIZE + INTSIZE);
    if (cp) {
	cp = set_range_check(cp, size);
	m_insert(cp);
    }
    return(cp);
}

dfree(cp)
char *cp;
{
    if (cp == NULL)
	maybe_abort("Freeing NULL pointer");
    else {
	m_delete(cp);
	cp = check_range(cp);
    }
    return(free(cp));
}

char *
set_range_check(cp,size)
char *cp;
int size;
{
    register int i;
    int tmp = size;

    for(i = 0; i < INTSIZE; i++) {	/* set the size in the string */
	cp[i] = tmp & 0xff;
	tmp >>= 8;
    }
    cp += INTSIZE;			/* skip the size */

    for(i = 0; i < RFRONT; i++)		/* set the front of the range check */
	cp[i] = RANGE[i];		/* string */

    cp += RFRONT;			/* skip the front range check */

    for(i = 0; i < RBACK; i++)		/* set the back odf the range check */
	cp[i+size] = RANGE[i+RFRONT];

    return(cp);
}

char *
check_range(cp)
char *cp;
{
    register char *bp = cp - RFRONT - INTSIZE;
    char *xp = bp;
    register int i;
    int size = 0;

    for(i = 0 ; i < INTSIZE; i++) {	/* get the size out of the string */
	size <<= 8;
	size |= bp[INTSIZE-i-1] & 0xff;
    }
    bp += INTSIZE;

    for(i = 0; i < RFRONT; i++)		/* check front range check */
	if (bp[i] != RANGE[i]) {
	    maybe_abort("leftside malloc buffer overrun");
	    break;
	}
    bp += RFRONT;			/* skip front range check */

    for(i = 0; i < RBACK; i++)		/* check back rnage check */
	if (bp[i+size] != RANGE[i+RFRONT]) {
	    maybe_abort("rightside malloc buffer overrun");
	    break;
	}
    return(xp);
}


#define BUCKETS 10000
char *m_used[BUCKETS];

m_insert(cp)
register char *cp;
{
    register int i;

    for(i = 0; i < BUCKETS; i++)
	if (m_used[i] == 0) {
	    m_used[i] = cp;
	    return;
	}
}

m_delete(cp)
register char *cp;
{
    register int i;

    for(i = 0; i < BUCKETS; i++)
	if (m_used[i] == cp) {
	    m_used[i] = 0;
	    return;
	}
    maybe_abort("Freeing unmalloc'ed pointer");
}

m_init() {
    register int i;
    for(i = 0; i < BUCKETS; i++)
	m_used[i] = 0;
}

m_done() {
    register int i,j;

#ifdef undef
    for(i = 0; i < BUCKETS; i++)
	if (m_used[i] != 0) {
	    if (memdebug) {
		if (j == 0)
		    fprintf(stderr,"unfree'ed buffers, indices: ");
		fprintf(stderr,"%d, ", i);
		j++;
	    }
	}
    if (j)
	fprintf(stderr,"\n");
    if (j)
	maybe_abort("Unfree'ed malloc buffers");
#endif
}

m_checkranges() {
    int i;

    for ( i = 0; i < BUCKETS; i++)
	if (m_used[i])
	    check_range(m_used[i]);
}

#endif /* MDEBUG */

maybe_abort(str)
char *str;
{
    if (memdebug == 0)
	return;
    fprintf(stderr,"%s\n",str);
    if (memdebug == 1)
	abort();
    if (memdebug == -1)
	if (ask("Abort? "))
	    abort();
}

ask(str)
char *str;
{
    char buf[100];
    FILE *in;
    int fd;
    
    fd = dup(fileno(stdin));
    in = fdopen(fd, "r");
    while(1) {
	fprintf(stderr,str);
	fflush(stderr);
	if (fgets(buf, 99, in) == NULL)	/* EOF? */
	    return(0);
	if (buf[0] == 'n' || buf[0] == 'N') {
	    fclose(in);
	    return(0);
	}
	if (buf[0] == 'y' || buf[0] == 'Y') {
	    fclose(in);
	    return(1);
	}
	fprintf(stderr,"please answer y/n.\n");
    }
}



