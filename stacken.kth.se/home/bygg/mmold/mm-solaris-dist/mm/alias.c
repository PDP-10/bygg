/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/alias.c,v 2.1 90/10/04 18:23:26 melissa Exp $";
#endif

#include "mm.h"
#include "parse.h"

Mail_aliases mail_aliases = { nil, 0 };

static brktab fldbrk = {
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0x00, 0x3f,
	0x80, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x1f,
    },
    {
	0xff, 0xff, 0xff, 0xff, 0xfb, 0xf9, 0x00, 0x1f,
	0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x1f,
    },
};

cmd_define(n)
int n;
{
    char *safe_strcpy();
    static char *alias = nil;
    static addresslist alias_addr = { nil, nil };
    keytab *mk_alias_keys(), *ak;
    static fdb fldfdb = { _CMFLD, CM_SDH, NULL, NULL, "new alias name",
			  NULL, &fldbrk};
    extern fdb aliasfdb;		/* get from parse.c */
    pval pv;
    fdb *used;
    
    ak = mk_alias_keys();
    aliasfdb._cmdat = (pdat) ak;

    noise("alias");
    if (mail_aliases.count > 0)
	parse(fdbchn(&aliasfdb, &fldfdb, nil), &pv, &used);
    else
	parse(fdbchn(&fldfdb, nil), &pv, &used);
    if (alias)
	free(alias);
    if (used == &aliasfdb) {
	alias = safe_strcpy(ak->_ktwds[pv._pvkey]._kwkwd);
    }
    else {
	if (strlen(atmbuf) < 1)
	    cmerr("Invalid character in alias name");
	alias = safe_strcpy(atmbuf);
    }
    noise("as");
    parse_define(&alias_addr);	/* get an addresslist for it */
    if (alias_addr.first)
	set_alias(safe_strcpy(alias),&alias_addr,MA_USER); /* set new alias */
    else
	free_alias(alias);
    alias_addr.first = alias_addr.last = nil;
    free(alias);
    alias = nil;
}

free_alias(name) 
char *name;
{
    mail_alias *ma;
    int i,j;


    if (mail_aliases.count == 0) 
	return;

    for(i = 0; i < mail_aliases.count; i++) {
	ma = &mail_aliases.aliases[i];
	if (ustrcmp(ma->name, name) == 0) {
	    free(ma->name);
	    free_addresslist(&ma->alias);
	    for(j = i+1; j < mail_aliases.count; i++, j++) {
		mail_aliases.aliases[i].name = 
		    mail_aliases.aliases[j].name;
		mail_aliases.aliases[i].alias = 
		    mail_aliases.aliases[j].alias;
		mail_aliases.aliases[i].type =
		    mail_aliases.aliases[j].type;
	    }
	    mail_aliases.count--;
	    return;
	}
    }
}


set_alias(name,addr,type)
char *name;
addresslist *addr;
{
    int i;
    free_alias(name);			/* free any old definition */
    i = mail_aliases.count;		/* remember old count */
    mail_aliases.aliases = (mail_alias *)safe_realloc(mail_aliases.aliases,
				   (++mail_aliases.count) *
				   sizeof(struct mail_alias));
    mail_aliases.aliases[i].name = name;
    mail_aliases.aliases[i].alias.first = addr->first;
    mail_aliases.aliases[i].alias.last = addr->last;
    mail_aliases.aliases[i].type = type;
}


disp_alias(fp,n,verbose,newline) 
FILE *fp;
int n;
int newline;
{
    mail_alias *ma = &mail_aliases.aliases[n];
    if (verbose)
	fprintf(fp,"define %s ", ma->name);
    disp_addresses(fp, nil, &ma->alias, true, newline, false);
    if (!newline)			/* add trailing newline */
        fputc ('\n', fp);
}


addresslist *
lookup_alias(name) 
char *name;
{
    mail_alias *ma;
    int i,j;

    if (mail_aliases.count == 0) 
	return(nil);

    for(i = 0; i < mail_aliases.count; i++) {
	ma = &mail_aliases.aliases[i];
	if (ustrcmp(ma->name, name) == 0) {
	    return(&ma->alias);
	}
    }
    return(nil);
}

