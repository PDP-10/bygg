/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/mm-trans.c,v 2.1 90/10/04 18:24:55 melissa Exp $";
#endif

/*
 * transform:  Transform a mail.txt format file brought over from the
 * 20 with ftp to fix the byte counts.  Since the 20's have CR/LF and
 * the UNIX's only have LF, ftp strips out the CRs.  This messes up the
 * byte counts in mail.txt format files.  Try to figure out where the
 * message ends and fix them up.
 *
 * For now, read from argv[1] and write to stdout, like good Unix program.
 * So say "transform mail-20.txt > mail-unix.txt".
 *
 * Note that this program is not yet very robust, but it does do the job.
 * for example, it must read from a real file, or it can't get the size
 * and read the whole thing in at once.
 *
 * Add Michael A. Cooper's modifications to also convert SRI UNIX-MM
 * mail.txt files into Columbia Unix MM format.  The SRI MM uses the
 * byte count only as a guess as to where the end of message is.
 *
 * Revised at University of Chicago by Jim Lichtenstein and Sam Gassel
 * May, 1989.  Extensive rewrite of main loop, pointers for
 * backtracking added, etc.  Older MM's use nulls as part of the
 * message separators, so the original routine which dies on nulls
 * became completely unacceptable.  Now nulls are changed to blanks.
 */

#include "config.h"
#include "osfiles.h"
#include "compat.h"
#include <ctype.h>

char *index (), *rindex ();

#define FALSE (0)
#define TRUE (!FALSE)

#define hdrlen 81			/* the header's less than 80 chars */
int msgnum = 0;			        /* which message we're up to */

main (argc,argv)
int argc;
char **argv;
{
    struct stat statb;
    FILE *filep, *outfile;

    char *hdrptr;			/* what's left of the mail file */
    char *date, *txt_size, *flags, *text; /* the pieces of a message */
    char *c,*endhdr;                    /* moving utility pointer */

    int msg_size;			/* believed message size */
    char *endbuf;

    if ((argc != 2) && (argc != 3)) {
	fprintf (stderr, "usage: %s filename filename\n", argv[0]);
	exit (1);
    }

    /* Get total length of file for mallocing */
    if (stat (argv[1],&statb) != 0) {	/* we need the length */
	perror (argv[1]);
	exit(1);
    }

    /* Open it for reading */
    if ((filep = fopen (argv[1], "r")) == NULL) {
	perror (argv[1]);
	exit(1);
    }

    /*
     * Make sure the file contains some data.
     */
    if (statb.st_size == 0) {
	fprintf (stderr, "Original file is empty.\n");
	exit(1);
      }
    /* get the whole file */
    if ((hdrptr = (char *) malloc (statb.st_size +1)) == NULL) {
	fprintf (stderr, "%s: File too big for internal buffer.\n", argv[1]);
	exit(1);
    }
    hdrptr[statb.st_size] = '\0';	/* mark the ned with null */
    if (fread (hdrptr, sizeof (char), statb.st_size, filep) != statb.st_size) {
	perror (argv[1]);
	exit(1);
    }
    endbuf = hdrptr+statb.st_size;
    fclose(filep);			/* we have all the data now */

    /* change nulls to blanks */
    for(c=hdrptr; c != endbuf; c++) {
	if (*c == '\0') *c = ' ';
    }

    /*
     *  See if this mail file has been transformed already.
     *  If so, let them know
     */
    if (alreadyTransformed(hdrptr))
        fprintf(stderr,
		"File looks already transformed, transforming anyway...\n");

    /* do this after reading infile, in case infile = outfile */
    if (argc == 3) {
	if ((outfile = fopen (argv[2], "w")) == NULL) {
	    perror (argv[2]);
	    exit(1);
	}
    }
    else
	outfile = stdout;		/* be a nice unix program */

    /* The first line does have to be right */
    if (!ishead(hdrptr))
	fail(msgnum);

    while (++msgnum) {			/* do next message */

	date = hdrptr;			/* header starts here */
	txt_size = index (hdrptr, ',') + 1; /* these have to be... */
	flags = index (hdrptr, ';') + 1; /* ... in the first line */
	text = index (hdrptr, '\n') + 1; /* the line after the header */
	if (txt_size > text || flags > text ||
	    (text - 1) == NULL || (flags - 1) == NULL || 
	    (txt_size - 1) == NULL) {
	    fprintf (stderr, "Errors in header line\n");
	    fail (msgnum);
	}

	msg_size = atoi (txt_size);
	/* we want to see if we got a negative number or a non-number */
	/* a non-number gives value 0 */
	if ((msg_size < 1) && (*txt_size != '0')) { /* really zero is okay */
	    fprintf (stderr, "Illegal size value.\n");
	    fail (msgnum);
	}

	endhdr = text;

        /* msg_size counted CR and LF, so count each LF an extra time */
	for (c = text; (c < text+msg_size) && (c != endbuf); c++) {
	    if (*c == '\n')
		msg_size--;
	}

	if ((c != endbuf) && (!ishead(c))) {
	    /*
	     * header not where we expected: could be nearer or farther, so
	     * move to beginning of message and search till we find it
	     */
	    fprintf(stderr, "\
Warning: message length error in message #%d, backtracking...\n", msgnum);
	    for (c = text; !ishead(c) && c != endbuf; c++)
		for(; *c != '\n'; c++)
		    ;
	    msg_size = c - text;
	}

	/* We've reached the msg end and are ready to write it out*/
	fwrite (date, sizeof (char), txt_size - date, outfile); /* date, */
	fprintf (outfile, "%d;", msg_size); /* length of message; */
	fwrite (flags, sizeof(char), endhdr - flags, outfile); /* flags */
	fwrite(text,sizeof(char),msg_size,outfile);

	if (c == endbuf)		/* made it! */
	    exit (0);
	hdrptr = c;			/* move to start of next message*/
    }
}

/*
 * alreadyTransformed:
 *     Check to see if a mail file has already been transformed.
 *     Use length of message to see if it predicts the next
 *     header line.
 *     If so, then we suspect the file has been transformed.
 */
alreadyTransformed(mail)
char *mail;
{
    char *msize= index(mail,',') + 1;
    char *nxtmsg, *msgtxt;
    int msglen= atoi(msize);

    msgtxt = index(mail,'\n') + 1;	/* text of message itself */
    nxtmsg = msgtxt + msglen;		/* header of next message/NULL */
    if (*nxtmsg == '\0') return(FALSE);	/* Just one message */
    if (ishead(nxtmsg)) return(FALSE);	/* Found a header line */
    return(0);
}

/*
 * fail: 
 * give a nice message on failure, tell them which message messed up
 * note that all previous messages were written out successfully
 */
fail (num)
int num;
{
    fprintf (stderr, 
	     "Bad mail-txt format in header of message #%d, aborting\n", num);
    exit (1);
}

/*
 * ishead:
 * compare str against a regex looking for a header line.
 * since re_exec() ignores \n's, copy the current "line" and
 * then call re_exec().
 */
ishead(str)
char *str;
{
    char buf[BUFSIZ];
    char *p;
    static int first = TRUE;
    int st;

    if (first) {
	first = FALSE;
	/* pattern is:         22-Jan-88  3:45:11-GMT,944;000000000201 */
	if ((p = (char *) re_comp(".*-[A-Za-z][A-Za-z][A-Za-z]-.*:[0-5][0-9]:[0-5][0-9]-.*,[0-9]*;[0-9]*" )) != NULL) {
	    fprintf("Bad regex: %s\n", p);
	    fail(msgnum);
	}
    }

    p = index(str, '\n');
    if (p) {
	if ((p - str) > sizeof(buf))
	    return(FALSE);		/* too long for a header line */
	strncpy(buf, str, (p - str));
	buf[p - str] = NULL;
    }
    else
	strncpy(buf, str, sizeof(buf));

    /* Only pass likely candidates to re_exec for verification */
    if (((p = index(buf, '-')) == NULL) ||
	((p = index(p+1, '-')) == NULL) ||
	((p = index(p, ':')) == NULL) ||
	((p = index(p+1, ':')) == NULL) ||
	((p = index(p, '-')) == NULL) ||
	((p = index(p, ',')) == NULL) ||
	((p = index(p, ';')) == NULL))
	return (FALSE);

    st = re_exec(buf);

    return(st);
}
