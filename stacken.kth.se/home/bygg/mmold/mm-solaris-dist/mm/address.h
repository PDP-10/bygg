/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef __ADDRESS
#define __ADDRESS

#ifdef RCSID
#ifndef lint
static char *addr_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/address.h,v 2.1 90/10/04 18:23:25 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * parse for RFC822 mail addresses.
 */

/* 
 * an entry in an address list can be a comment, a phrase: to start a 
 * list, a group end, or an actual address.  Keep these all chained in a list.
 * one of these is an address unit.
 */
typedef struct addr_unit {
    int type;
    char *data;
    struct addr_unit *next, *prev;
} addr_unit;

typedef struct addresslist {
    addr_unit *first;
    addr_unit *last;
} addresslist;

#define ADR_ADDRESS 1
#define ADR_GROUP 2
#define ADR_GROUPEND 3
#define ADR_FILE 4
#define ADR_ALIAS 5
#define ADR_AL_EXPAND 6
#define ADR_LISTFILE 7
#define ADR_MLIST 8


/* types of mail aliases */
#define MA_USER		0		/* user set alias, needs to be saved */
#define MA_SYSTEM	1		/* aliases from system init file */
#define MA_MAILRC	2		/* Mail alias from .mailrc */

typedef struct mail_alias {
    char *name;
    addresslist alias;
    int type;				/* type of alias */
} mail_alias;


typedef struct mail_aliases {
    mail_alias *aliases;
    int count;
} Mail_aliases;

#endif /* __ADDRESS */

