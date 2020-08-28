/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/token.c,v 2.1 90/10/04 18:26:55 melissa Exp $";
#endif

/*
 * token.c - rudimentary RFC822 address parsing
 */

#define MAIL11				/* for VMS type addresses */
#define ALLOWDOT			/* allow dots in some extra places */
/* #define NOTMM */			/* use to get rid of MM dependencies */

#include <stdio.h>
#include "token.h"
#define _CHARTYPE_ARRAY_		/* necessary for chartype.h */
#ifdef NOTMM
#include "chartype.h"
#else
#include "mm.h"
#include "address.h"
#endif /* NOTMM */
/*
 * token struct used for parsing rfc822 address lists
 */

typedef struct token {
    unsigned char type;			/* token type */
    unsigned char ptype;		/* parse type */
    unsigned short len;			/* token string length */
    unsigned short plen;		/* parse type length in tokens */
    short clen;				/* length of following comment */
    char *text;				/* pointer to static text */
    char *ctext;			/* text of following comment */
    struct token *next;			/* pointer to next token */
} token;

#ifdef TEST
#define add_addresslist(a,b,c)
#endif
/*
 * test for whether we're looking at whitespace -- should be a macro
 */

folding (s)
char *s;
{
    while (isblank (*s))		/* skip tabs, spaces */
	++s;
    if (*s=='\r')
	++s;
    if (*s == '\n' && isblank (*++s))	/* newline followed by whitespace? */
	return 1;			/* yes, line continuation */
    return 0;
}

/*
 * given a pointer to text after an opening '(', returns a pointer to the
 * character following the matching ')' (or a null if the comment wasn't
 * terminated properly).
 */

char *
eatcomment(s)
char *s;
{
    int parencount = 1;

    if (*s == '(') ++s;

    while (*s) {
	switch (*s) {
	  case '\\':
	    if (*++s) break;		/* skip unless null */
	    return s;
	  case '(':
	    ++parencount;		/* parens nest */
	    break;
	  case ')':
	    if (--parencount == 0)
		return ++s;
	    break;
	  case '\r':
	  case '\n':			/* check for continuation */
	    if (!folding (s))
		return s;		/* bad continuation line */
	    break;
	  default:
	    if (iscntrl(*s))
		return s;		/* return pointer to null */
	}
	++s;
    }
    return s;				/* return pointer to null */
}

/*
 * given a pointer to an opening '"', return a pointer to the char following
 * the closing '"'.
 */

char *
eatqst(s)
char *s;
{
    while (*++s) {
	switch (*s) {
	  case '\\':
	    ++s;
	    break;
	  case '"':
	    return ++s;
	  case '\n':
	    if (!isblank(s[1]))
		return s;		/* not a continuation line */
	    break;
	  case '\0':
	    return s;
	}
    }
}

/*
 * allocate a token struct and fill in the type and value
 */

token *
alloc_token (type, text, len)
int type, len;
char *text;
{
    register token *t;
    char * calloc ();

    if (t = ((token *) calloc (1, sizeof (token)))) {
	t->type = type;
	t->text = text;
	t->len = len;
    }
    return t;
}

/*
 * break a null-terminated string into a list of tokens and return it.
 * basically what you'd expect, except that any paren-delimited comment
 * string is attached as a unit to the previous token, if any.  Such
 * comments are lost if they appear before any  "significant" tokens, but
 * I don't think that's much to worry about.
 */

token *
lex (s)
char *s;
{
    register char *p = NULL;
    token head, *tail, *t = NULL;
    
    tail = head.next = &head;

    while (*s && tail) {
	while (isblank (*s)) {
	    do ++s; while (isblank (*s));
	    if (*s == '\n') {
		if (folding (++s))
		    continue;
		else {
		    /*
		     * data pointer in T_EOH token points to unparsed text
		     */
		    tail->next = alloc_token (T_EOH, s, 0);
		    tail = tail->next;
		    tail->next = NULL;
		    return head.next;
		}
	    }
	}

	if (!*s && (p == 0))
	    return NULL;

	p = s;				/* save start of string */
	if (isatom (*s)
#ifdef ALLOWDOT
		   || *s == '.'
#endif
	    ) {
	    s++;
	    while (isatom (*s)
#ifdef ALLOWDOT
		   || *s == '.'
#endif
		   )
		s++;
	    t = alloc_token (T_ATOM, p, (int) (s-p));
	}    
	else if (isspecial(*s)) {
	    switch (*s) {
	      case '(':
		s = eatcomment(s);
		tail->ctext = p;
		tail->clen = (s - p);
		continue;
	      case ')':
		t = alloc_token (T_RPAREN, s, 1);
		break;
	      case '<':
		t = alloc_token (T_LROUTE, s, 1);
		break;
	      case '>':
		t = alloc_token (T_RROUTE, s, 1);
		break;
	      case '@':
		t = alloc_token (T_AT, s, 1);
		break;
	      case ',':
		if (tail->type == T_COMMA) {
		    ++s;		/* multiple commas are allowed, but */
		    continue;		/*  aren't meaningful */
		}
		t = alloc_token (T_COMMA, s, 1);
		break;
	      case ';':
		t = alloc_token (T_SEMI, s, 1);
		break;
	      case ':':
#ifdef MAIL11
		if (*(s+1) == ':') {
		    t = alloc_token(T_COLCOL, s, 2);
		    s++;
		}
		else
#endif
		    t = alloc_token (T_COLON, s, 1);
		break;
	      case '\\':
		if (*++s)
		    t = alloc_token (T_QPAIR, p, 2);
		else
		    return NULL;
		break;
	      case '"':
		s = eatqst (s);
		t = alloc_token (T_QSTR, p, (int) (s-p));
		--s;
		break;
	      case '.':
		t = alloc_token (T_DOT, s, 1);
		break;
	      case '[':
		t = alloc_token (T_LDOMLIT, s, 1);
		break;
	      case ']':
		t = alloc_token (T_RDOMLIT, s, 1);
	    }
	    ++s;
	}
	else if (*s) ++s;		/* ignore the character */

	tail = (tail->next = t);
    }
    if (tail)
	tail->next = NULL;
    return head.next;
}

/*
 * advance a pointer along a token chain - should probably be a macro
 */

token *
advance (t, n)
token *t;
int n;
{
#if TEST > 1
    char *untoken ();
    char *p = untoken (t, n, ' ', 0);
    if (p) {
	printf ("advancing past %d tokens '%s'\n", n, p);
	free (p);
    }
#endif
    while (t && (n-- > 0))
	t = t->next;
    return t;
}

/*
 * turn a token list back into an ascii string.  the string returned should
 * be released with free().
 */

char *
untoken (t, n, stripcomments, dofree)
token *t;
int n, stripcomments, dofree;
{
    char *p, *cp, *malloc ();
    int i, len = 0;
    token *head = t;

    for (i = 0; t && (i < n); i++, t = t->next) {
	len += t->len;
	if (t->clen && !stripcomments)
	    len += t->clen + 1;
    }

    len += n;				/* count delimiters, trailing null */

    p = cp = malloc (len+1);		/* get the space */
    if (p) {
	for (t = head, i = n; t && (i > 0); i--) {
	    strncpy (cp, t->text, t->len);
	    cp += t->len;
	    if (t->clen && !stripcomments) {
		*cp = ' ';
		strncpy (++cp, t->ctext, t->clen);
		cp += t->clen;
		if (t->next && isspace(t->ctext[t->clen]))
		    *cp++ = ' ';
	    }
	    else
		if (!stripcomments && t->next && isblank(t->text[t->len]))
		    *cp++ = ' ';
	    if (dofree) {
		token *old = t;
		t = t->next;
		free (old);
	    }
	    else
		t = t->next;
	}		
	*cp = 0;
    }
    return p;
}

/*
 * are we looking at an RFC822 "phrase"?
 */

int
phrase (t)
token *t;
{
    int len = 0;
    while (t && (t->type == T_QSTR || t->type == T_ATOM)) {
	t = advance (t, 1);
	++len;
    }
    return len;
}

/*
 * try to parse a domain name (one or more dot-delimited atoms)
 */

int
domain (t)
token *t;
{
    int n = 0, needdot = 0;

    if (t && (t->type == T_LDOMLIT))
	return domlit (t);

    while (t) {
	if ((needdot && t->type == T_DOT) ||
	    (!needdot && t->type == T_ATOM))
	    needdot = ~needdot;
	else
	    break;

	t = advance (t, 1);
	++n;
    }		
    if (n && !needdot)
	--n;
    return n;
}

/*
 * parse a domain literal, e.g. "[128.59.16.20]"
 * the argument must be a pointer to a "[" token.
 */ 

int
domlit (t)
token *t;
{
    int n;

    if (t->type != T_LDOMLIT || (t = t->next) == NULL)
	return 0;

    n = domain (t);
    t = advance (t, n);
    if (t && t->type == T_RDOMLIT)
	return n + 2;
    return 0;
}

/*
 * parse a "local-part" of an rfc822 mailbox, consisting of a dot-delimited
 * list of quoted-strings and/or atoms.
 */

int
localpart (t)
token *t;
{
    int n = 0, needdot = 0;

    while (t) {
	if ((needdot && t->type == T_DOT) ||
	    (!needdot && (t->type == T_QSTR || t->type == T_ATOM))) {
	    needdot = ~needdot;
	    n++;
	}
	else
	    break;
	t = t->next;
    }		
    if (n && !needdot)
	--n;				/* don't swallow trailing dot */
    return n;
}

/*
 * parse an RFC822 addr-spec -- "localpart@domain"
 */

int
addrspec (t)
token *t;
{
    token *head = t;
    int n, len = 0;

    if (!t)
	return 0;

    len += (n = localpart (t));
    if (n == 0)
	return 0;
    t = advance (t, n);
    if (t && t->type == T_AT) {
	len += 1;
	if (t = advance (t, 1)) {
	    if (n = domain (t))
		len += n;
	    else if (n = domlit (t))
		len += n;
	}
	else
	    return 0;
    }
    head->ptype = T_ADDRSPEC;
    head->plen = len;
    return len;
}

/*
 * parse a route, e.g. "@domain,...@domain:"
 */

int
route (t)
token *t;
{
    int n = 0, len = 0;

    while (t && t->type == T_AT && (t = advance (t, 1))) {
	if ((n = domain (t)) || (n = domlit (t))) {
	    t = advance (t, n);
	    len += n + 2;		/* commit to next token */
	    if (t->type == T_COLON)
		return len;
	    else if (t->type == T_COMMA) {
		t = advance (t, 1);
		continue;
	    }
	    break;
	}
    }
    return 0;
}

/*
 * parse a routeaddr -- "<@domain,...,domain:localpart@domain>"
 */

int
routeaddr (t)
token *t;
{
    int n, len = 1;

    if (t) {
	if (t->type != T_LROUTE)
	    return 0;
	if ((t = t->next) == NULL)
	    return 0;
	if (t->type == T_AT) {
	    len += (n = route (t));
	    if (n == 0 || ((t = advance (t, n)) == NULL))
		return 0;
	}
	len += (n = addrspec (t));
	
	if (n == 0 || ((t = advance (t, n)) == NULL))
	    return 0;
	if (t->type == T_RROUTE)
	    return (len + 1);
    }
    return 0;
}

int
group (t)
token *t;
{
    int n, len = 0;
    token *tp = t;

    if ((n = phrase (t)) > 0) {
	if (t = advance (t, n)) {
	    if (t->type != T_COLON)
		return 0;
	    len += n;
	    t = advance (t, 1);
	    len += 1;
	    for (;;) {
		if (n = mailbox (t)) {
		    t = advance (t, n);
		    len += n;
		    if (t && (t->type == T_COMMA)) {
			len += 1;
			t = advance (t, 1);
			continue;
		    }
		}
		break;
	    }
	    if (t && (t->type == T_SEMI)) {
		tp->ptype = T_GROUPLIST;
		tp->plen = ++len;
		t->ptype = T_GROUPEND;
		t->plen = 1;
		return len;
	    }
	}
    }
    return 0;
}

#ifdef MAIL11
/* 
 * parse mail11 addresses
 *  or at least the hostname:: part.
 */
int
mail11_mailbox (t)
token *t;
{
    int n;
    token *head = t;
    if (t->type == T_ATOM) { 
	t = advance(t,1);
	if (t && (t->type == T_COLCOL)) {
	    t = advance(t,1);
	    n = addrspec(t);
	    if (n == 0)
		return(0);
	    head->ptype = T_MAIL11;
	    head->plen = n + 2;
	    return(n + 2);		/* addr_spec + hostname + "::" */
	}
    }
    return(0);
}
#endif
/*
 * Parse "phrase route-addr" or "addrspec".  
 */

int
rfc822_mailbox (t)
token *t;
{
    token *head = t;
    int n, len;

    if (n = phrase(t)) {
	len = n;
	t = advance (t, n);		/* skip past it */
    }
    if (n = routeaddr (t)) {
	head->ptype = T_PHRASEADDR;
	head->plen = len + n;
	return head->plen;		/* if followed by route-addr, done */
    }
    return addrspec (head);		/* see if it's an addrspec */
}

int
mailbox (t)
token *t;
{
    int n;
#ifdef MAIL11
    if (n = mail11_mailbox(t)) return(n);
#endif
    if (n = rfc822_mailbox(t)) return(n);
    return(0);
}

static int
addrlist (t)
token *t;
{
    token *tp;
    int n, naddrs = 0;

    while (t) {
	tp = t;
	if (n = group (t)) {
	    ++naddrs;
	    if (t = advance (t, n)) {
		if (t->type == T_COMMA) {
		    t->ptype = T_COMMA;
		    t->plen = 1;
		    t = advance (t, 1);
		    continue;
		}
	    }
	    else
		break;
	}
	else if (n = mailbox (t)) {
	    ++naddrs;
	    if (t = advance (t, n)) {
		if (t->type == T_COMMA) {
		    t->ptype = T_COMMA;
		    t->plen = 1;
		    t = advance (t, 1);
		    continue;
		}
	    }
	    else
		break;
	}
	if (t == NULL || t->type == T_EOH)
	    return naddrs;

	/* parse problem - mark invalid tokens and try to continue */

	tp->ptype = T_IGNORE;		/* mark token string bad */
	tp->plen = 1;
	t = tp->next;
	
	/* munch tokens till we find comma or end of string */
	while (t && (t->type != T_COMMA) && (t->type != T_EOH)) {
	    ++tp->plen;
	    t = advance (t, 1);
	}

	/* eat following comma */
	if (t && (t->type == T_COMMA)) {
	    t = advance (t, 1);
	}
    }
    return naddrs;
}

char *
unspace(str)
char *str;
{
    char *cp;
    while(isspace(*str)) str++;
    cp = str + strlen(str) - 1;
    while(isspace(*cp)) *cp-- = '\0';
    return(str);
}

#ifndef NOTMM
match_addresses (a, buf, len)
char **buf;
addresslist *a;
int len;
{
    int n;
    token *t, *newt;
    token *t2;
    char *p,*cp;

    if (strlen (*buf) < 1)
	return;
    t = lex (*buf);
    if ((t == 0) || (t->type == T_EOH))
	return;

    for (n = 0, t2 = t; t2; t2 = t2->next)
	++n;
    /*
     * Note that the untoken calls free the address token structs,
     * hence the use of newt to step through the list.
     */
    if (addrlist (newt = t)) {
	int n;
	token *t1;
	while (t = newt) {
	    switch (t->ptype) {
	    case T_GROUPLIST:
		for (n = 1, t1 = advance(t,1); t1->ptype == T_NONE;
		     t1 = advance(t1,1),n++) 
		    ;
		newt = advance (t, n);
		add_addresslist(a, unspace(untoken(t,n - 1,0,1)), ADR_GROUP);
		break;
	      case T_GROUPEND:
		newt = advance (t, 1);
		add_addresslist(a, unspace(untoken(t,1,0,1)), ADR_GROUPEND);
		break;
#ifdef MAIL11
	      case T_MAIL11:
#endif
	      case T_ADDRSPEC:
	      case T_PHRASEADDR:
		newt = advance (t, t->plen);
		cp = unspace(untoken(t,t->plen,0,1));
#ifndef TEST
		if (strcmp(cp,".") == 0)
		    add_addresslist(a,user_name,ADR_ADDRESS);
		else if (*cp == '*')
		    add_addresslist(a,tilde_expand(cp+1),ADR_FILE);
		else if (lookup_alias(cp))
		    add_addresslist(a,cp,ADR_ALIAS);
		else
#endif
		    add_addresslist(a,cp,ADR_ADDRESS);
		break;
	      case T_IGNORE:
		newt = advance (t, (t->plen ? t->plen : 1));
		cp = unspace(untoken (t, t->plen, 0, 1));
		if (use_address(cp))
		    add_addresslist(a, cp, ADR_ADDRESS);
		break;
	      default:
		newt = t->next;
#ifdef TEST
		if (t->type != T_COMMA)
		    printf ("unknown token \"%s\"\n",
			    untoken (t, 1, 0, 1));
#endif
		break;
	    }
	}
    }
}


use_address(str) 
char *str;
{
#ifndef TEST
    extern use_invalid_address;
    switch(use_invalid_address) {
      case SET_YES:
	return(true);
      case SET_NO:
	printf("Invalid address: \"%s\"\n", str);
	return(false);
      case SET_ASK:
	printf("Invalid address: \"%s\"\n", str);
	return(yesno("Use anyway? "));
    }
#else
    printf("Invalid address: \"%s\"\n", str);
    return(0);
#endif
}

#if TEST
main(argc,argv) 
int argc;
char **argv;
{
    addresslist a;
    char *buf;
    
    buf = (char *)malloc(512);
    a.first = a.last = NULL;
    while (fgets (buf, 512, stdin) != NULL) {
	match_addresses(&a,&buf,strlen(buf));
    }
}
#endif /* TEST */
#else /* NOTMM */
#if TEST
main (argc, argv)
int argc;
char *argv[];
{
    int n;
    token *t, *newt;
    char buffer[512];
    char *p;

    while ((p = fgets (buffer, sizeof (buffer), stdin)) != NULL) {
	if (strlen (buffer) < 1)
	    continue;
	t = lex (buffer);
	if ((t == 0) || (t->type == T_EOH))
	    continue;
	{
	    token *t2;
	    for (n = 0, t2 = t; t2; t2 = t2->next)
		++n;
	    printf ("n = %d, tokens = %s\n", n, untoken (t, n, 0, 0));
	}
	/*
	 * Note that the untoken calls free the address token structs,
	 * hence the use of newt to step through the list.
	 */
	if (addrlist (newt = t)) {
	    while (t = newt) {
		switch (t->ptype) {
		  case T_GROUPLIST:
		    newt = advance (t, t->plen);
		    printf ("group = %s\n", untoken (t, t->plen, 0, 1));
		    break;
#ifdef MAIL11
		  case T_MAIL11:
#endif
		  case T_ADDRSPEC:
		  case T_PHRASEADDR:
		    newt = advance (t, t->plen);
		    printf ("address = %s\n", untoken (t, t->plen, 0, 1));
		    break;
		  case T_IGNORE:
		    newt = advance (t, (t->plen ? t->plen : 1));
		    printf ("bad tokens: %s\n", untoken (t, t->plen, 0, 1));
		    break;
		  default:
		    newt = t->next;
		    if (t->type != T_COMMA)
			printf ("unknown token \"%s\"\n",
				untoken (t, 1, 0, 1));
		    break;
		}
	    }
	}
	else
	    printf ("no addresses found\n");
    }
    exit (0);
}
#endif /* TEST */

#endif NOTMM
