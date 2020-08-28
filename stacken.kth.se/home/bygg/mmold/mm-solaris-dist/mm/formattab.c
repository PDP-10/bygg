/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/formattab.c,v 2.1 90/10/04 18:24:20 melissa Exp $";
#endif

/*
 * a keyword table of the different formats we handle
 */

#include "mm.h"
#include "parse.h"
#include "rd.h"

/*
 * this has to stay in alphabetical order too, 
 * or they won't line up, will they?
 */
keywrd formatkeys[] = {
    { "babyl",	0,	(keyval) TYPE_BABYL },
    { "mbox",	0,	(keyval) TYPE_MBOX },
    { "mh",	KEY_INV|KEY_NOR, (keyval) TYPE_MH },
    { "mtxt",	0,	(keyval) TYPE_MTXT },
    { "pop2",	KEY_INV|KEY_NOR, (keyval) TYPE_POP2 },
    { "pop3",	KEY_INV|KEY_NOR, (keyval) TYPE_POP3 },
  };

keytab formattab = { (sizeof(formatkeys)/sizeof(keywrd)), formatkeys };

/*
 * default mail file names for different formats
 * must be kept in same order as formatkeys, above
 */
char *defmailfile[] = {
    "~/RMAIL", "~/mbox", "~/mh??", "~/mail.txt", "~/pop2??", "~/pop3??",
};
