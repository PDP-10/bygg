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

#define T_NONE           0               /* uninitialized */
#define T_ATOM           1               /* as defined in RFC822 */
#define T_COMMENT        2               /* as defined in RFC822 (unused) */
#define T_LPAREN         3               /* "(" (unused) */
#define T_RPAREN         4               /* ")" (unused) */
#define T_COMMA          5               /* "," */
#define T_LROUTE         6               /* "<" */
#define T_RROUTE         7               /* ">" */
#define T_AT             8               /* "@" */
#define T_SEMI           9               /* ";" */
#define T_COLON          10              /* ":" */
#define T_QPAIR          11              /* RFC822 quoted-pair */
#define T_QSTR           12              /* RFC822 quoted-string */
#define T_DOT            13              /* "." */
#define T_LDOMLIT        14              /* "[" */
#define T_RDOMLIT        15              /* "]" */
#ifdef MAIL11
#define T_COLCOL         16              /* "::" */
#endif
#define T_EOH            17              /* end of header */
/*
 * higher-level parse types
 */
#define T_IGNORE         18              /* bad data */
#define T_ADDRSPEC       19              /* addr-spec */
#define T_PHRASEADDR     20              /* phrase <route-addr> */
#define T_GROUPLIST      21              /* group list */
#define T_GROUPEND       22              /* ";" to end a group */
#ifdef MAIL11
#define T_MAIL11         23              /* mail11 address */
#endif
