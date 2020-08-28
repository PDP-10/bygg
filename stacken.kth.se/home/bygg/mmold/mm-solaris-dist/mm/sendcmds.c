/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/sendcmds.c,v 2.4 1997/10/21 19:33:32 howie Exp $";
#endif

#include "mm.h"
#include "cmds.h"
#include "message.h"
#include "parse.h"

#define FORWLINE "                ---------------\n\n"

static mail_msg outgoing, *msg;		/* outgoing message */
mail_msg *parse_msg();
headers *new_header();
char *safe_strncat();
extern mail_msg *current;
int get_msg();

int
cmd_send (n)
int n;
{
    static fdb txtfdb = { _CMTXT };
    static fdb cfmfdb = { _CMCFM , CM_SDH};
    volatile int ret;				/* return value */
    volatile int aa;
    static addresslist temp= { nil, nil };
    extern int prompt_rcpt_always, prompt_for_cc, prompt_for_fcc;
    extern int prompt_for_bcc;

    if (mode & MM_SEND) {
	confirm();
	return(deliver());		/* deliver it now */
    }

    cm_set_ind(false);			/* no indirections here */

    noise("message to");
    free_addresslist(&temp);
    parse_addresses(&temp);
    free_msg(&outgoing);
    set_send_defaults(&outgoing);
    if (outgoing.to == NULL)
	outgoing.to = new_header(TO, "To", HEAD_KNOWN, &outgoing);
    if (outgoing.to->address == NULL) {
	outgoing.to->address = (addresslist *)malloc(sizeof(addresslist));
	outgoing.to->address->first = nil;
	outgoing.to->address->last = nil;
    }
    send_mode(&outgoing);
    if (temp.first) {
	merge_addresses(outgoing.to->address, &temp);
    }
    else {
	prompt_address(" To: ", outgoing.to->address);
	files_to_fcc(outgoing.to->address, &outgoing);

    }
    if (!temp.first || prompt_rcpt_always) {
	if (temp.first)
	    free_addresslist(&temp);
	if (prompt_for_cc) {
	    if (outgoing.cc->address == NULL) {
		outgoing.cc->address =
		    (addresslist *)malloc(sizeof(addresslist));
		outgoing.cc->address->first = NULL;
		outgoing.cc->address->last = NULL;
	    }
	    prompt_address(" cc: ", outgoing.cc->address);
	    files_to_fcc(outgoing.cc->address, &outgoing);
	}

	if (prompt_for_bcc) {
	    if (outgoing.bcc == NULL) {
		outgoing.bcc = new_header(BCC, "Bcc", HEAD_KNOWN,&outgoing);
		outgoing.bcc->address = NULL;
	    }
	    if (outgoing.bcc->address == NULL) {
		outgoing.bcc->address =
		    (addresslist *)malloc(sizeof(addresslist));
		outgoing.bcc->address->first = NULL;
		outgoing.bcc->address->last = NULL;
	    }
	    prompt_address(" Bcc: ", outgoing.bcc->address);
	    files_to_fcc(outgoing.bcc->address, &outgoing);
	}
	if (prompt_for_fcc) {
	    if (outgoing.bcc == NULL) {
		outgoing.bcc = new_header(BCC, "Fcc", HEAD_KNOWN,&outgoing);
		outgoing.bcc->address = NULL;
	    }
	    prompt_fcc(" Fcc: ", outgoing.fcc->address);
	}
    }
    if (outgoing.to)
	files_to_fcc(outgoing.to->address, &outgoing);
    if (outgoing.cc)
	files_to_fcc(outgoing.cc->address, &outgoing);
    if (outgoing.bcc)
	files_to_fcc(outgoing.bcc->address, &outgoing);
    cm_set_ind(true);
    mode |= MM_SEND;
    aa = allow_aborts;
    cmseteof();
    cmseter();
    prompt(" Subject: ");
    cmsetrp();
    allow_aborts = true;
    parse(fdbchn(&txtfdb, &cfmfdb, nil), &pv, &used);
    if (used == &txtfdb) {
	outgoing.subject = new_header(SUBJECT, "Subject", HEAD_KNOWN,
				      &outgoing);
	outgoing.subject->string =
	    malloc(strlen(atmbuf)+1);
	strcpy(outgoing.subject->string, atmbuf);
    }
    allow_aborts = aa;
    outgoing.body = NULL;
    ret = get_msg(&outgoing);
    if ((escape_automatic_send && (ret == GET_ESC)) ||
	(control_d_automatic_send && (ret == GET_CTRLD))) { /* send! */
	deliver();
    }
}


cmd_reply(n)
int n;
{
    pval pv; 
    fdb *used;
    int towhom = CMD_ALL;
    int do_reply_many();

    if (mode & MM_READ) {		/* in read mode? */
	/* set default based on reply_to mm var */
	do_reply_one(cf->current,allow_aborts);
	mode &= ~MM_REPLY;
    }
    else {
	free_msg(&outgoing);
	if (!check_cf(O_RDONLY))
	    return;
	parse_sequence ("current",NULL,NULL);
	mode |= MM_REPLY;
	copy_sequence (cf->read_sequence, cf->sequence);
	if (sequence_start(cf->read_sequence)) {
	    do_reply_many();
	}
    }
}

do_reply_many()
{
    char buf[100];
    int ok = false;
    extern int allow_aborts;
    int aa = allow_aborts;
    volatile int doprev = false;

    sprintf(buf," Send reply for message %d to: ", cf->current);
    cmseteof();
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF) {
	    return CMxEOF;
	}
	else
	    doprev = true;
    }
    
    prompt(buf);
    if (doprev) {
	doprev = false;
	prevcmd();
    }
	
    allow_aborts = true;
    cmsetrp();
    if (!ignore (cf->current))
	do_reply_one (cf->current,aa);
    if (!sequence_next(cf->read_sequence))
	mode &= ~MM_REPLY;
}

/*
 * handle_reply_to
 * take care of parse for reply_to_fdb
 * return FALSE if reply session aborted
 */
handle_reply_to (key, allp, aborts)
int key, *allp;
int aborts;
{
    switch(key) {
    case CMD_ALL:
	*allp = true;
	break;
    case CMD_SENDER:
	*allp = false;
	break;
    case CMD_QUIT:
	confirm();
	allow_aborts = aborts;
	if (cf->current < sequence_last (cf->read_sequence))
	    if (!yesno (" Continue replying to remaining messages? ", nil))
		mode &= ~MM_REPLY;
	return (FALSE);			/* skip this message */
    }
    return (TRUE);			/* do this message */
}

/*
 * handle_include:
 * handle the include_fdb result
 */
handle_include (key, inclp)
int key, *inclp;
{
    switch(key) {
    case CMD_INCLUDE:
	*inclp = true;
	break;
    case CMD_NOINCLUDE:
	*inclp = false;
	break;
    }
}

/*
 * reply to the current message
 */
do_reply_one(n, aborts)
{
    int all, include;

    reply_to_fdb._cmdef = reply_all ? "all" : "sender";
    include_fdb._cmdef = reply_insert ? "including" : "not-including";

    parse(fdbchn (&reply_to_fdb, &include_fdb, NULL), &pv, &used);
    if (used == &reply_to_fdb) {
	if (!handle_reply_to (pv._pvkey, &all, aborts)) /* aborted? */
	    return;
	parse(fdbchn (&include_fdb, NULL), &pv, &used);
	handle_include (pv._pvkey, &include);
    }
    else {				/* used == &include_fdb */
	handle_include (pv._pvkey, &include);
	parse (fdbchn (&reply_to_fdb, NULL), &pv, &used);
	if (!handle_reply_to (pv._pvkey, &all))
	    return;
    }
    confirm();
    allow_aborts = aborts;
    do_reply(n, all, include);
}


do_reply(msgno, all, include) 
{
    int ret;
    int gotfrom = false;
    char *name;
    char *do_reply_indent();
    char *safe_strcpy(), *safe_strcat();
    keylist keylist_copy();

    msg = parse_msg(&(cf->msgs[msgno]));
    free_msg(&outgoing);
    set_send_defaults(&outgoing);
    if (outgoing.to == NULL) {
	new_header(TO,"To", HEAD_KNOWN, &outgoing);
	outgoing.to->address = NULL;
    }
    if (outgoing.to->address == NULL) {
	outgoing.to->address = (addresslist *) malloc(sizeof(addresslist));
	outgoing.to->address->first = outgoing.to->address->last = NULL;
    }
    
#ifdef notdef
    if (reply_include_me) {
	if (outgoing.cc == NULL) {
	    new_header(CC,"cc", HEAD_KNOWN, &outgoing);
	    outgoing.cc->address = NULL;
	}
	if (outgoing.cc->address == NULL) {
	    outgoing.cc->address = (addresslist *) malloc(sizeof(addresslist));
	    outgoing.cc->address->first = outgoing.cc->address->last = NULL;
	}
	add_addresslist(outgoing.cc->address, user_name, ADR_ADDRESS);
    }
#endif
    if (!gotfrom && msg->reply_to) {
	name = msg->reply_to->string;
	match_addresses(outgoing.to->address, &name, strlen(name));
	gotfrom = true;

    }
    if (!gotfrom && msg->from) {
	name = msg->from->string;
	match_addresses(outgoing.to->address, &name, strlen(name));
	gotfrom = true;
    }
    if (msg->resent_from) {
	name = msg->resent_from->string;
	if (outgoing.cc == NULL) {
	    new_header(CC,"cc", HEAD_KNOWN, &outgoing);
	    outgoing.cc->address = NULL;
	}
	if (outgoing.cc->address == NULL) {
	    outgoing.cc->address = (addresslist *) malloc(sizeof(addresslist));
	    outgoing.cc->address->first = outgoing.cc->address->last = NULL;
	}
	match_addresses(outgoing.cc->address, &name, strlen(name));
    }
    if (all) {
	if (msg->to || msg->cc) {
	    if (outgoing.cc == NULL) {
		new_header(CC,"cc", HEAD_KNOWN, &outgoing);
		outgoing.cc->address = NULL;
	    }
	    if (outgoing.cc->address == NULL) {
		outgoing.cc->address =
		    (addresslist *) malloc(sizeof(addresslist));
		outgoing.cc->address->first = outgoing.cc->address->last =
		    NULL;
	    }
	    if (msg->to)
		merge_addresses(outgoing.cc->address, msg->to->address);
	    if (msg->cc)
		merge_addresses(outgoing.cc->address, msg->cc->address);
	}
    }

    if (!reply_include_me)
	remove_me (&outgoing);

    if (outgoing.to)
	files_to_fcc(outgoing.to->address, &outgoing);
    if (outgoing.cc)
	files_to_fcc(outgoing.cc->address, &outgoing);
    if (outgoing.bcc)
	files_to_fcc(outgoing.bcc->address, &outgoing);

    if (include) {
	outgoing.body = do_reply_indent(msg->body);
    }
    if (msg->subject) {
	new_header(SUBJECT,"Subject", HEAD_KNOWN, &outgoing); 
	if (ustrncmp(msg->subject->string, "Re:", 3) != 0) {
	    outgoing.subject->string = safe_strcpy("Re: ");
	}
	outgoing.subject->string =
	    safe_strcat(outgoing.subject->string, msg->subject->string, false);
    }
    new_header(IN_REPLY_TO, "In-Reply-To", HEAD_KNOWN, &outgoing);
#ifdef undef				/* XXX */
    if (msg->keywords) {
	new_header(KEYWORDS, "Keywords", HEAD_KNOWN, &outgoing);
	outgoing.keywords->keys = keylist_copy (msg->keywords->keys);
    }
#endif /* undef */
    if (msg->date) {
	outgoing.in_reply_to->string = safe_strcpy("Your message of ");
	outgoing.in_reply_to->string =
	    safe_strcat(outgoing.in_reply_to->string,msg->date->string,false);
    }
    else if (msg->message_id) {
	outgoing.in_reply_to->string =
	    safe_strcat(outgoing.in_reply_to->string,msg->message_id->string,
			false);
    }
    free_msg(msg);

    if (reply_initial_display) {
	if (cmcsb._cmoj == NULL)
	    display_header (stdout, &outgoing, FALSE, FALSE);
	else
	    display_header (cmcsb._cmoj, &outgoing, FALSE, FALSE);
    }

    send_mode(&outgoing);
    mode |= MM_ANSWER;			/* to set ANSWER when appropriate */
    ret = get_msg(&outgoing);
    if ((escape_automatic_send && (ret == GET_ESC)) ||
	(control_d_automatic_send && (ret == GET_CTRLD))) { /* send! */
	deliver();
    }
}

char *
do_reply_indent (msg)
char *msg;
{
    char *p, *cp;
    int lines,i;
    
    if (reply_indent == NULL || strlen(reply_indent) == 0) {
	return(safe_strcpy (msg));
    }
    
    p = msg;
    lines = 0;
    while(p = index(p,'\n'))
	lines++, p++;
    p = malloc(strlen(msg) + lines * strlen(reply_indent) + 2);

    cp = p;
    for(i = 0; i < strlen(reply_indent); i++) /* start off with indent */
        *(cp++) = reply_indent[i];
    for(; *msg != '\0'; cp++, msg++) {
	*cp = *msg;
	if (*msg == '\n' && *(msg+1) != '\0') {
	    for(i = 0; i < strlen(reply_indent); i++)
		*(++cp) = reply_indent[i];
	}
    }
    *cp++ = '\n';
    *cp = '\0';
    return(p);
}

cmd_forward(n)
int n;
{
    pval pv; 
    fdb *used;
    int towhom = CMD_ALL;

    if (mode & MM_SEND) {
	printf ("\
The forward command can not be called while you are currently working on an\n\
outgoing message.\n");
	return;
    }

    if (!check_cf(O_RDONLY))
	return;
    if (mode & MM_READ) {		/* in read mode? */
	/* set default based on reply_to mm var */
	noise("message to");
	do_forward_one(cf->current);
    }
    else {
	parse_sequence ("current",NULL,NULL);
	if (sequence_start(cf->sequence))
	    do_forward_many();
    }
}

do_forward_one(which) {
    int len;
    int ret;
    static addresslist temp = { NULL, NULL };

    parse_addresses(&temp);
    free_msg(&outgoing);
    set_send_defaults(&outgoing);
    if (outgoing.to == NULL) {
	new_header(TO, "To", HEAD_KNOWN, &outgoing);
	outgoing.to->address = NULL;
    }
    if (outgoing.to->address == NULL) {
	outgoing.to->address = (addresslist *)malloc(sizeof(addresslist));
	outgoing.to->address->last = outgoing.to->address->first = NULL;
    }
    merge_addresses(outgoing.to->address, &temp);
    if (outgoing.to)
	files_to_fcc(outgoing.to->address, &outgoing);
    if (outgoing.cc)
	files_to_fcc(outgoing.cc->address, &outgoing);
    if (outgoing.bcc)
	files_to_fcc(outgoing.bcc->address, &outgoing);

    if (msg = parse_msg (&(cf->msgs[which])))
	set_forward_subject ();

    ret = get_msg(&outgoing);
    send_mode(&outgoing);
    if (outgoing.body) {
	if (*outgoing.body) {		/* did user type anything? */
	    if (outgoing.body[strlen(outgoing.body)-1] != '\n')
		outgoing.body = safe_strcat(outgoing.body, "\n", false);
	    outgoing.body = safe_strcat(outgoing.body, FORWLINE, false);
	}
	outgoing.body = safe_strncat(outgoing.body, cf->msgs[which].text,
				     cf->msgs[which].size);
	if ((escape_automatic_send && (ret == GET_ESC)) ||
	    (control_d_automatic_send && (ret == GET_CTRLD))) { /* send! */
	    deliver();
	}
	/* XXX shouldn't set this if sendmail() failed */
	if (!(cf->msgs[which].flags & M_FORWARDED) &&
	    !(cf->flags & MF_RDONLY)) {
	    cf->msgs[which].flags |= (M_FORWARDED|M_MODIFIED);
	    cf->flags |= MF_MODIFIED;	/* we'll have to save that flag */
	}
    }
    else {
	printf("Aborted.\n");
	mode &= ~MM_SEND;
	free_msg(&outgoing);
    }
}
    
/*
 * set the subject string for a forwarded message
 */

set_forward_subject()
{
    char *s, *f, *cp;
    int fl, sl, len = 0;

    if (!(msg->from && msg->subject))
	return;

    f = msg->from->string;
    fl = strlen (f);

    s = msg->subject->string;
    sl = strlen (s);

    new_header (SUBJECT, "Subject", HEAD_KNOWN, &outgoing);

    outgoing.subject->string = malloc (sl + fl + 4 + 4);
    sprintf (outgoing.subject->string, "[%.*s: %.*s]", fl, f, sl, s);
}

do_forward_many() 
{
    char *forwardees = NULL;
    char *head = NULL;
    char *forward_header(), *forward_banner();
    int ret;

    free_msg(&outgoing);
    set_send_defaults(&outgoing);
    if (outgoing.to == NULL) {
	outgoing.to = new_header(TO, "To", HEAD_KNOWN, &outgoing);
	outgoing.to->address = NULL;
    }
    if (outgoing.to->address == NULL) {
	outgoing.to->address = (addresslist *)malloc(sizeof(addresslist));
	outgoing.to->address->first = NULL;
	outgoing.to->address->last = NULL;
    }
    send_mode(&outgoing);
    prompt_address(" To: ", outgoing.to->address);
    files_to_fcc(outgoing.to->address, &outgoing);

    ret = get_msg(&outgoing);

    if (msg = parse_msg (&cf->msgs[cf->current]))
	set_forward_subject ();

    /*
     * If there's only one message to forward, we don't include a
     * summary of the forwarded messages.
     */
    if (sequence_first(cf->sequence) == sequence_last(cf->sequence)) {
	message *m = &cf->msgs[cf->current];

	if (*outgoing.body) {
	    if (outgoing.body[strlen(outgoing.body)-1] != '\n')
		outgoing.body = safe_strcat(outgoing.body, "\n", false);
	    outgoing.body = safe_strcat(outgoing.body, FORWLINE, false);
	}
	outgoing.body = safe_strncat(outgoing.body, m->text, m->size);
    }
    else {				/* more than one message */
	int i = 1;
	do {
	    message *m = &cf->msgs[cf->current];
	    head = safe_strcat(head, forward_header (m, i), false);
	    head = safe_strcat(head, "\n", false);
	    if (forwardees && forwardees[strlen(forwardees)-1] !='\n')
		forwardees = safe_strcat(forwardees,"\n", false);
	    forwardees = safe_strcat(forwardees, forward_banner(i), false);
	    forwardees = safe_strncat(forwardees, m->text, m->size);
	    ++i;
	} while (sequence_next (cf->sequence));
	head = safe_strcat(head, forwardees, false);
	if (outgoing.body) {
	    if (outgoing.body[strlen(outgoing.body)-1] != '\n')
		outgoing.body = safe_strcat(outgoing.body, "\n", false);
	    outgoing.body = safe_strcat(outgoing.body, FORWLINE, false);
	}
	outgoing.body = safe_strcat(outgoing.body, head, false);
	free(forwardees);
	free(head);
	if ((escape_automatic_send && (ret == GET_ESC)) ||
	    (control_d_automatic_send && (ret == GET_CTRLD))) { /* send! */
	    deliver ();
	}
    }
}


char *
forward_banner(n) {
    static char buf[50];
    sprintf(buf,"\nMessage %d -- *********************\n",n);
    return(buf);
}

char *
forward_header(m, n)
message *m;				/* which message */
int n;					/* position in forwarded message */
{
    static buffer line;
    char *cp = line;
    char *p;

    (void) sprintf (cp, "%4d) %s ", n, hdate (m->date));
    cp += strlen (cp);
    sprintf (cp, "%-12.12s ", htext ("from", m->text));
    cp += strlen (cp);
    sprintf (cp, "%-.32s (%ld chars)", htext ("subject", m->text), m->size);
    cp += strlen (cp);
    return(line);
}

cmd_remail(n)
int n;
{
    pval pv; 
    fdb *used;
    int towhom = CMD_ALL;
    addresslist to;

    if (mode & MM_SEND) {
	printf ("\
The remail command can not be called while you are currently working on an\n\
outgoing message.\n");
	return;
    }
    to.last = to.first = NULL;

    if (mode & MM_READ) {		/* in read mode? */
	/* set default base on reply_to mm var */
	noise("message to");
	parse_addresses(&to);
	while (to.first == NULL)	/* have to have one */
	    prompt_address(" To: ", &to);
	do_remail_one(cf->current, &to);
	free_addresslist(&to);
    }
    else {
	if (!check_cf(O_RDONLY))
	    return;
	parse_sequence ("current",NULL,NULL);
	if (sequence_start(cf->sequence)) {
	    do_remail_many();
	}
    }
}

do_remail_one(which, to) 
int which;
addresslist *to;
{
    int len;
    static addresslist temp = { NULL, NULL };

    /* not a memory leak unless parse_msg starts to malloc */
    msg = parse_msg(&(cf->msgs[which]));
    if (msg->resent_to) {
	free_header(msg, RESENT_TO);
    }
    new_header(RESENT_TO, "Resent-To", HEAD_KNOWN, msg);
    msg->resent_to->address = (addresslist *) malloc(sizeof(addresslist));
    msg->resent_to->address->last = msg->resent_to->address->first =  NULL;
	merge_addresses(msg->resent_to->address, to);
    if (msg->resent_date) 
	free_header(msg, RESENT_DATE);
    new_header(RESENT_DATE, "Resent-Date", HEAD_KNOWN, msg);
    msg->resent_date->string = safe_strcpy(rfctime(time(0)));
    if (msg->resent_from) 
	free_header(msg, RESENT_FROM);
    new_header(RESENT_FROM, "Resent-From", HEAD_KNOWN, msg);
					/* check from variable. */
    msg->resent_from->string = safe_strcpy(create_sender());
    len = 0;
    if (msg->to)
        files_to_fcc(msg->to->address, msg);
    sendmail(msg);
}
    
do_remail_many() 
{
    addresslist to;
    
    to.last = to.first = NULL;
    prompt_address(" To: ", &to);
    do {
	if (!ignore(cf->current))
	    do_remail_one(cf->current, &to);
    } while (sequence_next (cf->sequence));
}


int
cmd_bug (n)
int n;
{
    static fdb txtfdb = { _CMTXT };
    static fdb cfmfdb = { _CMCFM , CM_SDH};
    char *name;
    int ret;
    addresslist tmp;
    headers *h;
    int aa;
    volatile int doprev = false;

    cm_set_ind(false);			/* no indirections here */

    tmp.first = tmp.last = NULL;
    free_msg(&outgoing);
    set_send_defaults(&outgoing);
    confirm();
    send_mode(&outgoing);
    cm_set_ind(false);
    name = BUGSTO;
    match_addresses(outgoing.to->address, &name, strlen(name));
    merge_addresses(outgoing.to,&tmp);
    aa = allow_aborts;
    cmseteof();
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF) {
	    return CMxEOF;
	}
	else
	    doprev = true;
    }
    
    prompt(" Subject: ");
    if (doprev) {
	doprev = false;
	prevcmd();
    }
    cmsetrp();
    allow_aborts = true;
    parse(fdbchn(&txtfdb, &cfmfdb, nil), &pv, &used);
    if (used == &txtfdb) {
	outgoing.subject = new_header(SUBJECT, "Subject", HEAD_KNOWN,
				      &outgoing);
	outgoing.subject->string =
	    malloc(strlen(atmbuf)+1);
	strcpy(outgoing.subject->string, atmbuf);
    }
    allow_aborts = aa;
    h = new_header(USER_HEADERS, "Bug-Report", HEAD_UNKNOWN, &outgoing);
    h->string = malloc(strlen("Bug in ")+strlen(mm_version)+
		       strlen(OStype)+3+1);
    sprintf(h->string,"Bug in %s (%s)", mm_version, OStype);
    
    outgoing.body = NULL;
    ret = get_msg(&outgoing);
    if ((escape_automatic_send && (ret == GET_ESC)) ||
	(control_d_automatic_send && (ret == GET_CTRLD))) { /* send! */
	deliver ();
    }
}


cmd_insert (n)
int n;
{
  char *fname, *parse_input_file();
  char *text, *read_from_temp();

  if (!(mode & MM_SEND)) {
    fprintf (stderr, "\nNot in send mode.  Cannot insert file.\n");
    return;
  }
  noise ("from file");
  fname = parse_input_file (NULL, NULL, false);
  confirm();
  if ((text = read_from_temp(fname)) == NULL) {
    free (fname);
    return;
  }
  free (fname);
  if (outgoing.body == NULL) {
    outgoing.body = (char *) malloc (strlen(text)+1);
    outgoing.body[0] = '\0';
  }
  else
    outgoing.body = (char *) safe_realloc (outgoing.body, 
					 strlen(outgoing.body)+strlen(text)+1);
  strcat (outgoing.body, text);
  free (text);
}



mail_msg *
get_outgoing() {
  return (&outgoing);
}


set_outgoing (m) mail_msg *m; {
  free_msg (&outgoing);
  bcopy ((char *) m, (char *) &outgoing, sizeof(mail_msg));
}

/*
 * remove self from recipient headers.
 * invoked by "reply" command if reply-include-me is false.
 */

remove_me (msg)
mail_msg *msg;
{
    addr_unit au;
    addresslist al;

    /*
     * construct a trivial addr_unit consisting of our username.
     * we should really use a user-specifiable pattern instead.
     */
    au.type = ADR_ADDRESS;
    au.data = user_name;
    au.next = au.prev = nil;

    /*
     * build a trivial address list
     */
    al.first = al.last = &au;

    /*
     * remove ourself from the To: and cc: headers.
     */
    if (msg->to)
	remove_addr (msg->to->address, &al);
    if (msg->cc)
	remove_addr (msg->cc->address, &al);
}

int
cmd_smail (n)
int n;
{
    static fdb txtfdb = { _CMTXT };
    static fdb cfmfdb = { _CMCFM , CM_SDH};
    int ret;				/* return value */
    int aa;
    static addresslist temp= { nil, nil };
    extern int prompt_rcpt_always, prompt_for_cc, prompt_for_fcc;
    extern int prompt_for_bcc;

    cm_set_ind(false);			/* no indirections here */

    noise("message to");
    free_addresslist(&temp);
    parse_addresses(&temp);
    free_msg(&outgoing);
    set_send_defaults(&outgoing);
    if (outgoing.to == NULL)
	outgoing.to = new_header(TO, "To", HEAD_KNOWN, &outgoing);
    if (outgoing.to->address == NULL) {
	outgoing.to->address = (addresslist *)malloc(sizeof(addresslist));
	outgoing.to->address->first = nil;
	outgoing.to->address->last = nil;
    }
    send_mode(&outgoing);
    if (temp.first) {
	merge_addresses(outgoing.to->address, &temp);
    }
    if (outgoing.to)
	files_to_fcc(outgoing.to->address, &outgoing);
    if (outgoing.cc)
	files_to_fcc(outgoing.cc->address, &outgoing);
    if (outgoing.bcc)
	files_to_fcc(outgoing.bcc->address, &outgoing);
    cm_set_ind(true);
    mode |= MM_SEND;
    outgoing.body = NULL;
    ret = get_msg(&outgoing);
    deliver();
}


