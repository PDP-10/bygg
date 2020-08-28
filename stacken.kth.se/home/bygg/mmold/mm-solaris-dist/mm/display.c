/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/display.c,v 2.1 90/10/04 18:23:57 melissa Exp $";
#endif

/*
 * display.c - support for the mm display command
 */

#include "mm.h"
#include "cmds.h"
#include "message.h"
#include "parse.h"

FILE *more_pipe_open();
extern mail_msg *current;

static keywrd disp_swits[] = {
    { "expand", 0, (keyval) CMD_EXPAND },
};

static keytab disp_tab = { (sizeof(disp_swits)/sizeof(keywrd)), disp_swits };
static fdb disp_swi_fdb = { _CMSWI, 0, NULL, (pdat) &disp_tab, NULL, "/expand",
				NULL };

cmd_display(n)
int n;
{
    pval pv;
    fdb *used;
    int which;
    int expand;
    FILE *pipe;
    extern int use_crt_filter_always;

    noise("message field");
    parse(fdbchn(&hdr_cmd_fdb, &disp_cmd_fdb, &disp_swi_fdb, nil),&pv, &used);
    which = (int) pv._pvkey;
    expand = FALSE;
    if (which == CMD_EXPAND) {
	expand = TRUE;
	parse(fdbchn(&hdr_cmd_fdb, &disp_cmd_fdb, nil),&pv, &used);
	which = (int) pv._pvkey;
    }
    confirm();
    pipe = stdout;			/* default for output */
    switch(which) {
    case CMD_HEADER:
	display_header(pipe,current,expand, FALSE);
	break;
    case CMD_ALL:
	if (use_crt_filter_always ||
	    ((text_length(current) + num_headers(current)) >= cmcsb._cmrmx))
	    pipe = more_pipe_open(stdout);
	display_msg(pipe,current,expand);
	break;
    case CMD_BCC:
	if (current->bcc)
	    disp_addresses(pipe,"Bcc", current->bcc->address,
			   expand,true,false,true);
	break;
    case CMD_CC:
	if (current->cc)
	    disp_addresses(pipe,"Cc", current->cc->address,expand,true,false,
			   true);
	break;
    case CMD_FCC:
	if (current->fcc)
	    disp_addresses(pipe,"Fcc", current->fcc->address,
			   expand,true,false,false);
	break;
    case CMD_FROM:
	maybe_disp_header(pipe,"From",current->from);
	break;
    case CMD_IN_REPLY_TO:
	maybe_disp_header(pipe,"In-reply-to",current->in_reply_to);
	break;
    case CMD_REPLY_TO:
	maybe_disp_header(pipe,"Reply-to",current->reply_to);
	break;
    case CMD_SUBJECT:
	maybe_disp_header(pipe,"Subject", current->subject);
	break;
    case CMD_TEXT:
	if (use_crt_filter_always ||
	    (text_length(current) >= cmcsb._cmrmx))
	    pipe = more_pipe_open(stdout);
	display_text(pipe,current);
	break;
    case CMD_TO:
	if (current->to)
	    disp_addresses(pipe,"To",current->to->address,expand,true,false,
			   true);
	break;
    case CMD_USER_HEADER:
	disp_user_headers(pipe, current);
	break;
    }
    if (pipe == stdout)			/* didn't open the pipe */
	fflush(stdout);
    else
	more_pipe_close(pipe);
}

/*
 * maybe_disp_header:
 * write out the header name and the body of the header, if non-zero-length
 *
 * Chris thinks this business of prepending "X-" to unknown headers is rude,
 * if not illegal. Should we make it a user-settable option?
 */
int Xhack = 0;				/* 1 => prepend X- to odd headers */

maybe_disp_header(fp,name, header) 
FILE *fp;
char *name;
headers *header;
{
    int prelen;
    char *subj, *foldstring();

    if (header && header->string && strlen(header->string) > 0) {
	prelen = strlen (name) + 2 +
	    ((Xhack && (header->flags&HEAD_UNKNOWN)) ? strlen("X-") : 0);
	subj = foldstring (prelen, header->string);
	fprintf(fp,"%s%s: %s\n",
		(Xhack && (header->flags&HEAD_UNKNOWN)) ? "X-" : "",
		name, subj);
	free (subj);
    }
}

display_msg (pipe, msg, expand)
FILE *pipe;
mail_msg *msg;
int expand;
{
    display_header (pipe, msg, expand, FALSE);
    fputc ('\n', pipe);
    display_text (pipe, msg);
}
    
/*
 * display_header:
 * write out the headers (properly folded) into the message body (in a 
 * temp file or pipe).
 * If smail is TRUE, called by sendmail() so do not write out Bcc,
 * add a Sender: field when necessary, etc.
 * if expand is true, expand groups
 */


#ifdef DONT_EMIT_FROM_HEADERS
int suppress_from_headers = 1;
#else
int suppress_from_headers = 0;
#endif

display_header (pipe, msg, expand, smail)
FILE *pipe;
mail_msg *msg;
int expand,smail;
{
    headers *h = msg->headers;

    if (smail && !suppress_from_headers && (msg->resent_to == NULL) &&
	(msg->from == NULL || !valid_from_field(msg->from->string)))
	write_sender(pipe);

    while (h) {
	switch (h->type) {
	  case TO:
	  case CC:
	  case RESENT_TO:
	    if (h->address && h->address->last)
		disp_addresses(pipe,h->name, h->address,expand,true,smail,
			       true);
	    break;
	  case BCC:
	    if (!smail)			/* not for sendmail */
		disp_addresses(pipe, h->name, h->address,expand,true,smail,
			       true);
	    break;
	  case FCC:
	    if (!smail)
		disp_addresses(pipe, h->name, h->address,expand,true,smail,
			       false);
	  case KEYWORDS:
	    if (h->keys)
		disp_keywords(pipe,h->name, h->keys);
	    break;
	    /*
	     * Some sites may not want us to generate the Sender: and
	     * From: fields.  We omit the Date: header as well, primarily
	     * so it won't appear after the From: header added by the
	     * delivery agent (it looks a little strange that way).
	     *
	     * Note that this is bad news in terms of trying to make
	     * the Fcc'd files look like those sent out by the mailer.
	     * But the Message-ID is omitted in either case, and the
	     * mailer may be munging the sender/recipient headers
	     * anyway.
	     */ 
	  case FROM:
	  case DATE:
	    if (smail && suppress_from_headers)
		break;
	    /* fall through */
	  default:
	    maybe_disp_header (pipe, h->name, h);
	    break;
	}
	h = h->next;
    }
}

disp_keywords (fp, name, keys) 
FILE *fp;
char *name;
keylist keys;
{
    if (keys) {
	if (name)
	    fprintf(fp,"%s: ", name);
	for(; keys && *keys; keys++)
	    fprintf(fp,"%s%s", *keys, *(keys+1) ? ", " : "\n");
    }
}

/*
 * write out the body (text) of a message into a file (or pipe)
 */
display_text (pipe,msg)
FILE *pipe;
mail_msg *msg;
{
    if (msg->body && strlen(msg->body) > 0) {
	fwrite (msg->body, sizeof(char), strlen(msg->body), pipe);
	if (msg->body[strlen(msg->body) -1] != '\n')
	    fputc('\n',pipe);		/* and trailing newline */
    }
}


/*
 * text_length:
 * find out how many lines the text will take on the screen, making sure
 * we start and end with a newline
 */
int
text_length (msg)
mail_msg *msg;
{
    int len;

    if (msg->body && (len = strlen (msg->body)))
	return (logical_lines (msg->body, cmcsb._cmrmx) + 
		((msg->body[0] == '\n') ? 0 : 1) +
		((msg->body[len] == '\n') ? 0 : 1));
    return (0);				/* nothing */
}

/*
 * num_headers:
 * count how many real headers are in this message (to determine about
 * how many lines they'll take up)
 */
num_headers(msg)
mail_msg *msg;
{
    int num = 0;
    headers *h = msg->headers;

    while (h) {
	switch(h->type) {
	case TO:
	case CC:
	case BCC:
	case RESENT_TO:
	    if (h->address && h->address->last)
		num++;
	    break;
	case KEYWORDS:
	    if (h->keys)
		num++;
	    break;
	default:
	    if (h && h->string && strlen(h->string) > 0)
		num++;
	    break;
	}
	h = h->next;
    }
    return (num);
}

disp_user_headers(fd, m)
FILE *fd;
mail_msg *m;
{
    headers *h = m->headers;

    while(h) {
	if (h->type == USER_HEADERS)
	    fprintf(fd,"%s: %s\n",h->name,h->string);
	h = h->next;
    }
}

/*
 * foldstring:
 * fold this string if it's too long to fit on one line
 * whether or not it fits, malloc up the return value,
 * SO IT MUST BE FREED
 */
char *
foldstring(prefix, unfolded)
int prefix;				/* how much of line used already */
char *unfolded;
{
    char *folded, *stripped;		/* folded and unfolded versions */
    int ufp;				/* unfolded str pointer (stripped) */
    int fp;				/* folded string pointer */
    int bp;				/* break point pointer */
    int col;				/* columns used in current line */
    int len;				/* malloc'ed length of folded */
    int indentlen = 8;			/* amount to indent */
    int firstline = TRUE;		/* first line (line with prefix) */
    int i;
#define MAXCOL 78

    stripped = (char *) safe_strcpy(unfolded);
    stripped = (char *) stripspaces(stripped);	/* unfold to do it right */

    col = prefix;
    ufp = fp = 0;
    len = strlen(stripped) + 1;
    folded = malloc (len);
    while (stripped[ufp] != '\0') {
	folded[fp++] = stripped[ufp++];
	col++;
	if (col == MAXCOL) {
	    folded = (char *) 
		realloc (folded, len += (indentlen+1)); /* indent + nl */
	    fp--; ufp--;		/* point at last char copied */

	    /* find a good spot to break line - back up to whitespace */
	    bp = fp;
	    while ((folded[bp] != ' ') && (folded[bp] != '\t') && 
		   (folded[bp] != '\n') && (bp > 0)) {
		bp--;
	    }
	    /* did we find whitespace before we backed into the indentation? */
	    if (((folded[bp] == ' ') || (folded[bp] == '\t')) /* whitespace */
		&& (MAXCOL - (fp - bp) > (firstline ? prefix : indentlen))) {
		/* backup pointers to matching locations in their strings */
		ufp -= (fp - bp);	/* distance we backed up */
		fp = bp;		/* both point just past whitespace */
	    }
	    else {			/* no whitespace to break at */
		++ufp; ++fp; 
		while ((stripped[ufp] != ' ') && (stripped[ufp] != '\t')
		       && (stripped[ufp] != '\0')) {
		    folded[fp++] = stripped[ufp++];
		}
		if (firstline & (fp - bp) + indentlen <= MAXCOL) {
		    /* fits on next line (indentlen is less than prefix) */
		    /* break line: backup pointers to matching locations */
		    ufp -= (fp - bp + 1); /* distance we backed up */
		    fp = bp;		/* both point just past whitespace */
		}
		else {			/* break at next whitespace */
		    if (stripped[ufp] == '\0') /* end of string - done */
			break;
		    bp = fp;		/* prepare to put newline here */
		}
	    }
	    folded[fp++] = '\n';	/* replace white space with newline */
	    ufp++;			/* skip past the whitespace */
	    firstline = FALSE;		/* no longer on first line */
	    for (i = 0; i < indentlen; i++, fp++) /* insert indentation */
		folded[fp] = ' ';
	    col = indentlen;
	}
    }
    folded[fp] = '\0';
    free (stripped);			/* done with that */
    return (folded);
}
