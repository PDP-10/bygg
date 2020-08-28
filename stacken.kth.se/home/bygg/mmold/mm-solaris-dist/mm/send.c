/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/send.c,v 2.9 1997/10/21 19:33:32 howie Exp $";
#endif

/*
 * send.c - support for the mm send command
 */ 
#include "mm.h"
#include "cmds.h"
#include "message.h"
#include "parse.h"

#define empty_addr(type) ((current->type == NULL) || \
			  (current->type->address == NULL) || \
			  (current->type->address->first == NULL))

extern string default_from, default_reply_to;

int get_msg();

char *reserved_fields[] = {
    "Sender",
    "Date",
    "From",
    nil,
};

static int edited;

char *malloc(), *realloc(), *safe_strcpy(), *safe_free();
headers *new_header();

mail_msg *current;

prompt_address(pstr, a) char *pstr; addresslist *a; 
{
    addresslist tmp;
    extern int allow_aborts;
    int aa;
    volatile int doprev = false;

    aa = allow_aborts;
    allow_aborts = true;
    tmp.first = tmp.last = NULL;
    cm_set_ind(false);
    cmseteof();
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF) {
	    return CMxEOF;
	}
	else
	    doprev = true;
    }
    
    prompt(pstr);
    if (doprev) {
	doprev = false;
	prevcmd();
    }

    cmsetrp();
    parse_addresses(&tmp);
    merge_addresses(a,&tmp);
    cm_set_ind(true);
    allow_aborts = aa;
}

prompt_fcc(pstr, a)
char *pstr;
addresslist *a; 
{
    addresslist tmp;
    extern int allow_aborts;
    int aa;
    volatile int doprev = false;
    
    aa = allow_aborts;
    allow_aborts = true;
    tmp.first = tmp.last = NULL;
    cm_set_ind(false);
    cmseteof();
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF) {
	    return CMxEOF;
	}
	else
	    doprev = true;
    }
    
    prompt(pstr);
    if (doprev) {
	doprev = false;
	prevcmd();
    }
    cmsetrp();
    cmd_fcc(CMD_FCC);
    merge_addresses(a,&tmp);
    cm_set_ind(true);
    allow_aborts = aa;
}

int
deliver ()
{
    if (current) {
	if (empty_addr (to) && empty_addr (cc) && empty_addr (bcc) &&
	    empty_addr (fcc)) {
	    if (current->to == NULL) {
		current->to = new_header(TO, "To", HEAD_KNOWN,current);
		current->to->address = NULL;
	    }
	    if (current->to->address == NULL) {
		current->to->address =
		    (addresslist *) malloc(sizeof(addresslist));
		current->to->address->first = NULL;
		current->to->address->last = NULL;
	    }
	    while (current->to->address->first == NULL)	/* gotta have it! */
		prompt_address(" To: ", current->to->address);
	    files_to_fcc(current->to->address, current);
	}
	if (mode & (MM_ANSWER|MM_REPLY)) { /* mark message as answered */
	    if (!(cf->msgs[cf->current].flags & M_ANSWERED) &&
		!(cf->flags & MF_RDONLY)) {
		cf->msgs[cf->current].flags |= (M_ANSWERED|M_MODIFIED);
		cf->flags |= MF_MODIFIED; /* we'll have to write out the file*/
	    }
	}
	if (sendmail(current))
	    free_msg(current);
	else {
	    fprintf (stderr, "\
Could not send mail.  Use SAVE-DRAFT to save a copy of this message\n");
	    return;
	}
    }
    current = NULL;
    mode &= ~(MM_SEND|MM_ANSWER);
}

cmd_to(n)
int n;
{
    addresslist temp;

    temp.first = temp.last = NULL;
    parse_addresses(&temp);
    if (!current->to) {
	current->to = new_header(TO, "To", HEAD_KNOWN,current);
	current->to->address = (addresslist *)malloc(sizeof (addresslist));
	current->to->address->first = current->to->address->last = NULL;
    }
    merge_addresses(current->to->address, &temp);
    files_to_fcc(current->to->address, current);

}

cmd_cc(n)
int n;
{
    addresslist temp;

    temp.first = temp.last = NULL;
    parse_addresses(&temp);
    if (!current->cc) {
	current->cc = new_header(CC, "Cc", HEAD_KNOWN,current);
	current->cc->address = (addresslist *)malloc(sizeof(addresslist));
	current->cc->address->first = current->cc->address->last = NULL;
    }
    merge_addresses(current->cc->address, &temp);
    files_to_fcc(current->cc->address, current);
}

cmd_bcc(n)
int n;
{
    addresslist temp;

    temp.first = temp.last = NULL;
    parse_addresses(&temp);
    if (!current->bcc) {
	current->bcc = new_header(BCC, "Bcc", HEAD_KNOWN,current);
	current->bcc->address = (addresslist *)malloc(sizeof(addresslist));
	current->bcc->address->first = current->bcc->address->last = NULL;
    }
    merge_addresses(current->bcc->address, &temp);
    files_to_fcc(current->bcc->address, current);
}

cmd_fcc(n)
int n;
{
    char **filelist, **fl, **parse_filelist();

    filelist = parse_filelist(0,0,"filename",false);

    if (!current->fcc) {
	current->fcc = new_header(FCC, "Fcc", HEAD_KNOWN,current);
	current->fcc->address = (addresslist *)malloc(sizeof(addresslist));
	current->fcc->address->first = current->fcc->address->last = NULL;
    }
    for(fl = filelist; fl && *fl; fl++)
	add_addresslist(current->fcc->address, *fl, ADR_FILE);
}


static int esc = FALSE;
char *
escact(text, modified, ret) char *text; int *modified, *ret; 
{
    *ret = TRUE;
    esc = TRUE;
    return(text);
}

int user_aborted = false;

char *
abortaction (text, modified, ret)
char *text;
int *modified, *ret;
{
    csb oldcsb;
    if (control_n_abort == SET_ASK) {
	int cmdbuf[64];	char atmbuf[64], wrkbuf[64];
	save_parse_context ();
	oldcsb = cmcsb;
	cmbufs (cmdbuf, sizeof cmdbuf / sizeof cmdbuf[0],
		atmbuf, sizeof atmbuf, wrkbuf, sizeof wrkbuf);
	cmact(nil);
	*modified = false;
	*ret = user_aborted = yesno ("Abort? ", "yes");
	cmcsb = oldcsb;
	restore_parse_context ();
	cmcsb._cmcol = 0;		/* XXX */
    }
    else if (control_n_abort == SET_NEVER)
	cmsti1('\016', 0);		/* XXX not quite right */
    else
	*ret = user_aborted = true;	/* set control-n-abort always */

    return (text);
}

char *
editaction(text, modified, ret) char *text; int *modified, *ret; {
  mail_msg *m, *get_outgoing();

  cmxprintf ("^E\n");
  *modified = TRUE;
  *ret = TRUE;
  m = get_outgoing();
  if (m->body)
    free (m->body);
  m->body = safe_strcpy(text);
  edit_outgoing(FALSE);
  edited = true;
  return (safe_strcpy(m->body));
}


/*
 * GETMSG:
 * parse for a message (invoking editor when necessary).
 * Returns:
 *	GET_ESC:  	user exited paragraph parse with ESC
 *	GET_CTRLD:	user exited paragraph parse with Control-D
 *	GET_EDIT:	editor was invoked.
 *	GET_ABORT:	user aborted.
 */

int
get_msg(outg)
mail_msg *outg; 
{
    static para_actions msg_actions[] = {
        { '\033', escact },		/* ESC handler */
	{ '\005', editaction },		/* ^E handler */
	{ '\016', abortaction },	/* ^N handler */
        { NULL, NULL }
    };
    static para_data pd;
    static fdb parafdb = { _CMPARA, CM_NEOF|PARA_DEF, NULL, (pdat) &pd, NULL,
				NULL, NULL };
    pval parseval;
    fdb *used;
    int i;

    /* 
     * set up ^E and ^N actions appropriately
     */
    for (i = 0; msg_actions[i].actionchar; i++)
	if (msg_actions[i].actionchar == '\005')
	    msg_actions[i].actionfunc = control_e_editor ? editaction : nil;
	else if (msg_actions[i].actionchar == '\016')
	    msg_actions[i].actionfunc = control_n_abort ? abortaction : nil;
      
    if (use_editor_always) {
      edit_outgoing (FALSE);		/* don't parse keyword */
      return (GET_EDIT);
    }
    pd.actions = msg_actions;
    pd.buf = outg->body; 

#ifdef undef
    if (terse_text_prompt)
	cmxprintf (" Msg:\n");
    else
	cmxprintf(" Message (End with CTRL/D or ESC\n\
  Use CTRL/B to insert a file, CTRL/E to enter editor, CTRL/F to run text\n\
  through a filter, CTRL/K to redisplay message, CTRL/L to clear screen and\n\
  redisplay, CTRL/N to abort, CTRL/P to run a program and insert output.):\n");

    if (pd.buf != NULL)
      redisplast (pd.buf, terse_text_prompt ? 1 : 4); /* # of lines in msg */
#endif

    if (terse_text_prompt)
	prompt_for_text (" Msg:\n", pd.buf);
    else
	prompt_for_text (" Message (End with CTRL/D or ESC\n\
  Use CTRL/B to insert a file, CTRL/E to enter editor, CTRL/F to run text\n\
  through a filter, CTRL/K to redisplay message, CTRL/L to clear screen and\n\
  redisplay, CTRL/N to abort, CTRL/P to run a program and insert output.):\n",
			 pd.buf);

    esc = FALSE;
    edited = FALSE;
    user_aborted = false;
    parse(&parafdb,&parseval,&used);
    if (parseval._pvpara != NULL) {
      if (outg->body)
	free (outg->body);
      outg->body = safe_strcpy(parseval._pvpara);
    }
    if (user_aborted)
      return (GET_ABORT);
    if (esc)
      return (GET_ESC);
    if (edited)
      return (GET_EDIT);
    return (GET_CTRLD);
}

cmd_text(n)
int n;
{
    int ret;

    confirm();
    ret = get_msg(current);
    if ((escape_automatic_send && (ret == GET_ESC)) ||
	(control_d_automatic_send && (ret == GET_CTRLD))) { /* send! */
        deliver();
    }
}


cmd_erase(n)
int n;
{
    pval pv;
    fdb *used;
    int which;

    noise("message field");
    parse(fdbchn(&hdr_cmd_fdb,&erase_cmd_fdb, nil),&pv, &used);
    which = (int) pv._pvkey;
    switch(which) {
    case CMD_ALL:
	confirm();
	free_msg(current);
	break;
    case CMD_BCC:
	confirm();
	free_header(current,BCC);
	break;
    case CMD_CC:
	confirm();
	free_header(current,CC);
	break;
    case CMD_FCC:
	confirm();
	free_header(current,FCC);
	break;
    case CMD_FROM:
	confirm();
	free_header(current,FROM);
	break;
    case CMD_REPLY_TO:
	confirm();
	free_header(current,REPLY_TO);
	break;
    case CMD_IN_REPLY_TO:
	confirm();
	free_header(current,IN_REPLY_TO);
	break;
    case CMD_SUBJECT:
	confirm();
	free_header(current,SUBJECT);
	break;
    case CMD_TEXT:
	confirm();
	free_body();
	break;
    case CMD_TO:
	confirm();
	free_header(current,TO);
	break;
    case CMD_USER_HEADER:
        {
	    static char *uh;
	    static fdb keyfdb = { _CMKEY };
	    static fdb fldfdb = { _CMFLD, CM_SDH, nil, nil, "Field name" };
	    if (uh) {
		free(uh);
		uh = nil;
	    }
	    if (user_headers) {
		keyfdb._cmdat = (pdat) keylist_to_keytab(user_headers);
		parse(fdbchn(&keyfdb, &fldfdb,nil),&pv,&used);
	    }
	    else 
		parse(fdbchn(&fldfdb,nil), &pv,&used);
	    uh = safe_strcpy(atmbuf);
	    confirm();
	    free_user_header(current,uh);
	    free(uh);
	    uh = nil;
	}
	break;
    }
    
}

free_msg(msg)
mail_msg *msg;
{
    while(msg->headers) {
	headers *t = msg->headers;
	free_header(msg,t->type);
    }
    if (msg->body) {
	free(msg->body);
    }
    msg->body = NULL;
}

free_header(msg,type) 
mail_msg *msg;
int type;
{
    headers *h = msg->headers, *h1 = NULL;
    while(h) {
	if (h->type == type) {
	    if (h1)
		h1->next = h->next;
	    else 
		msg->headers = h->next;
	    
	    h->next = NULL;
	    h->name = safe_free(h->name);
	    switch(type) {
	    case TO:
		if (h)  {
		    free_addresslist(h->address);
		    free(h->address);
		    h = (headers *) safe_free(h);
		}
		msg->to = NULL;
		break;
	    case CC:
		if (h)  {
		    free_addresslist(h->address);
		    free(h->address);
		    h = (headers *) safe_free(h);
		}
		msg->cc = NULL;
		break;
	    case BCC:
		if (h)  {
		    free_addresslist(h->address);
		    free(h->address);
		    h = (headers *) safe_free(h);
		}
		msg->bcc = NULL;
		break;
	    case FCC:
		if (h)  {
		    free_addresslist(h->address);
		    free(h->address);
		    h = (headers *) safe_free(h);
		}
		msg->fcc = NULL;
		break;
	    case RESENT_TO:
		if (h)  {
		    free_addresslist(h->address);
		    free(h->address);
		    h = (headers *) safe_free(h);
		}
		msg->resent_to = NULL;
		break;
	    case FROM:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->from = NULL;
		break;
	    case DATE:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->date = NULL;
		break;
	    case SUBJECT:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->subject = NULL;
		break;
	    case REPLY_TO:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->reply_to = NULL;
		break;
	    case IN_REPLY_TO:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->in_reply_to = NULL;
		break;
	    case RESENT_DATE:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->resent_date = NULL;
		break;
	    case RESENT_FROM:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->resent_from = NULL;
		break;
	    case SENDER:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->sender = NULL;
		break;
	    case REFERENCES:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->references = NULL;
		break;
	    case COMMENTS:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->comments = NULL;
		break;
	    case MESSAGE_ID:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->message_id = NULL;
		break;
	    case KEYWORDS:
		if (h) {
		    keylist free_keylist();
		    h->keys = free_keylist(h->keys);
		    h = (headers *) safe_free(h);
		}
		msg->keywords = NULL;
		break;
	    case ENCRYPTED:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->encrypted = NULL;
		break;
	    case RECEIVED:
		if (h) {
		    h->string = safe_free(h->string);
		    h = (headers *) safe_free(h);
		}
		msg->received = NULL;
		break;
	    }
	    if (h1)
		h = h1->next;
	    else 
		h = msg->headers;
	}
	else {
	    h1 = h;
	    h = h->next;
	}
    }
    if (msg->headers == NULL)
	msg->last = NULL;
    else for(h = msg->headers; h->next != NULL; h = h->next);
    msg->last = h;
}

free_user_header(msg,type) 
mail_msg *msg;
char *type;
{
    headers *h = msg->headers, *h1 = NULL;
    while(h) {
	if (h->type == USER_HEADERS && ustrcmp(h->name,type) == 0) {
	    if (h1)
		h1->next = h->next;
	    else 
		msg->headers = h->next;
	    
	    h->next = NULL;
	    h->name = safe_free(h->name);
	    h->string = safe_free(h->string);
	    h = (headers *) safe_free(h);

	    if (h1)
		h = h1->next;
	    else 
		h = msg->headers;
	}
	else {
	    h1 = h;
	    h = h->next;
	}
    }
    if (msg->headers == NULL)
	msg->last = NULL;
    else for(h = msg->headers; h->next != NULL; h = h->next);
    msg->last = h;
}

cmd_continue(n)
int n;
{
    noise("sending message");
    confirm();
    if (current) {
	mode |= MM_SEND;
    }
    else {
	cmerr("No current Message");
    }
}

cmd_remove(n)
int n;
{
    addresslist a;

    a.first = a.last = NULL;
    noise("address");
    parse_addresses(&a);
    if (current->to)
	remove_addr(current->to->address,&a);
    if (current->cc)
	remove_addr(current->cc->address,&a);
    if (current->bcc)
	remove_addr(current->bcc->address,&a);
    if (current->fcc)
	remove_addr(current->fcc->address,&a);
    free_addresslist(&a);
}    

cmd_subject(n)
int n;
{
    char *t; 

    t = parse_text("new subject", NULL);
    if (current->subject) {
	free_header(current,SUBJECT);
    }
    current->subject = new_header(SUBJECT, "Subject", HEAD_KNOWN,current);
    current->subject->string = safe_strcpy(t);
}

cmd_from(n)
int n;
{
    char *t, *create_sender();

    t = parse_text("new from field", NULL);
    if (current->from == NULL) {
	current->from = new_header(FROM, "From", HEAD_KNOWN,current);
    }
    else
	free(current->from->string);
    current->from->string = safe_strcpy(t);
    if (current->reply_to == NULL) {	/* no reply-to field yet */
        current->reply_to = new_header(REPLY_TO, "Reply-To", 
				       HEAD_KNOWN,current);
	if (strlen(default_reply_to) != 0) {
	    current->reply_to->string = malloc (strlen(default_reply_to)+1);
	    strcpy (current->reply_to->string, default_reply_to);
	}
	else {
	    current->reply_to->string = safe_strcpy(create_sender());
	}
    }
}

cmd_reply_to(n)
int n;
{
    char *t; 

    t = parse_text("new address to have replies go to",NULL);
    if (current->reply_to) {
	free_header(current,REPLY_TO);
    }
    current->reply_to = new_header(REPLY_TO, "Reply-To", HEAD_KNOWN,current);
    current->reply_to->string = safe_strcpy(t);
}

cmd_in_reply_to(n)
int n;
{
    char *t; 

    t = parse_text("Line of text",NULL);
    if (current->in_reply_to) {
	free_header(current,IN_REPLY_TO);
    }
    current->in_reply_to = new_header(IN_REPLY_TO, "In-Reply-To", HEAD_KNOWN,
				      current);
    current->in_reply_to->string = safe_strcpy(t);
}


send_mode(msg) mail_msg *msg; {
    mode |= MM_SEND;
    current = msg;
}

headers *
new_header(type, name, flags, current)
int type, flags;
char *name;
mail_msg *current;
{
    headers *x;
    x = (headers *)malloc(sizeof(headers));
    if (!x)
	panic ("Out of memory");

    x->type = type;
    x->name = safe_strcpy(name);
    x->flags = flags;
    x->next = NULL;
    x->string = NULL;
    x->address = NULL;
    x->keys = NULL;
    if (current->last)
	current->last = current->last->next = x;
    else
	current->headers = current->last = x;

    switch(type) {
    case TO:
        current->to = x;
        break;
    case CC:
        current->cc = x;
        break;
    case BCC:
        current->bcc = x;
        break;
    case FCC:
        current->fcc = x;
        break;
    case FROM:
        current->from = x;
        break;
    case DATE:
        current->date = x;
        break;
    case SUBJECT:
        current->subject = x;
        break;
    case REPLY_TO:
        current->reply_to = x;
        break;
    case IN_REPLY_TO:
        current->in_reply_to = x;
        break;
    case RESENT_TO:
        current->resent_to = x;
        break;
    case RESENT_DATE:
        current->resent_date = x;
        break;
    case RESENT_FROM:
        current->resent_from = x;
        break;
    case SENDER:
        current->sender = x;
        break;
    case REFERENCES:
        current->references = x;
        break;
    case COMMENTS:
        current->comments = x;
        break;
    case MESSAGE_ID:
        current->message_id = x;
        break;
    case KEYWORDS:
        current->keywords = x;
        break;
    case ENCRYPTED:
        current->encrypted = x;
        break;
    }
    return(x);
}

free_body() {
    current->body = safe_free(current->body);
}

set_send_defaults(msg)
mail_msg *msg;
{
    char **list;
    addresslist temp;
    char *pname;
    char *create_sender();
    int need_replyto = FALSE;

    if (msg->from == NULL) {
	new_header(FROM,"From", HEAD_KNOWN, msg);
	if (strlen(default_from) != 0) {
	    msg->from->string = malloc (strlen(default_from)+1);
	    strcpy (msg->from->string, default_from);
	    need_replyto = TRUE;
	}
	else {
	    pname = personal_name;
	    if (user_name[0] != '\0') {
		if (pname == NULL || pname[0] == '\0')
		    pname = real_personal_name;
		if (pname) {
		    if (mailhostname) {
			msg->from->string = malloc(strlen(pname)+
						   strlen(user_name)+
						   strlen(mailhostname) +5);
			sprintf(msg->from->string, "%s <%s@%s>", pname, 
				user_name, mailhostname);
		    }
		    else {
			msg->from->string = malloc(strlen(pname)+
						   strlen(user_name)
						   +4);
			sprintf(msg->from->string, "%s <%s>", 
				pname, user_name);
		    }
		}
		else {
		    if (mailhostname) {
			msg->from->string = malloc(strlen(user_name) +
						   strlen(mailhostname) +4);
			sprintf(msg->from->string, "<%s@%s>", user_name,
				mailhostname);
		    }
		    else {
			msg->from->string = malloc(strlen(user_name) +3);
			sprintf(msg->from->string, "<%s>", user_name);
		    }
		}
	    }
	}
    }
    if ((msg->reply_to == NULL) && 
	((default_reply_to[0] != '\0') || need_replyto)) {
	new_header(REPLY_TO, "Reply-To", HEAD_KNOWN, msg);
	if (default_reply_to[0] != '\0') {
	    msg->reply_to->string = malloc (strlen(default_reply_to)+1);
	    strcpy (msg->reply_to->string, default_reply_to);
	}
	else {
	    msg->reply_to->string = safe_strcpy(create_sender());
	}
    }
    temp.first = temp.last = NULL;	/* XXX what's this used for? */
    if (msg->to == NULL) {
	new_header(TO, "To", HEAD_KNOWN, msg);
	msg->to->address = (addresslist *) malloc(sizeof(addresslist));
	msg->to->address->first = msg->to->address->last = NULL;
    }
    if (msg->cc == NULL) {
	new_header(CC, "Cc", HEAD_KNOWN, msg);
	msg->cc->address = (addresslist *) malloc(sizeof(addresslist));
	msg->cc->address->first = msg->cc->address->last = NULL;
	merge_addresses(msg->cc->address, &default_cc_list);
    }

    if (msg->bcc == NULL) {
	new_header(BCC, "Bcc", HEAD_KNOWN, msg);
	msg->bcc->address = (addresslist *) malloc(sizeof(addresslist));
	msg->bcc->address->first = msg->bcc->address->last = NULL;
	merge_addresses(msg->bcc->address, &default_bcc_list);
    }
    if (msg->fcc == NULL) {
	keylist df = default_fcc_list;
	new_header(FCC, "Fcc", HEAD_KNOWN, msg);
	msg->fcc->address = (addresslist *) malloc(sizeof(addresslist));
	msg->fcc->address->first = msg->fcc->address->last = NULL;
	while (df && *df) {
	    add_addresslist(msg->fcc->address, *df, ADR_FILE);
	    df++;
	}
    }
    read_header_file(msg);
}    

outgoing_keyword(key)
char *key;
{
    keylist add_keyword();
    if (current->keywords == nil) {
	new_header(KEYWORDS, "Keywords", HEAD_KNOWN, current);
    }
    current->keywords->keys = add_keyword(key, current->keywords->keys);
}

unoutgoing_keyword(key)
char *key;
{
    keylist rem_keyword();
    current->keywords->keys = rem_keyword(key,  current->keywords->keys);
}

cmd_user_header(n)
int n;
{
    char *uh;
    char *data;
    static fdb fldfdb = { _CMFLD, CM_SDH, nil, nil, "Field name" };
    static fdb keyfdb = { _CMKEY };
    headers *h;
    
    if (user_headers) {
	keyfdb._cmdat = (pdat) keylist_to_keytab(user_headers);
	parse(fdbchn(&keyfdb, &fldfdb,nil),&pv,&used);
    }
    else 
	parse(fdbchn(&fldfdb,nil), &pv,&used);
    uh = safe_strcpy(atmbuf);
    data = parse_text("Line of Text", nil);
    if (reserved_field(uh)) {	/* make sure it is not reserved */
	char *name1;
	name1 = malloc(strlen(uh)+3);
	sprintf(name1,"X-%s", uh); /* reserved - add an "X-" to it */
	free(uh);
	uh = name1;
    }
    h = new_header(USER_HEADERS, uh, HEAD_UNKNOWN, current);
    h->string = safe_strcpy(data);
    free(uh);
}

char*
read_file(fname)
char *fname;
{
    int fd;
    struct stat sbuf;
    char *newtext;

    if (stat(fname, &sbuf) == -1) {	/* get file length */
	perror(fname);
	return (NULL);
    }
    if ((fd = open(fname, O_RDONLY,0)) < 0) { /* open file for read */
	perror(fname);
	return (NULL);
    }
    newtext = (char *) malloc (sbuf.st_size+2);
    if (read(fd, newtext, sbuf.st_size) != sbuf.st_size) {
	perror(fname);
	close (fd);
	free (newtext);
	return (NULL);
    }
    close (fd);
    if (sbuf.st_size > 1) 
	if (newtext[sbuf.st_size-1] != '\n')
	    newtext[sbuf.st_size++] = '\n';
    newtext[sbuf.st_size] = '\0';		/* null terminate the text */
    return (newtext);
}

read_header_file(msg) 
mail_msg *msg;
{
    char *contents, *cp, *skipheader(),*name, *cp1,*bp,*text;
    headers *h;

    if (strcmp(header_options_file, "/dev/null") == 0) 
	return;

    contents = read_file(header_options_file); /* read the file */
    if (contents == nil)
	return;

    cp1 = contents;
    while(1) {
	if (cp1 == nil || *cp1 == '\0')
	    break;
	cp = cp1;
	cp1 = skipheader(cp);		/* find the next header */
	if (cp1 == nil)
	    cp1 = cp + strlen(cp);
	while(isspace(*cp) && cp != cp1) cp++;
	if (cp == cp1)
	    continue;
	bp = index(cp, ':');
	if (bp == 0 || bp > cp1) {
	    fprintf(stderr,"%%Illegal User Header in %s:\n '",
		    header_options_file);
	    fwrite(cp, sizeof (char), (cp1 - cp) -1, stderr);
	    fprintf(stderr,"'\n");
	}
	else {
	    name = malloc(bp - cp + 1);	/* get the field name */
	    strncpy(name, cp, bp - cp);
	    name[bp-cp] = '\0';
	    if (reserved_field(name)) {	/* make sure it is not reserved */
		char *name1;
		name1 = malloc(strlen(name)+3);
		sprintf(name1,"X-%s", name); /* reserved - add an "X-" to it */
		free(name);
		name = name1;
	    }
	    bp++;			/* skip past the ':' */
	    if (*bp == ' ') bp++;
	    text = malloc((cp1 - bp)+1); /* get the text of the field */
	    strncpy(text,bp,(cp1-bp));
	    text[cp1-bp] = '\0';
	    if (text[strlen(text)-1] == '\n')
		text[strlen(text)-1] = '\0';
	    h = new_header(USER_HEADERS, name, HEAD_UNKNOWN, msg);
	    h->string = text;
	    free(name);
	}
	if (*cp1 == '\0')
	    break;
	cp = cp1;
    }
    free(contents);
}

reserved_field(str) {
    int i;
    char *cp;

    
    for(i = 0; reserved_fields[i] != nil; i++) {
	cp = reserved_fields[i];
	while(isblank(*cp)) cp++;
	if (ustrcmp(cp, str) == 0) return(true);
    }
    return(false);
}


/* 
 * move ADR_FILES from the from address into the fcc field on the tomsg
 */

files_to_fcc(from, tomsg) 
addresslist *from;
mail_msg *tomsg;
{
    addr_unit *a, *a1;
    addresslist *fcc;

    if (tomsg == NULL) return;
    if (tomsg->fcc && from == tomsg->fcc->address) return;

    if (!tomsg->fcc) {
	tomsg->fcc = new_header(FCC, "Fcc", HEAD_KNOWN,tomsg);
	tomsg->fcc->address = (addresslist *)malloc(sizeof(addresslist));
	tomsg->fcc->address->first = tomsg->fcc->address->last = NULL;
    }
    fcc = tomsg->fcc->address;
    for(a = from->first; a != nil; ) {
	if (a->type == ADR_FILE) {
	    if (a->prev)
		a->prev->next = a->next;
	    if (a->next)
		a->next->prev = a->prev;
	    if (from->first == a)
		from->first = a->next;
	    if (from->last == a)
		from->last = a->prev;
	    a1 = a->prev;

	    a->prev = fcc->last;
	    a->next = nil;
	    if (fcc->last)
		fcc->last = fcc->last->next = a;
	    else
		fcc->first = fcc->last = a;

	    a = a1;
	    if (a == nil)
		a = from->first;
	}
	else
	    a = a->next;
    }
}


prompt_for_text(prompt, text) 
char *prompt;
char *text; 
{
  int i, li, co, cols, ov=FALSE;
  char c;
  int plen, plines = 0;
  int tlen;

  cmxprintf ("%s", prompt);

  if (text == NULL || !display_outgoing_message) /* no text yet */
    return;

  tlen = strlen(text);
  plen = strlen(prompt);
  for (i = 0; i < plen; i++)
    if (prompt[i] == '\n')
      plines++;

  li = cmcsb._cmrmx;			/* get number of lines */
  co = cmcsb._cmcmx;			/* and number of columns */

  li -= plines;				/* number of lines to keep at top */

  for (i = strlen(text)-1, cols = 0; i >= 0; i--) { /* figure out what */
    c = text[i] & 0x7f;			/*   we can display */
    if (c == '\t')
	cols = ((cols + 8) / 8) * 8;
    else 
	cols++;				/* incr column count */
    if (iscntrl(c) && !isspace(c))	/* control char takes two chars */
      cols++;				/*   to display, count the ^ */
    if (cols > co) {		
      --li;				/* we overflowed the line */
      cols = 0;				/* reset column count */
      ov = TRUE;			/* remember */
    }
    if (c == '\n') {			/* another line */
      cols = 0;				/* reset the column count */
      li--;
      ov = FALSE;
    }
    if (li == 0)			/* can't display any more lines */
      break;
  }
  if (ov && li == 0) {			/* top line doesn't fit on display */
    int p;
    p = i;
    while ((c = text[p] & 0x7f) != '\n' &&  p > 0) /* find beginning of line */
      p--;
    p += ((i - p)/co + 1) * co;
    i = p-1;				/* skip over the \n */
  }
    
  for(i++; i < tlen; i++) {	/* display the screenful */
    c = text[i] & 0x7f;
    if (iscntrl(c) && !isspace(c)) {
      cmxputc('^');			/* display control chars as */
      cmxputc(c | 0100);		/*   caret and uppercased */
    }
    else cmechx(c);
  }
}
