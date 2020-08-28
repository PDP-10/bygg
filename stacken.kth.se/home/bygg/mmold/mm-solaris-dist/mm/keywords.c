/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/keywords.c,v 2.1 90/10/04 18:24:35 melissa Exp $";
#endif

#include "mm.h"
#include "parse.h"
#include "message.h"
#include "rd.h"

extern keylist user_keywords;
char *safe_strcpy();

keylist
free_keylist(kl)
keylist kl;
{
    keylist fk;

    if (kl) {
	for(fk = kl; *fk != nil; fk++)
	    free(*fk);
	free(kl);
	kl = nil;
    }
    return(nil);
}

keylist
add_keyword(str,kl) 
char *str;
keylist kl;
{
    int count;
    char *key_validate();
    char *cp;
    if (!lookup_keyword(str,kl)) {
	cp = key_validate(str);
	if (cp == NULL || *cp == NULL)
	    return(kl);
	for(count = 0; kl && kl[count]; count++);
	kl = (keylist) safe_realloc(kl,
			       (2+count)*sizeof(char *));
	kl[count++] = safe_strcpy(cp);
	kl[count] = nil;
    }
    return(kl);
}

keylist
rem_keyword(str, kl)
char *str;
keylist kl;
{
    keylist fk;
    
    if (strcmp(str,"*") == 0 && kl) {
	free(kl);
	return(nil);
    }
    for(fk = kl; fk && *fk != nil; fk++)
	if (ustrcmp(*fk, str) == 0)
	    break;
    if (fk && *fk) {
	free(*fk);
	do {
	    *fk = *(fk+1);
	    fk++;
	} while (*fk);

	if (kl[0] == nil) {
	    free(kl);
	    return(nil);
	}
    }
    return(kl);
}

lookup_keyword(str, kl)
char *str;
keylist kl;
{
    keylist fk;
    
    if (strcmp(str,"*") == 0 && kl)
	return(true);
    for(fk = kl; fk && *fk != nil; fk++)
	if (ustrcmp(*fk, str) == 0)
	    return(true);
    return(false);
}

keytab *
mk_keyword_keytab(k1, k2)
keylist k1,k2;
{
    keylist k,keys=nil;
    int l1,l2;
    keytab *keylist_to_keytab();

    for(k = k1; k && *k; k++)
	keys = add_keyword(*k,keys);
    for(k = k2; k && *k; k++)
	keys = add_keyword(*k,keys);
    if (keys != nil)
	sort_keys(keys);
    return(keylist_to_keytab(keys));
}

keycmp(a,b)
char **a,**b;
{
    return(strcmp(*a,*b));
}

sort_keys(kl)
keylist kl;
{
    int i;
    keylist k;

    if (kl == nil)
	return;
    for(i = 0, k = kl; *k; k++, i++);
    return(qsort(kl, i, sizeof(char *), keycmp));
}


keytab *
keylist_to_keytab(kl)
keylist kl;
{
    keylist k;
    static keywrd *keys = nil;
    static keytab tab;
    int i,len;

    for(len = 0, k = kl; k && *k; k++, len++);
    if (keys != nil) {
	free(keys);
    }
    if (len > 0)
	keys = (keywrd *)malloc(len * sizeof(keywrd));
    else keys = nil;
    for(i = 0; i < len; i++) {
	keys[i]._kwkwd = kl[i];
	keys[i]._kwflg = 0;
	keys[i]._kwval = i;
    }
    tab._ktcnt = len;
    tab._ktwds = keys;
    return(&tab);
}


static brktab keybrk = {
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3f,
	0x80, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f,
    },
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x3f,
	0x80, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f,
    },
};

static brktab fldbrk = {
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3f,
	0x80, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f,
    },
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x3f,
	0x80, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f,
    },
};

char *
parse_keywords(kt)
keytab *kt;
{
    static fdb keys = { _CMKEY, 0, nil, nil, nil, nil, &keybrk };
    static fdb astfdb = { _CMTOK, CM_SDH, nil, (pdat) "*" };
    keys._cmdat = (pdat) kt;
    parse(fdbchn(&astfdb,&keys,nil), &pv, &used);
    if (used == &keys)
	return(kt->_ktwds[pv._pvkey]._kwkwd);
    else
	return("*");
}

char *
parse_new_keywords()
{
    keytab *kt = mk_keyword_keytab(cf?cf->keywords:nil,user_keywords);
    static fdb keys = { _CMKEY, 0, nil, nil, nil, nil, &keybrk };
    static fdb fldfdb = { _CMFLD, CM_SDH, nil, nil, "New Keyword", nil,
			      &fldbrk };

    keys._cmdat = (pdat) kt;
    parse(fdbchn(&keys,&fldfdb,nil), &pv, &used);
    if (used == &keys)
	return(kt->_ktwds[pv._pvkey]._kwkwd);
    else
	return(atmbuf);
}

char *
parse_old_keywords()
{
    keytab *kt = mk_keyword_keytab(cf?cf->keywords:nil,nil); 




    static fdb keys = { _CMKEY, 0, nil, nil, nil, nil, &keybrk };
    static fdb astfdb = { _CMTOK, 0, nil, (pdat) "*" };

    keys._cmdat = (pdat) kt;
    parse(fdbchn(&keys,&astfdb,nil), &pv, &used);
    if (used == &keys)
	return(kt->_ktwds[pv._pvkey]._kwkwd);
    else
	return("*");
}


cmd_keyword(n) 
int n;
{
    if (mode & MM_SEND) {
	char *key;

	noise("on outgoing message");
	key = (safe_strcpy(parse_new_keywords()));
	confirm();
	outgoing_keyword(key);
    }
    else {
	do_keyword();
    }
}

do_keyword() {
    message *m;
    char *key;

    if (!check_cf(O_RDWR))		/* pre-check file existence */
	return;
    key = (safe_strcpy(parse_new_keywords()));
    if (!parse_sequence ("current",NULL,NULL)) {
	if (!check_cf(O_WRONLY))	/* need write permission */
	    return;
	m = &cf->msgs[cf->current];
	m->keywords = add_keyword(key,m->keywords);
	set_msg_keywords(m);
	cf->keywords = add_keyword(key,cf->keywords);
	m->flags |= M_MODIFIED;
	(*msg_ops[cf->type].wr_msg)(cf,m,cf->current,0);
    }
    else {
	int n;
	if (!check_cf(O_WRONLY))	/* need write permission */
	    return;
	for (n = sequence_start (cf->sequence); n;
	     n = sequence_next (cf->sequence)) {
	    m = &cf->msgs[cf->current];
	    m->keywords = add_keyword(key,m->keywords);
	    set_msg_keywords(m);
	    cf->keywords = add_keyword(key,cf->keywords);
	    m->flags |= M_MODIFIED;
	    (*msg_ops[cf->type].wr_msg)(cf,m,cf->current,0);
	}
	seq_print (true);
    }
    free(key);
}

cmd_unkeyword(n) 
int n;
{
    char *key;

    if (mode & MM_SEND) {
	char *parse_current_keywords();
	noise("from outgoing message");
	key = (safe_strcpy(parse_current_keywords()));
	confirm();
	unoutgoing_keyword(key);
    }
    else {
	message *m;
	if (!check_cf(O_RDWR))		/* going to try and modify things */
	    return;
	key = (safe_strcpy(parse_old_keywords()));
	if (!parse_sequence ("current",NULL,NULL)) {
	    if (!check_cf(O_WRONLY))	/* need write permission */
		return;
	    m = &cf->msgs[cf->current];
	    m->keywords = rem_keyword(key,m->keywords);
	    set_msg_keywords(m);
	    m->flags |= M_MODIFIED;
	    (*msg_ops[cf->type].wr_msg)(cf,m,cf->current,0);
	}
	else {
	    int n;
	    if (!check_cf(O_WRONLY))	/* need write permission */
		return;
	    for (n = sequence_start (cf->sequence); n;
		 n = sequence_next (cf->sequence)) {
		m = &cf->msgs[cf->current];
		m->keywords = rem_keyword(key,m->keywords);
		set_msg_keywords(m);
		m->flags |= M_MODIFIED;
		(*msg_ops[cf->type].wr_msg)(cf,m,cf->current,0);
	    }
	    seq_print (true);
	}
    }
    free(key);
}

set_msg_keywords(m)
message *m;
{
    char *hfind();
    char *begin = m->text;
    char *oldkey = hfind("keywords",begin);
    char *newkey=nil;
    char *end=nil;
    keylist k;
    int len=0;
    char *newmsg, *skipheader();

    if (oldkey) 
	end = skipheader(oldkey);	/* point to next header */
    else {				/* make it the last header */
	if (m->keywords == nil)
	    return;
	oldkey = search("\n\n",m->text);
	if (oldkey == nil)
	    oldkey = end = begin;
	else
	    end = ++oldkey;		/* move past one '\n' */
    }

    if (m->keywords) {			/* create the new keywords: field */

	for(k = m->keywords; *k; k++)
	    len += strlen(*k) + ((*(k+1)) ? 2 : 0); /* new keyword + ", " */

	newkey = malloc(len + 2 + strlen("Keywords: "));

	strcpy(newkey,"Keywords: ");
	for(k = m->keywords; *k; k++) {
	    strcat(newkey,*k);
	    if (*(k+1))
		strcat(newkey,", ");
	}
	strcat(newkey,"\n");
    }
    len = (oldkey - begin);
    if (newkey) len += strlen(newkey);
    if (end) len += strlen(end);
    newmsg = malloc(len+1);
    strncpy(newmsg,begin,oldkey-begin);
    newmsg[oldkey-begin] = '\0';
    if (newkey) {
	strcat(newmsg,newkey);
	free(newkey);
    }
    if (end)
	strcat(newmsg,end);
    free(m->text);
    m->text = newmsg;
    m->size = len;
}

char *
parse_current_keywords()
{
    keytab *kt;
    static fdb keys = { _CMKEY };
    static fdb astfdb = { _CMTOK, 0, nil, (pdat) "*" };
    extern mail_msg *current;
    int flag;

    flag = (current->keywords && current->keywords->keys);
    kt = mk_keyword_keytab(flag ? current->keywords->keys : nil ,nil);
    keys._cmdat = (pdat) kt;
    parse(fdbchn(&keys,&astfdb,nil), &pv, &used);
    if (used == &keys)
	return(kt->_ktwds[pv._pvkey]._kwkwd);
    else
	return("*");
}

get_incoming_keywords(cf,m)
msgvec *cf;
message *m;
{
    char *htext();
    char *k = htext("keywords", m->text);
    char *c1, *c2, *stripspaces();

    c2 = k;
    if (k == nil)
	return;
    k = stripspaces(k);
    while((c1 = index(k,',')) != nil) {
	*c1 = '\0';
	while(*k == ' ') k++;
	m->keywords = add_keyword(k,m->keywords);
	if (cf)				/* sendmail only wants message done */
	    cf->keywords = add_keyword(k,cf->keywords);
	*c1 = ',';
	k = c1 + 1;
    }
    while(*k == ' ') k++;
    if (*k) {
	m->keywords = add_keyword(k,m->keywords);
	if (cf)
	    cf->keywords = add_keyword(k,cf->keywords);
    }    
    free(c2);
}

keylist 
match_keylist(s)
char *s;
{
    char *c1, *c2, *stripspaces();
    keylist kl=nil;

    s = stripspaces(s);
    while((c1 = index(s,',')) != nil) {
	*c1 = '\0';
	while(*s == ' ') s++;
	kl = add_keyword(s,kl);
	*c1 = ',';
	s = c1 + 1;
    }
    while(*s == ' ') s++;
    if (*s) {
	kl = add_keyword(s, kl);
    }    
    return(kl);
}

char *
key_validate(str) 
char *str;
{
    static char buf[100];
    int i,j;
    
    strcpy(buf,str);
    for(i = strlen(buf) - 1; i>= 0; i--)
	if (isspace(buf[i])) 
	    buf[i] = '\0';
	else
	    break;
    
    for(i = 0; i < strlen(buf); i++)
	if (!isspace(buf[i])) 
	    break;
    if (i > 0)
	bcopy(&buf[i], buf, strlen(&buf[i])+1);
    for(i = 0; i < strlen(buf); i++) {
	if (isspace(buf[i])){
	    buf[i] = '_';
	}
	else if (!isprint(buf[i])) {
	    buf[i] = 'X';
	}
    }
    return(buf);
}
    


/*
 * keylist_copy:
 * take a keylist and malloc up a copy
 */

keylist
keylist_copy (kl)
keylist kl;
{
    int i = 0;
    keylist nk, k;

    for (k = kl; *k != nil; k++)
	i++;
    nk = (keylist) malloc ((i+1)*sizeof (char *));
    for (i = 0; kl[i] != nil; i++)
	nk[i] = safe_strcpy (kl[i]);
    nk[i] = nil;
    return (nk);
}
    
