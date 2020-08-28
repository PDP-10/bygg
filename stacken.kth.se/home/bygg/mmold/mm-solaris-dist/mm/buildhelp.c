/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/buildhelp.c,v 2.1 90/10/04 18:23:36 melissa Exp $";
#endif

/*
 * buildhelp:
 * read in help source file and build an indexed help file for use
 * by mm.
 */

#include <stdio.h>
#include "config.h"
#include "osfiles.h"
#include "compat.h"
#include "help.h"

long str_start;				/* where the help strings start */
hlp_offset *offsets;			/* indexes into file, strlens */
int numtopics;				/* number of commands */

char buf[BUFSIZ];

/*
 * main:
 */

main (argc, argv)
int argc;
char **argv;
{
    char *infile, *outfile;
    FILE *INF, *OUTF;

    switch (argc) {
    case 1:
	infile = DEF_SRCFILE;
	outfile = DEF_HLPFILE;
	break;
    case 2:
	infile = argv[1];
	outfile = DEF_HLPFILE;
	break;
    case 3:
	infile = argv[1];
	outfile = argv[2];
	break;
    default:
	fprintf (stderr, "usage: %s [ source [ destination ] ]\n", argv[0]);
	exit (1);
    }
    printf ("%s -> %s\n", infile, outfile);

    if ((INF = fopen (infile, "r")) == NULL) {
	perror (infile);
	exit(1);
    }
    if ((OUTF = fopen (outfile, "w")) == NULL) {
	perror (outfile);
	exit(1);
    }

    get_indexes(INF);
    write_indexes(OUTF);
    write_strings(INF,OUTF);
    fclose (INF);
    fclose(OUTF);
    exit(0);
}

/*
 * get_indexes:
 * snarf in the file and find out the lengths, plus what the offsets will be
 * for all the helpstrings
 */
get_indexes (in)
FILE *in;
{
    long cur_pos;
    int cmd_xxx, len, eostring, i;
    int l[3];
    int n;

    do {				/* assume we get the whole line */
	if (fgets (buf, BUFSIZ, in) == NULL) {
	    fprintf (stderr, "Premature end of input file\n");
	    exit(1);
	}
    } while (strncmp (buf, "@@@@", 4) != 0); /* start of data */

    if (sscanf (buf, "@@@@%d", &numtopics) != 1) {
	fprintf (stderr,"Bad format in source file -- can't find numtopics\n");
	exit(1);
    }
    if ((offsets = (hlp_offset *) 
	 malloc(numtopics*HELPLEVEL*sizeof(hlp_offset))) == NULL) {
	fprintf (stderr, "No room for index array!\n");
	exit(1);
    }

    for (i = 0; i < numtopics*HELPLEVEL; i++)
	offsets[i].offset = -1;		/* signal no string yet */

    cur_pos = 0;			/* no strings seen yet */

    do {				/* find first string start */
	if (fgets (buf, BUFSIZ, in) == NULL) {
	    fprintf (stderr, "No strings found, aborting\n");
	    exit(1);
	}
    } while (strncmp (buf, "@@", 2) != 0);
    str_start = ftell (in);		/* remember for pass 2 */

    /* now buf has the header line of a string */
    while (strncmp (buf, "@@@@", 4) != 0) { /* till no more strings */
	n=sscanf (buf, "@@%d %d %d %d", &cmd_xxx, &l[0], &l[1], &l[2]);
	if (n < 2) {
	    fprintf (stderr, "Bad header format %s", buf);
	    exit(1);
	}
	for (i = 0; i < n-1; i++)
	    offsets[cmd_xxx*HELPLEVEL+l[i]].offset = cur_pos;

	len = 0;
	eostring = FALSE;
	do {
	    if (fgets (buf, BUFSIZ, in) == NULL) {
		fprintf (stderr, "End of file in help string, aborting.\n");
		exit(1);
	    }
	    if (strncmp (buf, "@@", 2) == 0) /* end of string */
		eostring = TRUE;
	    else
		len += strlen (buf);
	} while (!eostring);
	for (i = 0; i < n-1; i++)
	    offsets[cmd_xxx*HELPLEVEL+l[i]].length = len;
	cur_pos += len + INTERLEN;	/* string plus interstring chars */
    }
    return;
}

/*
 * write_indexes:
 * write all the indexes out in a file
 */
write_indexes (out)
FILE *out;
{
    int i, n;

    fprintf (out, "%d,%d\n", numtopics, HELPLEVEL); /* for sanity checks */

    n = numtopics*HELPLEVEL;
    for (i = 0; i < n; i++)
	fprintf (out, "%d,%d\n", offsets[i].offset, offsets[i].length);
}

/*
 * write_strings:
 * snarf the strings from the input to the output
 */
write_strings (in, out)
FILE *in, *out;
{
    int eostring;

    fseek (in, str_start, SEEK_SET);	/* get to start of strings */
    strcpy (buf, "@@");			/* what we're just past */
    fprintf (out, "@@\n");

    /* ready to read the first line of a string */
    while (strncmp (buf, "@@@@", 4) != 0) { /* till no more strings */
	eostring = FALSE;
	do {
	    if (fgets (buf, BUFSIZ, in) == NULL) {
		fprintf (stderr, 
			 "End of file writing help strings, aborting.\n");
		exit(1);
	    }
	    if (strncmp (buf, "@@", 2) == 0) /* end of string */
		eostring = TRUE;
	    else			/* part of string, print it */
		fwrite (buf, sizeof (char), strlen (buf), out);
	} while (!eostring);
	fprintf (out, "@@\n");		/* inter string marker */
    }
    return;
}
