/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/read.c,v 2.1 90/10/04 18:25:43 melissa Exp $";
#endif

/*
 * read.c - support for the mm read command
 */ 
#include "mm.h"
#include "cmds.h"
#include "parse.h"

static int mark;			/* shall we mark message as seen? */

int
cmd_read (n)
int n;
{
    if (!check_cf (O_RDWR))		/* want writable, settle for read */
	return;

    if (mode != MM_TOP_LEVEL)
	cmerr ("Previous READ command still active; type QUIT first");

    parse_sequence ((n == CMD_REVIEW) ? "" : "unseen",NULL,NULL);
    copy_sequence (cf->read_sequence, cf->sequence); /* XXX */
    check_mark();			/* can we mark things? */

    if (sequence_start (cf->read_sequence)) {
	mode |= MM_READ;
	mode &= ~MM_SEND;		/* quit from send mode */
	show_message (cf->current);
    }
    return true;
}

static int ltype;
static FILE *type_pipe = NULL;

static int
do_type (n)
int n;
{
    if (n > 0)
	if (!ignore (n)) {
	    display_message (type_pipe, &cf->msgs[n], false,
			     (ltype ? nil : only_type_headers), 
			     (ltype ? nil : dont_type_headers));
	    if (!(cf->msgs[n].flags & M_SEEN) && mark) {
		cf->flags |= MF_MODIFIED; /* file (msg flags) must be saved */
		cf->msgs[n].flags |= M_SEEN|M_MODIFIED;
	    }
	}
    return true;
}

/*
 * cmd_type:
 * TYPE command.  Parse a sequence and hand everything off to real_type.
 */


int
cmd_type (n)
int n;
{
    int real_seq;

    if (!check_cf (O_RDONLY))
	return;
    real_seq = parse_sequence ("current",NULL,NULL);
    check_mark();			/* can we mark things? */
    real_type (n, real_seq);		/* this is the real function */
}


/*
 * real_type:
 * the real type command is here.  cmd_type (above) only parses a
 * sequence.  It will call this function.  The LITERAL command which
 * defaults to LITERAL TYPE will also call this after IT does a 
 * sequence parse.
 */

int
real_type (n, real_seq)
int n;
int real_seq;
{
    int m;
    char *msg, *fmt_message();
    int len;
    FILE *out, *more_pipe_open();
    extern int use_crt_filter_always;

    ltype = (n == CMD_LITERAL);

    if (real_seq) {
	out = cmcsb._cmoj ? cmcsb._cmoj : stdout;
	if (!use_crt_filter_always)	/* have to check the total length */
	    for (len = 0, m = sequence_start (cf->sequence);
		 m && (len < cmcsb._cmrmx);
		 m = sequence_next (cf->sequence)) {
		if (cf->msgs[m].flags & M_DELETED) /* deleted message? */
		    len++;		/* one line error */
		else {
		    msg = fmt_message (&(cf->msgs[m]), 
				       (ltype ? nil : only_type_headers), 
				       (ltype ? nil : dont_type_headers));
		    if (msg != NULL) {
			len += logical_lines (msg, cmcsb._cmrmx) +1;
			free (msg);
		    }
		}
	    }

	if (use_crt_filter_always ||
	    (len >= cmcsb._cmrmx))
	    type_pipe = more_pipe_open(out);
	else
	    type_pipe = out;
	sequence_loop (do_type);
	if (type_pipe == out)		/* not a pipe */
	    fflush (type_pipe);
	else
	    more_pipe_close(type_pipe);	/* really a pipe */
	type_pipe = NULL;		/* set back to NULL so other modules */
    }					/* can use ignore() too */
    else {
	/*
	 * Can't use do_type since do_type checks for message deleted.
	 */
	if (cf->current) {
	    display_message (stdout, &cf->msgs[cf->current], false,
			     (ltype ? nil : only_type_headers),
			     (ltype ? nil : dont_type_headers));
	    if (!(cf->msgs[cf->current].flags & M_SEEN) && mark) {
		cf->flags |= MF_MODIFIED; /* file (msg flags) must be saved */
		cf->msgs[cf->current].flags |= M_MODIFIED|M_SEEN;
	    }
	}
    }
    return true;
}


int
cmd_next (cmd)
int cmd;
{
    confirm ();
    if (!check_cf (O_RDONLY))		/* no mod's needed */
	return;
    if (cmd == CMD_KILL) {
	if (!mark)
	    fprintf (stderr, 
		     "File %s \"examine\"d, not deleting message.\n",
		     cf->filename);
	else {
	    if (!(cf->msgs[cf->current].flags & M_DELETED)) {
		cf->msgs[cf->current].flags |= (M_DELETED|M_MODIFIED);
		cf->flags |= MF_MODIFIED; /* file (msg flags) must be saved */
	    }
	    if (!(mode & MM_READ))
		printf (" %d\n", cf->current);
	}
    }
    if (mode & MM_READ)
	if (sequence_next (cf->read_sequence))
	    show_message (cf->current);
	else
	    mode &= ~MM_READ;
    else
	if (cf->current < cf->count)
	    show_message (++cf->current);
	else
	    printf (" Currently at end, message %d\n", cf->current);
    return true;
}

int
cmd_previous (n)
int n;
{
    confirm ();
    if (!check_cf (O_RDONLY))		/* no modifying intended */
	return;

    if (mode & MM_READ)
	if (sequence_prev (cf->read_sequence))
	    show_message (cf->current);
	else
	    cmerr ("Already at start of sequence");
    else
	/*
	 * just step through the file
	 */
	if (cf->current > 1)
	    show_message (--cf->current);
	else
	    printf (" Currently at beginning, message %d\n", cf->current);
	
    return true;
}

int
cmd_count (cmd)
int cmd;
{
    if (!check_cf(O_RDONLY))		/* not wanting to modify */
	return;

    if (parse_sequence ("all",NULL,NULL)) {
	int count,n;
	for (count = 0, n = sequence_start (cf->sequence); n;
	     n = sequence_next (cf->sequence))
	    ++count;
	seq_print (false);
	printf ("%s %d message%s\n", count > 0 ? " =" : "",
		count, count != 1 ? "s" : "");
    } else 
	cmerr ("Command not valid in this context");
}

int
cmd_mark (cmd)
int cmd;
{
    int real_seq;

    if (!check_cf (O_RDONLY))		/* find out for ourselves */
	return;
    real_seq = parse_sequence ("current",NULL,NULL);
    check_mark();
    if (!mark)				/* can't do anything */
	return;

    if (!real_seq)
	change_flag (cf->current, cmd);
    else {
	int n;
	for (n = sequence_start (cf->sequence); n;
	     n = sequence_next (cf->sequence))
	    change_flag (n, cmd);
	seq_print (true);
    }
}

change_flag (n, what)
int n, what;
{
    message *m = & cf->msgs[n];

    switch (what) {
      case CMD_DELETE:
	if (!(m->flags& M_DELETED)) {
	    m->flags |= (M_DELETED|M_MODIFIED);
	    cf->flags |= MF_MODIFIED;
	}
	break;
      case CMD_UNDELETE:
	if (m->flags & M_DELETED) {
	    m->flags &= ~M_DELETED;
	    m->flags |= M_MODIFIED;
	    cf->flags |= MF_MODIFIED;
	}
	break;
      case CMD_UNMARK:
	if (m->flags & M_SEEN) {
	    m->flags |= M_MODIFIED;
	    m->flags &= ~M_SEEN;
	    cf->flags |= MF_MODIFIED;
	}
	break;
      case CMD_MARK:
	if (!(m->flags & M_SEEN)) {
	    m->flags |= (M_SEEN|M_MODIFIED);
	    cf->flags |= MF_MODIFIED;
	}
	break;
      case CMD_FLAG:
	if (!(m->flags & M_FLAGGED)) {
	    m->flags |= (M_FLAGGED|M_MODIFIED);
	    cf->flags |= MF_MODIFIED;
	}
	break;
      case CMD_UNFLAG:
	if (m->flags & M_FLAGGED) {
	    m->flags &= ~M_FLAGGED;
	    m->flags |= M_MODIFIED;
	    cf->flags |= MF_MODIFIED;
	}
	break;
      case CMD_UNANSWER:
	if (m->flags & M_ANSWERED) {
	    m->flags &= ~M_ANSWERED;
	    m->flags |= M_MODIFIED;
	    cf->flags |= MF_MODIFIED;
	}
	break;
    }
}

int
cmd_jump (n)
int n;
{
    if (!check_cf(O_RDONLY))
	return;
    noise ("to message number");
    n = parse_number (10, "message number", nil);
    if (n < 1 || n > cf->count)
	cmerr("Number out of range");
    confirm();
    if (mode & MM_READ)
	if (!in_sequence(cf->sequence, n))
	    cmerr ("Number not in current message sequence");
    cf->current = n;
}

/*
 * decide whether to ignore a message, and tell the user why.
 */
ignore (n)
int n;					/* message number in current file */
{
    if ((mode == MM_TOP_LEVEL) && (cf->msgs[n].flags & M_DELETED)) {
	fprintf (type_pipe ? type_pipe : stdout, 
		 " Message %d deleted, ignored.\n", n);
	return true;
    }
    return false;
}

/*
 * show_message is used by read, next, previous.
 */

static
show_message (n)
int n;
{
    message *m = &cf->msgs[n];

    if (m->flags & M_DELETED)
	printf (" Message %d deleted, ignored.\n", n);
    else {
	display_message (stdout, m, true,
			 only_type_headers, dont_type_headers);
	if (!(m->flags & M_SEEN) && mark) {
	    cf->flags |= MF_MODIFIED;	/* file (msg flags) must be saved */
	    m->flags |= (M_SEEN|M_MODIFIED);
	}
    }
}

/*
 * check_mark:
 * find out if we can mark the messages in the sequence, and set "mark"
 */
check_mark()
{
    if (!(cf->flags & MF_RDONLY))
	mark = TRUE;			/* go ahead! */
    else {				/* read-only */
	if (modify_read_only == SET_NEVER) {
	    fprintf (stderr,
	   "Warning: cannot mark messages in file visited with \"examine\"\n");
	    mark = FALSE;		/* don't bother trying */
	}
	else if (modify_read_only == SET_ASK)
	    mark=yesno("File is read-only, mark messages anyway? ", "no");
	else			/* SET_ALWAYS */
	    mark = TRUE;
    }
}
