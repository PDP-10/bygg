/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *token_rcsid = "$Header: /amd/watsun/w/src1/sun4.bin/cucca/mm/RCS/token.H,v 2.1 90/10/04 18:26:53 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * tokens.h - token types for RFC822 address parsing
 */

#number 0
#def T_NONE	uninitialized
#def T_ATOM     as defined in RFC822
#def T_COMMENT  as defined in RFC822 (unused)
#def T_LPAREN   "(" (unused)
#def T_RPAREN   ")" (unused)
#def T_COMMA	","
#def T_LROUTE   "<"
#def T_RROUTE   ">"
#def T_AT       "@"
#def T_SEMI     ";"
#def T_COLON    ":"
#def T_QPAIR    RFC822 quoted-pair
#def T_QSTR     RFC822 quoted-string
#def T_DOT      "."
#def T_LDOMLIT  "["
#def T_RDOMLIT  "]"
#ifdef MAIL11
#def T_COLCOL	"::"
#endif
#def T_EOH	end of header
/*
 * higher-level parse types
 */
#def T_IGNORE		bad data
#def T_ADDRSPEC		addr-spec
#def T_PHRASEADDR	phrase <route-addr>
#def T_GROUPLIST	group list
#def T_GROUPEND		";" to end a group
#ifdef MAIL11
#def T_MAIL11		mail11 address
#endif
