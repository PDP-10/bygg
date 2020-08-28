/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/seq.c,v 2.2 90/10/04 18:26:32 melissa Exp $";
#endif

/*
 * seq.c - message sequence selection routines
 *
 *
 * Notes:
 *
 * Sequence constraint minima are inclusive and maxima are exclusive.  For
 * example, a 2000 byte message received "today" is "longer (than) 2000" and
 * "since today" but not "shorter (than) 2000" or "before today".
 */

#include "mm.h"
#include "parse.h"

#define CS (cf->sequence)
#define PS (cf->prev_sequence)
#define RS (cf->read_sequence)

#define CSfirst (sequence_first(CS))
#define CSlast (sequence_last(CS))
#define CSinverted (sequence_inverted(CS))
#define PSfirst (sequence_first(PS))
#define PSlast (sequence_last(PS))
#define PSinverted (sequence_inverted(PS))
#define RSfirst (sequence_first(RS))
#define RSlast (sequence_last(RS))
#define RSinverted (sequence_inverted(RS))

/*
 * message selection keywords
 */

keywrd seq_keys[] = {
    { "a", KEY_INV|KEY_ABR, 2 },	/* a --> all */
    { "after", 0, (keyval) SEQ_AFTER },
    { "all", 0, (keyval) SEQ_ALL },
    { "answered", 0, (keyval) SEQ_ANSWERED },
    { "before", 0, (keyval) SEQ_BEFORE },
    { "current", 0, (keyval) SEQ_CURRENT },
    { "deleted", 0, (keyval) SEQ_DELETED },
    { "flagged", 0, (keyval) SEQ_FLAGGED },
    { "from", 0, (keyval) SEQ_FROM },
    { "inverse", 0, (keyval) SEQ_INVERSE },
    { "keyword", 0, (keyval) SEQ_KEYWORD },
    { "l", KEY_INV|KEY_ABR, (keyval) 12 }, /* 12 --> last (groan) */
    { "last", 0, (keyval) SEQ_LAST },
    { "longer", 0, (keyval) SEQ_LONGER },
    { "new", 0, (keyval) SEQ_NEW },
    { "on", 0, (keyval) SEQ_ON },
    { "previous-sequence", 0, (keyval) SEQ_PREVIOUS },
    { "recent", 0, (keyval) SEQ_RECENT },
    { "seen", 0, (keyval) SEQ_SEEN },
    { "shorter", 0, (keyval) SEQ_SHORTER },
    { "since", 0, (keyval) SEQ_SINCE },
    { "subject", 0, (keyval) SEQ_SUBJECT },
    { "text", 0, (keyval) SEQ_TEXT },
    { "to", 0, (keyval) SEQ_TO },
    { "u", KEY_ABR|KEY_INV, (keyval) 29 }, /* u -> unseen */
    { "unanswered", 0, (keyval) SEQ_UNANSWERED },
    { "undeleted", 0, (keyval) SEQ_UNDELETED },
    { "unflagged", 0, (keyval) SEQ_UNFLAGGED },
    { "unkeyword", 0,  (keyval) SEQ_UNKEYWORD },
    { "unseen", 0, (keyval) SEQ_UNSEEN }
};

seq_node seq_stack[SEQ_STACK_SIZE];
int seq_stackp;
#define stackinit() seq_stackp = -1, invert_sequence = 0, use_previous = 0
#define stacktop seq_stack[seq_stackp]
#define stackdata stacktop.data
#define stackrange stackdata.range
#define stackdates stackdata.dates
#define stackpush(x) if (++seq_stackp >= sizeof seq_stack/sizeof seq_stack[0])\
			ccmd_errmsg("Too many sequence constraints");\
		     else\
			seq_stack[seq_stackp].type = x;
#define stackpop() seq_stackp--
static int invert_sequence, use_previous;

int seq_date (), seq_num (), seq_string (), seq_misc(), seq_int(), 
    seq_keyword();

int (*seq_fns[sizeof(seq_keys)/sizeof(seq_keys[0])])() = { 
    seq_date,				/* after */
    seq_misc,				/* all */
    seq_misc,				/* answered */
    seq_date,				/* before */
    seq_misc,				/* current */
    seq_misc,				/* deleted */
    seq_misc,				/* flagged */
    seq_string,				/* from */
    seq_misc,				/* inverse */
    seq_keyword,			/* keyword */
    seq_int,				/* last */
    seq_num,				/* longer */
    seq_misc,				/* new */
    seq_date,				/* on */
    seq_misc,				/* previous-sequence */
    seq_misc,				/* recent */
    seq_misc,				/* seen */
    seq_num,				/* shorter */
    seq_date,				/* since */
    seq_string,				/* subject */
    seq_string,				/* text */
    seq_string,				/* to */
    seq_misc,				/* unanswered */
    seq_misc,				/* undeleted */
    seq_misc,				/* unflagged */
    seq_keyword,			/* unkeyworded */
    seq_misc				/* unseen */
};

keytab seq_keytab = { sizeof (seq_keys) / sizeof (keywrd), seq_keys };

fdb seq_fdb_key = 
    { _CMKEY, 0, 0, (pdat) &seq_keytab, "message sequence, ", NULL, NULL, 
      "invalid message sequence type"};
fdb seq_fdb_n =
    { _CMNUM, CM_SDH, 0, (pdat) 10, "message number\n\
  or range of message numbers, n:m\n\
  or range of message numbers, n-m\n\
  or range of message numbers, n+m (m messages beginning with n)",
    NULL, NULL, "Invalid message sequence" };
fdb seq_fdb_dot =
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) ".",
	  "\".\" to specify the current message" };
fdb seq_fdb_star =
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) "*",
	  "\"*\" to specify the last message" };
fdb seq_fdb_percent =
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) "%", NULL };
fdb seq_fdb_colon = 
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) ":",
	  "\":\" to specify a message range",
      NULL, NULL, "Invalid message range delimiter"};
fdb seq_fdb_dash =
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) "-",
	  "\"-\" to specify a message range",
      NULL, NULL, "Invalid sequence range delimiter"};
fdb seq_fdb_hash =
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) "#", NULL , NULL, NULL,
	  "Invalid message range delimiter" };
fdb seq_fdb_plus =
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) "+",
	  "\"+\" to specify a message range",
      NULL, NULL, "Invalid sequence range delimiter" };
fdb seq_fdb_comma =
    { _CMTOK, CM_SDH|TOK_WAK, 0, (pdat) ",",
	  "\",\" and another message sequence", 
      NULL, NULL, "Expected a \",\""};
fdb seq_fdb_timeonly =
    { _CMTAD, CM_SDH|DTP_N24|DTP_NDA|DTP_NDW|DTP_NSP, nil, nil,
	  "date and time", NULL, NULL, "Invalid date and time" };
fdb seq_fdb_dateonly =
    { _CMTAD, CM_SDH|DTP_N24|DTP_NTI|DTP_NDW|DTP_NSP, nil, nil,
	  "date and time", NULL, NULL, "Invalid date" };
fdb seq_fdb_tad = { _CMTAD, CM_SDH|DTP_N24|DTP_NDW|DTP_NSP };
extern keytab timetab;
fdb seq_fdb_daykey = { _CMKEY, CM_SDH, nil, (pdat) &timetab, "day, ", NULL,
		       NULL, "Invalid day" };

#define SEQ_FDBS &seq_fdb_n, &seq_fdb_dot, &seq_fdb_star, &seq_fdb_key, \
	&seq_fdb_percent, &seq_fdb_comma
#define SEQ_TAD_FDBS &seq_fdb_tad, &seq_fdb_dateonly, &seq_fdb_timeonly, \
	&seq_fdb_daykey

/* miscellaneous easy sequence functions */
seq_keyword(n)
{
    stackpush(n);
    strcpy(stackdata.s,parse_old_keywords());
    parse_seq ((char *) nil, nil, nil);
}
    

seq_misc(n)
{
    stackpush(n);
    switch (n) {
      case SEQ_PREVIOUS:
	use_previous = true;
	break;
      case SEQ_INVERSE:
	invert_sequence = !invert_sequence; /* XXX should be a seq_node */
	break;
      case SEQ_ALL:
      case SEQ_ANSWERED:
      case SEQ_CURRENT:
      case SEQ_DELETED:
      case SEQ_FLAGGED:
      case SEQ_NEW:
      case SEQ_RECENT:
      case SEQ_SEEN:
      case SEQ_UNANSWERED:
      case SEQ_UNDELETED:
      case SEQ_UNFLAGGED:
      case SEQ_UNSEEN:
	break;				/* XXX get rid of these */
      default:
	ccmd_errmsg ("Unknown sequence constraint (%d) in seq_misc", n);
    }
    parse_seq ((char *) nil, nil, nil);
}

seq_date (n)
{
    fdb *fdbs;

    stackpush(n);

    fdbs = fdbchn (&seq_fdb_daykey, &seq_fdb_dateonly, (fdb*) nil);
    parse (fdbs, &pv, &used);

    switch (used->_cmfnc) {
      case _CMKEY:
	stackdates.first = key2time(pv._pvint);
	break;
      case _CMTAD:
	stackdates.first = datimetogmt (&pv._pvtad);
	break;
      default:
	ccmd_errmsg ("Bad parse function (%d) in seq_date", used->_cmfnc);
    }

    /* got date, now look for additional time field or something else */
    fdbs = fdbchn (&seq_fdb_timeonly, SEQ_FDBS, &cfm_fdb, (fdb *) nil);
    parse (fdbs, &pv, &used);
    if (n == SEQ_ON)
	stackdates.last = stackdates.first + (24 * 60 * 60);
    if ((n == SEQ_AFTER) && (used->_cmfnc != _CMTAD))
	stackdates.first += (24*60*60);
    switch (used->_cmfnc) {
      case _CMCFM:
	return true;
      case _CMTAD:
	stackdates.first += (((pv._pvtad._dthr*60)+pv._pvtad._dtmin)*60) +
	    pv._pvtad._dtsec;
	return parse_seq ((char *) nil, nil, nil);
      default:
	return seq_interpret_fdb(used);
    }
}

seq_comma (comma_seen)
int comma_seen;
{
    if (!comma_seen)
	if (parse (fdbchn (&seq_fdb_comma, &cfm_fdb, nil), &pv, &used) ==
	    CMxOK)
	    switch (used->_cmfnc) {
	      case _CMCFM:
		return true;
	      case _CMTOK:
		break;
	    }
    parse_seq ("", nil, nil);
    return true;
}

seq_num (n)
{
    noise ("than");
    p_num ("number of characters");
    stackpush(n);
    stackrange.n = pv._pvint;
    parse_seq (nil, nil, nil);
}

seq_int (n)
{
    static fdb numfdb = { _CMNUM, CM_SDH, 0, (pdat) 10, NULL, NULL, NULL,
			  "invalid number" };
    numfdb._cmhlp = "decimal number";
    numfdb._cmdef = "1";

    noise("number of messages");
    parse(&numfdb, &pv, &used);
    stackpush(n);
    stackrange.n = pv._pvint;
    parse_seq (nil, nil, nil);
}

seq_string (n)
{
    extern brktab rembrk;		/* remote user name break table */
    static fdb string_fdb = { _CMFLD, CM_SDH|FLD_EMPTY|FLD_WAK, nil, nil,
			      "string", NULL, &rembrk, "Invalid String" };
    static fdb qst_fdb = { _CMQST, nil, nil, nil, nil, nil, nil,
			       "Invalid String" };
    int len;

    noise ("string");
    parse (fdbchn (&qst_fdb, &string_fdb, (fdb *) nil), &pv, &used);
    if ((len = strlen (atmbuf)) < 1)
	ccmd_errmsg ("Invalid string.  Try enclosing it in quotes");
    if (len > (sizeof stackdata.s - 1))
	ccmd_errmsg ("String too long - \"%s\"", atmbuf);
    stackpush(n);
    strcpy (stackdata.s, atmbuf);
    return parse_seq ((char *) nil, nil, nil);
}

seq_range (n)
int n;
{
    fdb *fdbs = fdbchn (&seq_fdb_colon, &seq_fdb_dash, &seq_fdb_plus,
			&seq_fdb_hash, &seq_fdb_comma, &cfm_fdb, nil);

    if (n < 1 || n > cf->count)
	ccmd_errmsg ("Number out of range");
    stackpush(SEQ_RANGE);
    stackrange.n = stackrange.m = n;
    if (parse (fdbs, &pv, &used) == CMxOK) {
	if (used == &cfm_fdb) {
	    return true;		/* return just 1 msg num */
	}
	if (used->_cmfnc == _CMTOK)
	    switch (used->_cmdat[0]) {
	      case ',':
		parse_seq ("", nil, nil);
		return true;
		break;
	      case '+':
	      case '#':
		stackrange.m += p_num ("number of messages") - 1;
		break;
	      case '-':
	      case ':':
		{
		    static fdb numfdb = { _CMNUM, CM_SDH, 0, (pdat) 10,
					      "end of range", NULL, NULL,
					      "bad range end" };
		    parse (fdbchn (&numfdb, &seq_fdb_star, &seq_fdb_percent, 
				   nil), &pv, &used);
		    stackrange.m = ((used == &seq_fdb_star) ||
				    (used == &seq_fdb_percent)) ?
				      cf->count : pv._pvint;
		}		    
		break;
	      default:
		ccmd_errmsg ("Internal error in token evaluation");
	    }
	if (stackrange.m < stackrange.n)
	    ccmd_errmsg ("Invalid range");
/*	seq_comma (nil);*/
	parse_seq (nil, nil, nil);
	return true;
    }
    else
	return false;
}

/*
 * chain together various pieces for a sequence parse
 */
fdb *
build_seq_chn (chn, def, chnlen, realseq)
fdb *chn;
char *def;
int *chnlen;
int realseq;				/* sequence or confirm */
{
    fdb *f;
    
    *chnlen = 0;

    (*chnlen)++;			/* at least one element */

    /* chain the normal stuff at the end of this chain */
    for (f = chn; f->_cmlst != nil; f = f->_cmlst)
	(*chnlen)++;
    /* f now points to the last element of the chain */
    if (!realseq)			/* just stick in a confirm */
        fdbchn (f, &cfm_fdb, (fdb *) nil);
    else if (def)	 		/* non-nil def means required input */
	fdbchn (f, SEQ_FDBS, (fdb *) nil);
    else
	fdbchn (f, SEQ_FDBS, &cfm_fdb, (fdb *) nil);
    return (chn);
}

/*
 * You want parse_sequence from outside this module...
 *
 * recursively parse a message sequence.  if def is non-null, a response
 * will be required; if the string it points to isn't null, use that as a
 * default string.  Thus:
 *
 * 	parse_seq (nil, ...);		parse sequence or confirm
 * 	parse_seq ("", ...);		parse required sequence
 * 	parse_seq ("all", ...);		parse sequence, confirm -> all
 */

static
parse_seq (def, chn, altused)
char *def;
fdb *chn;
fdb **altused;
{
    fdb *fdbs, *f;
    int l = 0;
    int i;

    seq_fdb_n._cmdef = (def && *def) ? def : nil; /* XXX */

    if (!chn) {
	if (def)			/* non-nil def means required input */
	    fdbs = fdbchn (SEQ_FDBS, (fdb *) nil);
	else
	    fdbs = fdbchn (SEQ_FDBS, &cfm_fdb, (fdb *) nil);
    }
    else
	fdbs = build_seq_chn (chn, def, &l, TRUE); /* chain extras in front */

    parse (fdbs, &pv, &used);	/* parse the first token */
    seq_fdb_n._cmdef = nil;		/* XXX */
    for (i = 0, f = chn; i < l; i++, f = f->_cmlst) {
	if (used == f) {		/* one of the chain elements */
	    *altused = used;
	    return;
	}
    }

    /* otherwise, it was really a sequence parse */
    seq_interpret_fdb (used);
}

/*
 * parse a message sequence and save the result as the "current sequence"
 *
 * 	parse_sequence (nil, ...);	parse sequence or confirm
 * 	parse_seqeunce ("", ...);	parse required sequence
 * 	parse_sequence ("all", ...);	parse sequence, confirm -> all
 *
 * returns false if changes the message sequence is prohibited by the
 * current context, true otherwise.
 *
 * If an fdb chain is passed, link this to the front of our sequence.
 * After parsing, fill in used with the fdb we parsed if it was one of the
 * passed ones (pval in global pv), or NULL if we parsed a sequence.
 * This is done so optional items can be parsed before a sequence.
 * (chn should be NULL iff altused is)
 *
 */

int
parse_sequence (def, chn, altused)
char *def;				/* default response */
fdb *chn;
fdb **altused;
{
    int i, l;
    sequence_t temp;

    /*
     * MM-20 compatibility: don't allow user to change the message
     * sequence in the middle of a read or send command.  We can
     * relax this later, once we redefine the semantics of "previous-sequence".
     */

    if (altused)
	*altused = NULL;		/* no fdb yet */

    if (mode != MM_TOP_LEVEL) {
	if (chn) {			/* parse optional stuff */
	    parse (build_seq_chn (chn, def, &l, FALSE), &pv, &used);
	    if (used != &cfm_fdb)
		*altused = used;
	}
	else
	    confirm();
	return false;
    }

    noise ("messages");

    stackinit();

    parse_seq (def, chn, altused);	/* do the actual work */
    if (altused && *altused)		/* sequence not parsed */
	return true;

    /*
     * Save the current sequence as the "previous-sequence", using
     * a swap to avoid free and malloc.
     */
    temp = PS;
    PS = CS;
    CS = temp;
    
    clear_sequence (CS);
    
    for (i = 1; i <= cf->count; i++) {
	if (use_previous) {
	    if (i < PSfirst)
		i = PSfirst;
	    else if (i > PSlast)
		break;
	    if (!in_sequence (PS, i))
		continue;
	}
	if (!seq_test (i))
	    continue;
	setbit (sequence_bits(CS), i);
	if (CSfirst == 0)
	    CSfirst = i;
	CSlast = i;
    }
    if (CSlast < CSfirst)		/* sequence is empty */
	CSlast = CSfirst = 0;
    
    sequence_inverted(CS) = invert_sequence;
    if (use_previous && sequence_inverted(PS))
	sequence_inverted(CS) = !sequence_inverted(CS);
    if (debug)
	fprintf (stderr, "sequence: first = %d, last = %d, current = %d\n",
		 CSfirst, CSlast, cf->current);
    return true;
}

/*
 * sequence_init - allocate and initialize sequence bitmaps
 *
 */

sequence_init (cf)
msgvec *cf;
{
    int n = cf->count / NBBY + 1;
    if ((CS = (sequence_t) malloc (sizeof (*CS))) &&
	(sequence_bits (CS) = (unsigned char *) malloc (n)) &&
	(PS = (sequence_t) malloc (sizeof (*PS))) &&
	(sequence_bits (PS) = (unsigned char *) malloc (n)) &&
	(RS = (sequence_t) malloc (sizeof (*RS))) &&
	(sequence_bits (RS) = (unsigned char *) malloc (n))) {

	sequence_file(CS) = cf;
	sequence_file(PS) = cf;
	sequence_file(RS) = cf;
	
	clear_sequence (CS);
	clear_sequence (PS);
	clear_sequence (RS);
	return true;
    }
    return false;
}

sequence_free (cf)
msgvec *cf;
{
    free_sequence(CS);
    free_sequence(PS);
    free_sequence(RS);
}

/*
 * Make sequence S1 a copy of sequence S2.
 */
copy_sequence (S1, S2)
sequence_t S1, S2;
{
    sequence_first (S1) = sequence_first (S2);
    sequence_last (S1) = sequence_last (S2);
    sequence_inverted (S1) = sequence_inverted (S2);
    sequence_file (S1) = sequence_file (S2);
    bcopy (sequence_bits(S2), sequence_bits(S1),
	   sequence_last (S1) / NBBY + 1);
}

/*
 * set the current message number, and return it.  The result will be
 * zero if the message sequence is empty.
 */
int
sequence_start (S)
sequence_t S;
{
    if (!valid_sequence(S))
	return 0;
    return (sequence_file(S)->current =
	    sequence_inverted(S) ? sequence_last(S) : sequence_first(S));
}

/* XXX make these macros later on */
int
sequence_next (S)
sequence_t S;
{
    return sequence_next1 (S, 1);
}

int
sequence_prev (S)
sequence_t S;
{
    return sequence_next1 (S, -1);
}

/*
 * advance to the next message in sequence S.
 * dir is 1 to move forward, -1 to move backards.
 */
sequence_next1 (S, dir)
sequence_t S;
int dir;
{
    int n;
    if (!valid_sequence(S))
	return 0;

    n = sequence_file(S)->current;	/* get current message */

    if (sequence_inverted(S))		/* change direction if inverse seq */
	dir = -dir;

    while ((n += dir) && (n <= sequence_last(S)))
	if (in_sequence (S, n))
	    return (sequence_file(S)->current = n);
    return 0;
}


/*
 * find the next message in the sequence
 * arg is +1 to move up in the sequence, and -1 to move back
 * OBSOLETE
 */

int
next_current (up)			/* was next_current */
{
    int n = cf->current;

    up = (up < 0) ? -1 : 1;
    if (CSinverted)
	up = -up;

    /* XXX this could be a lot faster! */
    while ((n += up) && (n > 0) && (n <= CSlast))
	if (in_sequence (CS, n)) {
	    cf->current = n;
	    return n;
	}
    return 0;
}


int
seq_next (n)
int n;
{
    if (CSinverted)
	if (n < 1)
	    return CSlast;
	else
	    while (n-- > CSlast)
		if (in_sequence (CS, n))
		    return n;
    else
	if (n < 1)
	    return CSfirst;
	else
	    while (n++ < CSlast)
		if (in_sequence (CS, n))
		    return n;
    return 0;
}

int
seq_print (nl)
{
    int lastn, lastprinted;
    int n = sequence_start (cf->sequence);

    if (!n)
	return (0);

    printf (" %d", lastprinted = n);

    lastn = 0;				/* initialize this */
    while (n = sequence_next (cf->sequence)) {
	if (n == (lastprinted + 1)) {
	    lastn = lastprinted = n;
	    continue;
	}
	if (lastn) {
	    printf (":%d", lastn);
	    lastn = 0;
	}
	printf (", %d", lastprinted = n);
    }
    if (lastn)
	printf (":%d", lastn);
    if (nl)
	printf ("\n");
    return 1;
}

seq_interpret_fdb (used)
fdb *used;
{
    int n;

    switch (used->_cmfnc) {
      case _CMCFM:
	return true;
      case _CMKEY:
	n = pv._pvint;
	if (seq_fns[n])
	    (*seq_fns[n]) (n);		/* call handler function */
	else
	    parse_seq ((char *) nil, nil, nil);
	break;
      case _CMTOK:
	switch (used->_cmdat[0]) {
	  case '.':
	    if (seq_range (cf->current)) /* XXX */
		return true;
	    break;
	  case '*':
	  case '%':
	    if (seq_range (cf->count))	/* XXX */
		return true;
	    break;
	  case ',':
	    parse_seq ("", nil, nil);
	    return true;
	  default:
	    ccmd_errmsg ("Invalid token '%s' parsed", used->_cmdat);
	}
	break;
      case _CMNUM:
	if (seq_range (pv._pvint))
	    return true;
	seq_comma (true);
	break;
      default:
	ccmd_errmsg ("Got an unexpected fdb in seq_interpret_fdb (%d)",
	       used->_cmfnc);
    }
}

FILE *header_pipe = NULL;

int
cmd_headers(n)
int n;
{
    void header_print();
    FILE *more_pipe_open();

    if (!check_cf (O_RDONLY))
	return;
    header_pipe = cmcsb._cmoj ? cmcsb._cmoj : stdout; /* don't write to NULL */
    if (parse_sequence ("current", nil, nil)) {	/* count how many headers */
	int count,n;
	for (count = 0, n = sequence_start (cf->sequence); 
	     n && count < cmcsb._cmrmx;
	     n = sequence_next (cf->sequence))
	    ++count;
	if (count >= cmcsb._cmrmx)
	    header_pipe = more_pipe_open(header_pipe);
    }
    sequence_loop (header_print);
    if ((header_pipe == cmcsb._cmoj) ||
	(header_pipe == stdout))	/* didn't open any pipe */
	fflush(header_pipe);		/* ... since we're not closing it */
    else
	more_pipe_close(header_pipe);
    header_pipe = NULL;
}

void
header_print(n)
{
    static header_count = 0;
    extern int header_summary ();

    switch (n) {
      case 0:
	header_count = 0;
	break;
      case -1:
	if (debug)
	    printf ("Matching headers = %d\n", header_count);
	break;
      default:
	header_count++;
	header_summary (n, header_pipe, 0); /* 0 => "header command" */
    }
}

/*
 * Apply a function to each message in the current message sequence.
 * 
 * In addition to valid message numbers, the function is also called
 * with zero to indicate the beginning of a sequence, and -1 to indicate
 * that the sequence has been exhausted.
 */

sequence_loop (fn)
void (*fn)();
{
    int i;
    int new = 1;

    (*fn) (0);				/* initialize the function */

    if (mode == MM_TOP_LEVEL) {
	sequence_t S = CS;		/* use current sequence */

	if (valid_sequence (S)) {
	    if (sequence_inverted (S))	/* run backwards */
		for (i = sequence_last (S); i >= sequence_first (S); i--) {
		    if (in_sequence (S, i)) {
			cf->current = i;
			(*fn) (i);
		    }
		}
	    else			/* run forwards */
		for (i = sequence_first(S); i <= sequence_last(S); i++)
		    if (in_sequence (S, i)) {
			cf->current = i;
			(*fn) (i);
		    }
	}
    }
    else
	if (cf->current)
	    (*fn) (cf->current);	/* just use current message */

    (*fn) (-1);				/* tell function we're done */
}

/*
 * Test whether message n is part of the user's selection.
 * This is pretty awful, but it'll do until we finish defining
 * the message sequence grammar (at which point we'll need an
 * and/or tree anyway).
 */

seq_test (n)
int n;					/* message number */
{
    int i, inrange = false, haverange = false;
    
    /*
     * We test ranges first as a (lame) optimization.
     * Obviously this could be a lot faster.
     */

    for (i = 0; i <= seq_stackp; i++)
	if (seq_stack[i].type == SEQ_RANGE) {
	    haverange = true;
	    if (seq_test1 (n, &seq_stack[i]) == false)
		continue;
	    inrange = true;
	    break;
	}

    if (haverange && !inrange)
	return false;

    for (i = 0; i <= seq_stackp; i++)
	if (seq_stack[i].type != SEQ_RANGE)
	    if (seq_test1 (n, &seq_stack[i]) == false)
		return false;

    return true;
}

static
seq_test1(n, node)
int n;
seq_node *node;
{
    message *m = &cf->msgs[n];
    switch (node->type) {
      case SEQ_SINCE:
      case SEQ_AFTER:
	return (m->date >= node->data.dates.first);
      case SEQ_BEFORE:
	return (m->date < node->data.dates.first);
      case SEQ_INVERSE:
      case SEQ_ALL:
	return true;		/* XXX should not get here */
      case SEQ_ANSWERED:
	return (m->flags & M_ANSWERED);
      case SEQ_CURRENT:
	return (cf->current == n);
      case SEQ_DELETED:
	return (m->flags & M_DELETED);
      case SEQ_FLAGGED:
	return (m->flags & M_FLAGGED);
      case SEQ_FROM:
	if (search_header ("from", node->data.s, m))
	    return true;
	return false;
      case SEQ_KEYWORD:
	return(lookup_keyword(node->data.s, m->keywords));
      case SEQ_LAST:
	return (cf->count < n + node->data.range.n);
      case SEQ_SHORTER:
	return (m->size < node->data.range.n);
      case SEQ_LONGER:
	return (m->size >= node->data.range.n);
      case SEQ_NEW:
	return (!(m->flags & M_SEEN) && (m->flags & M_RECENT));
      case SEQ_ON:
	return (m->date >= node->data.dates.first &&
		m->date <= node->data.dates.last);
      case SEQ_PREVIOUS:
	return true;			/* XXX caller has to check... */
      case SEQ_RECENT:
	return (m->flags & M_RECENT);
      case SEQ_SEEN:
	return (m->flags & M_SEEN);
      case SEQ_SUBJECT:
	if (search_header ("subject", node->data.s, m))
	    return true;
	return false;
      case SEQ_TEXT:
	if (search_text (node->data.s, m))
	    return true;
	return false;
      case SEQ_TO:
	if ((search_header ("to", node->data.s, m)) ||
	    (search_header ("cc", node->data.s, m)) ||
	    (search_header ("bcc", node->data.s, m)))
	    return true;
	return false;
      case SEQ_UNANSWERED:
	return (!(m->flags & M_ANSWERED));
      case SEQ_UNDELETED:
	return (!(m->flags & M_DELETED));
      case SEQ_UNFLAGGED:
	return (!(m->flags & M_FLAGGED));
      case SEQ_UNKEYWORD:
	return(!lookup_keyword(node->data.s, m->keywords));
      case SEQ_UNSEEN:
	return (!(m->flags & M_SEEN));
      case SEQ_RANGE:
	return ((n >= node->data.range.n) &&
		(n <= node->data.range.m));
    }
    ccmd_errmsg ("Unknown sequence constraint type encountered");
}


#ifdef SEQDEBUG
char *FNS[] = {
    "no function",
    "cmcfm",
    "cmkey",
    "cmnum",
    "cmqst",
    "cmnoi",
    "cmtxt",
    "cmfld",
    "cmswi",
    "cmtok",
    "cmtad",
    "cmfil",
    "cmusr",
    "cmgrp",
    "cmpara"
};

printfdb(fdbs)
fdb *fdbs;
{
    if (fdbs == nil) {
	printf ("nil\n");
	return;
    }
    printf ("%s", FNS[fdbs->_cmfnc]);
    if (fdbs->_cmhlp)
	printf (" help='%s'", fdbs->_cmhlp);
    if (fdbs->_cmdef)
	printf (" def='%s'", fdbs->_cmdef);
    printf ("\n");
    printfdb(fdbs->_cmlst);
}

seq_print (node,last)
seq_node *node;
int last;
{
    int i;

    for (i = 0; i <= last; i++) {
	switch (node[i].type) {
	  case SEQ_AFTER:
	    printf ("after %s", daytime (node[i].data.dates.first)); break;
	  case SEQ_ALL:
	    printf ("all"); break;
	  case SEQ_ANSWERED:
	    printf ("answered"); break;
	  case SEQ_BEFORE:
	    printf ("before %s", daytime (node[i].data.dates.first)); break;
	  case SEQ_CURRENT:
	    printf ("current"); break;
	  case SEQ_DELETED:
	    printf ("deleted"); break;
	  case SEQ_FLAGGED:
	    printf ("flagged"); break;
	  case SEQ_FROM:
	    printf ("from \"%s\"", node[i].data.s); break;
	  case SEQ_INVERSE:
	    printf ("inverse"); break;
	  case SEQ_KEYWORD:
	    printf ("keyword"); break;
	  case SEQ_LAST:
	    printf ("last"); break;
	  case SEQ_LONGER:
	    printf ("longer than %d chars", node[i].data.range.n); break;
	  case SEQ_NEW:
	    printf ("new (recent and unseen)"); break;
	  case SEQ_ON:
	    printf ("between %s", daytime (node[i].data.dates.first));
	    printf (" and %s", daytime (node[i].data.dates.last)); break;
	  case SEQ_PREVIOUS:
	    printf ("previous sequence"); break;
	  case SEQ_RECENT:
	    printf ("recent"); break;
	  case SEQ_UNSEEN:
	    printf ("seen");
	  case SEQ_SHORTER:
	    printf ("shorter than %d chars", node[i].data.range.n); break;
	  case SEQ_SINCE:
	    printf ("since %s", daytime (node[i].data.dates.first)); break;
	  case SEQ_SUBJECT:
	    printf ("subject \"%s\"", node[i].data.s); break;
	  case SEQ_TEXT:
	    printf ("text \"%s\"", node[i].data.s); break;
	  case SEQ_TO:
	    printf ("to \"%s\"", node[i].data.s); break;
	  case SEQ_UNANSWERED:
	    printf ("not answered"); break;
	  case SEQ_UNDELETED:
	    printf ("not deleted"); break;
	  case SEQ_UNFLAGGED:
	    printf ("not flagged"); break;
	  case SEQ_UNSEEN:
	    printf ("not seen");
	  case SEQ_RANGE:
	    if (node[i].data.range.n != node[i].data.range.m)
		printf ("%d:%d", node[i].data.range.n, node[i].data.range.m);
	    else
		printf ("%d", node[i].data.range.n);
	}
	printf ("\n");
    }
}
#endif /*SEQDEBUG*/

