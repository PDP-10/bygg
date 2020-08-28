/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

/*
 * program to move file a to file b, obeying file locking.
 * meant to be suid'ed or sgid'ed.   
 *
 * The point is to be able to move a user's mail file out of /usr/spool/mail, 
 * to a file of their own.
 * 
 * Must check accesses of REAL uid, to be sure that the file can be moved.
 * The user must have read access to the original, and write access to the 
 * destination directory.  The destination file must not exist.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/movemail.c,v 2.1 90/10/04 18:25:11 melissa Exp $ ";
#endif

#include "config.h"
#include "osfiles.h"
#include "compat.h"

extern int errno;

#define TRUE 1
#define FALSE 0

FILE *infile = NULL;
FILE *outfile = NULL;

main(argc, argv)
int argc;
char **argv;
{
    if (argc != 3) {
	usage(argv[0]);
	exit(1);
    }
    if (readaccess(argv[1]) &&		/* must be able to read the src */
	writeaccess_dir(argv[2]) &&	/* have write access to the directory*/
					/* of the destination file */
	!exist(argv[2]) &&		/* the destination cannot exist */
	(locked(argv[1])))		/* and the source cannot be locked */
    {
	return(!movefile(argv[1], argv[2]));	/* do it!! */
    }
    else
	return(1);
}

usage(pname) 
char *pname;
{
    fprintf(stderr,"Usage: %s srcfile destfile\n", pname);
}

/*
 * check if the real user (no suid/sgid bits considered) is readable
 */
readaccess(fname)
char *fname;
{
    if (access(fname,R_OK)) {
	perror(fname);
	return(FALSE);
    }
    return(TRUE);
}
	
writeaccess_dir(fname)
char *fname;
{
    char *cp, *rindex();
    char c;

    cp = rindex(fname,'/');
    if (cp) {
	c = *(++cp);
	*cp = '\0';
	if (access(fname,W_OK) != 0) {
	    perror(fname);
	    return(FALSE);
	}
	*cp = c;
	return(TRUE);
    }
    else if (access(".", W_OK) != 0){
	perror(fname);
	return(FALSE);
    }
    return(TRUE);
}

exist(fname)
{
    if (access(fname,F_OK) == 0) {
	fprintf(stderr,"%s: file exists\n", fname);
	return(TRUE);
    }
    return(FALSE);
}


locked(fname)
char *fname;
{
    char buf[MAXPATHLEN];

    infile = fopen(fname,"r");		/* open the input file */
    if (infile == NULL) {
	perror(fname);
	return(FALSE);
    }
    return(lock(infile, fname));	/* and lock it */
}

/* 
 * move from "from" to "to"
 */
movefile(from, to)
char *from;
char *to;
{
    int c;
    outfile = fopen(to, "w");
    if (outfile == NULL) {		/* can't open....stop */
	perror(to);
	unlock(infile,from);		/* but unlock the locked file */
	return(FALSE);
    }
    while((c = getc(infile)) != EOF)	/* copy */
	if (putc(c,outfile) == EOF) {
	    fclose(outfile);
	    unlink(outfile);
	    unlock(infile,from);
	    return(FALSE);
	}
    if (fclose(outfile) == EOF) {	/* error flushing */
	unlink(outfile);
	unlock(infile,from);
	return(FALSE);
    }
    if (unlink(from) < 0) {		/* unlink the original, */
	truncate(from,0);		/* or at least truncate it */
    }
    unlock(infile,from);		/* unlock the source. */
    return(TRUE);
}


lock(fp, fname)
FILE *fp;
char *fname;
{
#ifdef MAIL_USE_FLOCK
    if (flock(fileno(fp),LOCK_EX|LOCK_NB) < 0) {
	fprintf(stderr,"%s: already locked\n",fname);
	return(FALSE);
    }
#else
    char buf[MAXPATHLEN];
    int fd;
    sprintf(buf,"%s.lock", fname);
    if ((fd = open(buf,O_CREAT|O_EXCL|O_WRONLY, 0)) < 0) {
	if (errno == EEXIST)
	    fprintf(stderr,"%s: already locked\n",fname);
	else
	    perror(buf);
	return(FALSE);
    }
#endif
    return(TRUE);
}

unlock(fp,fname)
FILE *fp;
char *fname;
{
#ifdef MAIL_USE_FLOCK
    flock(fileno(fp),LOCK_EX|LOCK_UN);
#else
    char buf[MAXPATHLEN];
    sprintf(buf,"%s.lock", fname);
    unlink(buf);
#endif
    return(TRUE);
}
