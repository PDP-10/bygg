/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Howie Kaye
*/


/*
 * cmchar
 *
 * This module contains a parse for a single character.
 * it is trivial.  May be used when a single character of input is 
 * required.
 */

#define	CHARERR				/* char error table allocated here */

#include "ccmdlib.h"			/* get standard symbols */
#include "cmfncs.h"			/* and internal symbols */

/*
 * forward declare parse functions.
 */
int char_parse(), char_help(), char_cplt();

#define charbrk  cmallbk		/* break on everything. */

ftspec ft_char = { char_parse, char_help, char_cplt, 0, &charbrk };

/*
 * char_parse:
 * parse a single character.
 */
PASSEDSTATIC int
char_parse(text, textlen, fdbp, parselen, value)
char *text;
int textlen;
fdb *fdbp;
int *parselen;
pval *value;
{
  *parselen = 0;
  if (textlen < 1) return(CMxINC);	/* no data, no parse */
  *parselen = 1;
  value->_pvchr = text[0];
  return(CMxOK);
}

/*
 * give a help message
 */

PASSEDSTATIC int
char_help(text, textlen, fdbp, cust, lines)
char *text;
int textlen,cust;
fdb *fdbp;
int lines;
{
  if (!cust)
    cmxputs("a character\n");	/* then print the token string */
  return(lines-1);
}

/*
 * character completion.  complete if one character
 */
PASSEDSTATIC int
char_cplt(text,textlen,fdbp,full,cplt,cpltlen)
char *text,**cplt;
int textlen,full,*cpltlen;
fdb *fdbp;
{
  *cplt = NULL; *cpltlen = 0;
  if (textlen == 1) 
    return(CMP_GO | CMP_SPC);
  if (textlen == 0)
    return(CMP_BEL);
  else 
    return(CMP_GO);
}
