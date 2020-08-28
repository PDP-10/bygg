/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/parsemsg.c,v 2.4 90/10/04 18:25:27 melissa Exp $";
#endif

#include "mm.h"
#include "parse.h"
#include "cmds.h"
#include "message.h"

extern msgvec *cf;

keywrd header_keys[] = {
    { "to", 0, TO },
    { "cc", 0, CC },
    { "bcc", 0, BCC },
    { "fcc", 0, FCC },
    { "from", 0, FROM },
    { "date", 0, DATE },
    { "subject", 0, SUBJECT },
    { "reply-to", 0, REPLY_TO },
    { "in-reply-to", 0, IN_REPLY_TO },
    { "resent-to", 0, RESENT_TO },
    { "resent-date", 0, RESENT_DATE },
    { "resent-from", 0, RESENT_FROM },
    { "sender", 0, SENDER },
    { "references", 0, REFERENCES },
    { "comments", 0, COMMENTS },
    { "message-id", 0, MESSAGE_ID },
    { "keywords", 0, KEYWORDS },
    { "encrypted", 0, ENCRYPTED },
    { "received", 0, RECEIVED },
};

static keytab header_tab = { (sizeof(header_keys)/ sizeof(keywrd)),
				 header_keys };

static brktab hdrbrk = {		/* break table for field name */
    {					/* letters only in first position */
      0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x20, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
    },
    {					/* letters, digits and hyphens here */
      0xff, 0xff, 0xff, 0xff, 0xb2, 0xe8, 0x00, 0x3f,
      0x80, 0x00, 0x00, 0x16, 0x80, 0x00, 0x00, 0x1f
    }					/* (also +%_.!) */
};

static fdb header_fdb = { _CMKEY, 0, nil, (pdat) &header_tab,"header field, ",
			  NULL, &hdrbrk};
static fdb other = { _CMFLD, FLD_EMPTY, NULL, NULL, NULL, NULL, &hdrbrk };
static fdb colon = { _CMTOK, 0, nil, (pdat) ":" };
static fdb text = { _CMTXT };

/*
 * take a 'text' mail message, and convert it to internal format.
 * parse out all of the headers, etc.
 */
static mail_msg mesg;

mail_msg *
parse_msg(msg)
message *msg;
{
    char *s = msg->text, *next_header();
    char *s1, *realloc(), *malloc();
    int next;
    int retval;
    pval pv;
    fdb *used;
    int parselen;
    headers *h, *new_header();
    int len;
    keylist match_keylist(), kl, k, add_keyword();
    
    bzero (&mesg, sizeof (mail_msg));
    while(s1 = next_header(&s,&next)) {
	int l;
	stripspaces(s1);
	l = strlen(s1);
	s1 = realloc(s1, strlen(s1) + 2);
	s1[l] = '\0';
	strcat(s1, "\n");
	retval = match(s1, strlen(s1), fdbchn(&header_fdb, &other,
					      NULL), &pv, &used, &parselen);
	if (retval == CMxOK) {
	    char *name = malloc(parselen+1);
	    strncpy(name,s1,parselen);
	    name[parselen] = '\0';
	    s1 += parselen;
	    if (used == &header_fdb) {
		int which = pv._pvkey;
		if (match(s1, strlen(s1), fdbchn(&colon, nil),
			  &pv, &used, &parselen) == CMxOK) {
		    h = new_header(which, name, HEAD_KNOWN, &mesg);
		    free(name);
		    s1 += parselen;
		}
		else {
		    free(name);
		    break;
		}
		switch (which) {
		  case TO:
		  case CC:
		  case BCC:
		  case RESENT_TO:
		    h->address = (addresslist *) malloc(sizeof(addresslist));
		    h->address->first = h->address->last = NULL;
		    match_addresses(h->address, &s1, strlen(s1));
		    break;
		  case KEYWORDS:
		    h->keys = match_keylist(s1);
		    if (!(mode& MM_SEND)) {
			k = h->keys;
			while(k && *k) {
			    cf->keywords = add_keyword(*k, cf->keywords);
			    k++;
			}
		    }
		    break;
		  case FCC:
		    k = kl = match_keylist(s1);
		    h->address = (addresslist *) malloc(sizeof(addresslist));
		    h->address->first = h->address->last = NULL;
		    while (k && *k) {
		      add_addresslist(h->address, *k, ADR_FILE);
		      k++;
		    }
		    free_keylist(kl);
		    break;
		  case FROM:
		  case DATE:
		  case SUBJECT:
		  case REPLY_TO:
		  case IN_REPLY_TO:
		  case RESENT_DATE:
		  case RESENT_FROM:
		  case SENDER:
		  case REFERENCES:
		  case COMMENTS:
		  case MESSAGE_ID:
		  case ENCRYPTED:
		  case RECEIVED:
		    skip_spaces(&s1);
		    retval = match(s1, strlen(s1), fdbchn(&text,nil), &pv,
				   &used, &parselen);
		    h->string = malloc(parselen+1);
		    strncpy(h->string,s1,parselen);
		    s1 += parselen;
		    /* zero-terminate and trim trailing whitespace */
		    do { h->string[parselen] = 0; }
		    while (--parselen >= 0 && isspace (h->string[parselen]));
		    break;
		}
	    }
	    else {
		if (match(s1, strlen(s1), fdbchn(&colon, nil),
			  &pv, &used, &parselen) == CMxOK) {
		    h = new_header(USER_HEADERS, name, HEAD_UNKNOWN,
				   &mesg);
		    free(name);
		    s1 += parselen;
		    skip_spaces(&s1);
		    retval = match(s1, strlen(s1), fdbchn(&text,nil), &pv,
				   &used, &parselen);
		    h->string = malloc(parselen+1);
		    strncpy(h->string,s1,parselen);
		    s1 += parselen;
		    /* zero-terminate and trim trailing whitespace */
		    do { h->string[parselen] = 0; }
		    while (--parselen >= 0 && isspace (h->string[parselen]));
		}
		else {
		    /*
		     * this line is not a valid header, so back up to previous
		     * newline and treat that as the start of the message body.
		     */
		    char *cp = s - 2;	/* skip over \n */
		  
		    while (cp > msg->text && *cp != '\n')
			--cp;
		    if (*cp == '\n')
			s = cp;
		    free(name);
		    break;
		}
	    }
	}
	else {
	    /* see code above */
	    char *cp = s - 2;

	    while (cp > msg->text && *cp != '\n')
		--cp;
	    if (*cp == '\n')
		s = cp;
	    break;
	}
	if (!next) break;
    }
    if (*s == '\n')			/* if leading newline... */
	s++;				/* ...separator, not body */
    len = msg->size - (s - msg->text);
    mesg.body = malloc(len+1);
    strncpy(mesg.body,s,len);
    mesg.body[len] = '\0';
    return(&mesg);
}
