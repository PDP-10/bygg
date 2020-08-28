/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/print.c,v 2.1 90/10/04 18:25:35 melissa Exp $";
#endif

#include "mm.h"
#include "parse.h"
#include "cmds.h"

#define NULLSWIT 0
#define HEADERS_ONLY 1
#define SEPARATE_PAGES 2


/*
 * cmd_list:
 * list messages to file or pipe.
 * a combination of the MM-20 LIST and FILE-LIST commands.
 */

int
cmd_list (n)
int n;
{
    int sw = NULLSWIT;
    static char *listdest = NULL;
    char *parse_listfile();
    FILE *od, *mm_popen(), *fopen();
    int pipep;
    int last_message;
    message *m;
    char *msgtxt, *fmt_message();
    keylist only, dont;
    int seq;

    if (listdest != NULL) {		/* catch the memory leak */
	free (listdest);
	listdest = NULL;
    }

    if (!check_cf(O_RDONLY))		/* have to have a current file */
	return;
    noise ("to");
    listdest = parse_listfile(&sw);	/* get destination file/pipe */
    seq = parse_sequence("current",NULL,NULL); /* get a sequence */

    /*
     * NOTE:
     * If seq is TRUE (we parsed a sequence from the top level)
     * then we want to check that there is something in the sequence,
     * so we call sequence_start.
     * If seq is FALSE we want to print the current message and DO NOT
     * want to call sequence_start or sequence_next down in the while().
     */
    if (!seq || sequence_start(cf->sequence)) {	/* do we have a sequence? */
	last_message = sequence_last (cf->sequence); /* remember last msg */

	if (listdest == NULL) {		/* /print */
	    pipep = true;
	    if (strlen(print_filter) == 0)
		cmerr ("no print filter - use SET PRINT-FILTER to define one");
	    od = mm_popen (print_filter, "w");
	}
	else if (listdest[0] == '|' && listdest[1] == ' ') { /* pipe */
	    pipep = true;
	    od = mm_popen (listdest+2, "w"); /* open the pipe */
	}
	else {
	    pipep = false;
	    od = fopen (listdest, "w");	/* open the output file */
	}

	if (od == NULL) {		/* do we have an output descriptor */
	    if (pipep)
		cmxeprintf ("?could not start process %s\n", listdest+2);
	    else
		perror (listdest);
	    free (listdest);		/* clean up */
	    listdest = NULL;
	    return false;
	}

	/* title (filename and date of listing) */
	fprintf (od, "-- Messages from file: %s --\n", cf->filename);
	fprintf (od, "   %s\n\n", daytime(0));

	if ((sw == HEADERS_ONLY) || list_include_headers) {
	    do {			/* pass one for headers */
		header_summary (cf->current, od); /* ta da! */
	    } while (seq && sequence_next (cf->sequence));
	}

	if (sw != HEADERS_ONLY) {	/* message bodies too */
	    dont = (n == CMD_LITERAL) ? nil : dont_print_headers;
	    only = (n == CMD_LITERAL) ? nil : only_print_headers;
	    if (seq)
	        sequence_start (cf->sequence); /* same sequence */
	    fputc ('\n', od);		/* separate headers from messages */
	    fputc ('\f', od);		/* with a formfeed */
	    fputc ('\n', od);
	    do {			/* pass 2 for message body */
		m = &cf->msgs[cf->current];
		fprintf(od, "Message %d -- *********************\n",
			cf->current);
		if ((msgtxt = fmt_message(m, only, dont)) == NULL)
		    fputc ('\n', od);
		else {
		    fwrite (msgtxt, sizeof(char), strlen(msgtxt), od);
		    fputc('\n', od);
		    free (msgtxt);
		}
		if (((sw == SEPARATE_PAGES) || list_on_separate_pages)
		    && (cf->current != last_message)) {
		    fputc ('\f', od);
		    fputc ('\n', od);
		}
	    } while (seq && sequence_next (cf->sequence));
	}
	
	if (pipep)			/* close up */
	    mm_pclose (od);
	else
	    fclose (od);
	if (mode == MM_TOP_LEVEL)	/* print that sequence so they */
	    seq_print (true);		/* know we did something */

    }

    if (listdest != NULL) {		/* clean up */
	free (listdest);
	listdest = NULL;
    }
    return true;			/* done! */
}


/*
 * parse_listfile:
 * parse for optional switches followed by a filename or '|' and command
 * on return, the return value is either a filename or a command starting
 * with "| ".
 * Or, if it returns NULL, then the /print switch was parsed instead of
 * a filename or command.
 * sw may also be set.
 * Note:
 * caller is responsible of freeing the return value when done.
 */

char *
parse_listfile (sw)
int *sw;				/* switch parsed if any */
{
    static keywrd swkeys[] = {
	{ "headers-only", 0, HEADERS_ONLY },
	{ "separate-pages", 0, SEPARATE_PAGES }
    };
    static keytab swtab = { sizeof(swkeys) / sizeof (keywrd), swkeys };
    static fdb switches = { _CMSWI, 0, NULL, (pdat) &swtab, NULL, NULL, NULL };
    static keywrd prkey[] = {
	{ "print", 0, 0 }
    };
    static keytab prtab = { sizeof(prkey) /sizeof (keywrd), prkey };
    static fdb prfdb = { _CMSWI, 0, NULL, (pdat) &prtab, NULL, NULL, NULL };
    static fdb filnam = { _CMFIL, CM_SDH|FIL_PO|FIL_VAL|FIL_NODIR, NULL, NULL, 
			    "file to list to", NULL, NULL };
    static fdb pipe = { _CMTOK, CM_SDH, NULL, (pdat) "|", 
			  "\"|\" followed by a quoted command line", 
			  NULL, NULL };
    static fdb comm = { _CMQST, CM_SDH, NULL, NULL, "quoted command line",
			  NULL, NULL };
    pval parseval;
    fdb *used;
    char *ret;

    *sw = NULLSWIT;
#ifdef undef
    pipe._cmdef = list_destination;	/* XXX add this var sometime */
#endif /* undef */
    parse (fdbchn(&switches, &prfdb, &pipe, &filnam, NULL), &parseval, &used);
    if (used == &switches) {
	*sw = parseval._pvint;
	parse (fdbchn(&prfdb, &pipe, &filnam, NULL), &parseval, &used);
    }

    if (used == &prfdb) {
	return (NULL);
    }
    else if (used == &filnam) {
	if (*parseval._pvfil[0] == '\0')
	    cmerr ("destination for output not specified");
	ret = (char *) malloc (strlen(parseval._pvfil[0])+1);
	strcpy (ret, parseval._pvfil[0]);
	return (ret);
    }
    else {				/* if (used == &pipe)  */
	parse (&comm, &parseval, &used);
	ret = (char *) malloc (strlen(parseval._pvstr)+3);
	sprintf (ret, "| %s", parseval._pvstr);
	return (ret);
    }
}


/*
 * cmd_print:
 */

int
cmd_print (n)
int n;
{
    static keywrd swkeys[] = {
	{ "separate-pages", 0, SEPARATE_PAGES }
    };
    static keytab swtab = { sizeof(swkeys) / sizeof (keywrd), swkeys };
    static fdb switches = { _CMSWI, 0, NULL, (pdat) &swtab, NULL, NULL, NULL };
    fdb *u;
    FILE *pp, *mm_popen();
    message *m;
    char *msgtxt, *fmt_message();
    keylist only, dont;
    int seq;
    int last_message;
    int sw = NULLSWIT;

    if (!check_cf(O_RDONLY))		/* need a file of messages */
	return;

    seq = parse_sequence("current", fdbchn(&switches,NULL), &u);
    if (u != NULL) {
	sw = pv._pvint;
	seq = parse_sequence("current", NULL, NULL);
    }


    /*
     * NOTE:  See NOTE in cmd_list
     */
    if (!seq || sequence_start(cf->sequence)) {	/* got a sequence? */
	last_message = sequence_last (cf->sequence); /* remember last msg */

	if (strlen(print_filter) == 0)
	    cmerr ("no print filter - use SET PRINT-FILTER to define one");
	if ((pp = mm_popen (print_filter, "w")) == NULL) {
	    cmxeprintf ("?could not start process %s\n", print_filter);
	    return false;
	}
	
	dont = (n == CMD_LITERAL) ? nil : dont_print_headers;
	only = (n == CMD_LITERAL) ? nil : only_print_headers;
	do {
	    m = &cf->msgs[cf->current];
	    if ((msgtxt = fmt_message(m, only, dont)) == NULL)
		fputc ('\n', pp);
	    else {
		fwrite(msgtxt, sizeof(char), strlen(msgtxt), pp);
		fputc('\n', pp);
		free (msgtxt);
	    }
	    if (((sw == SEPARATE_PAGES) || list_on_separate_pages)
		&& (cf->current != last_message)) {
		fputc ('\f', pp);
		fputc ('\n', pp);
	    }
	} while (seq && sequence_next (cf->sequence));
	
	mm_pclose (pp);
	if (mode == MM_TOP_LEVEL)
	    seq_print (true);
    }
    return true;
}


/*
 * cmd_literal:
 * parse for type/print/list (default type) and call appropriate command
 */

cmd_literal(n)
int n;
{
    static keywrd litkeys[] = {
	{ "list", 0, CMD_LIST },
	{ "print", 0, CMD_PRINT },
	{ "type", 0, CMD_TYPE }
    };
    static keytab littab = { sizeof(litkeys) / sizeof (keywrd), litkeys };
    static fdb litfdb = { _CMKEY, 0, NULL, (pdat) &littab, NULL,
			      "type", NULL };
    int seq;
    fdb *u;

    if (!check_cf (O_RDONLY))		/* since we call parse_sequence */
	return;
    seq = parse_sequence ("current", fdbchn(&litfdb,NULL), &u);
    if (u != NULL) {			/* parsed a command */
	(void) (*mm_cmds[pv._pvkey]) (CMD_LITERAL);
    }
    else {				/* got a sequence */
	check_mark();			/* default TYPE */
	real_type (CMD_LITERAL, seq);
    }
}
