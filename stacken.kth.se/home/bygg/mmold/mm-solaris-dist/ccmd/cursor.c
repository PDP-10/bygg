/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Authors: Howie Kaye
*/

/*
 * cursor control
 * handle cursor motion and line updating for command line editing.
 */


#include "ccmdlib.h"
#include "cmfncs.h"

int *
go_from(p1,p2) 
int *p1, *p2;
{
    int l,c;

    if (p1 != p2) {
	relcharpos(&l,&c,p1,p2);
	if (l > 0)
	    go_down_line(l);
	else
	    go_up_line(-l);
	if (c > 0)
	    go_forward(c);
	else
	    go_backward(-c);
    }
    return(p2);
}

int *
goto_end_of_line()
{
  int *p = cmcsb._cmcur;
  int i=0;
  int l,c;

  while(p < cmcsb._cmptr + cmcsb._cminc && ((*p & CC_CHR) != '\n')) {
    if (!(*p & (CC_NEC|CC_HID))) i++;
    p++;
  }
  go_from(cmcsb._cmcur, p);
  return(p);
}


int *
goto_beginning_of_line()
{
    int *bp;
    int l,c;

    for(bp = cmcsb._cmcur-1; bp > cmcsb._cmbfp; bp--)
	if ((*bp & CC_CHR) == NEWLINE) {
	    bp++;
	    break;
	}
    go_from(cmcsb._cmcur, bp);
    return(bp);
}

goto_current_pos()
{
  int *p;

  goto_beginning_of_line();
  for(p = cmcsb._cmbfp; p < cmcsb._cmcur; p++) {
    if (!(*p & (CC_NEC|CC_HID))) 
      cmechx(*p & CC_CHR);
  }
}


go_up_line(n)
{
    for(; n > 0; n--)
	if (!cmupl()) {			/* then move up to next line */
	    cmcr(cmcsb._cmoj);
	    cmputc(BS,cmcsb._cmoj);	/* no luck.. try backspace-return */
	    cmcr(cmcsb._cmoj);
	}
}

go_down_line(n)
{
    for(; n > 0; n--)
	cmdownl();
}

int *
go_forward_char(n)
int n;
{
    go_from(cmcsb._cmcur, cmcsb._cmcur + n);
    return(cmcsb._cmcur + n);
}

int *
go_backward_char(n)
int n;
{
    go_from(cmcsb._cmcur, cmcsb._cmcur - n);
    return(cmcsb._cmcur - n);
}


go_backward(n)
int n;
{
    static char *le = NULL;

    if (n == cmcsb._cmcol) {
	cmputc('\r', cmcsb._cmoj);
	cmcsb._cmcol = 0;
    }
    else {
	for(; n > 0; n--)
	    cmleft();
    }
}

go_forward(n)
int n;
{
    static char *le = NULL;

    for(; n > 0; n--)
	cmright();
}

refresh_eol()
{
    int *cp, *cp1;
    int lines = 0;
    int col;

    

/* 
 * output the line.  If we wrap, must fix subsequent lines.
 * if we hit a '\n' or the end of the buffer, we can stop.
 */
    for(cp = cmcsb._cmcur; cp< cmcsb._cmptr + cmcsb._cminc; cp++) {
	if (!(*cp & (CC_NEC|CC_HID))) {
	    if ((*cp & CC_CHR) == NEWLINE && lines == 0)
		break;
	    cmechx(*cp);
	    if (cmcsb._cmcol == 0) {	/* wrap point? */
		lines++;		/* count the line */
	    }
	}
    }
    go_from(cp, cmcsb._cmcur);
}


/* relcharpos
**
** Purpose:
**   Compute line and column position of end, relative to start.
**   if start is cmcsb._cmbfp, include the prompt.
**
** Input parameters:
**   start, end: start and endpoints of the search
**
** Output parameters:
**   lpos - Number of lines from the one containing the beginning of the
**     prompt string.
**   cpos - Column position (0 = left edge of screen).
**/
relcharpos(lpos,cpos,start,end)
int *lpos,*cpos,*start,*end;
{
    int sl, sc, el, ec;

    if (cmcsb._cmoj == NULL) {		/* not outputting anyway */
	*lpos = *cpos = 0;		/* so don't do anything */
    }
    else {
	abscharpos(start, &sl, &sc);
	abscharpos(end, &el, &ec);
	*lpos = el - sl;
	*cpos = ec - sc;
    }
}

abscharpos(ptr, lpos, cpos)
int *ptr;
int *lpos, *cpos;
{
  int count;				/* counts characters */
  int *cp,cc;				/* for scanning buffer characters */
  char c;

  count = strlen(cmcsb._cmrty);		/* get length of prompt string */
  *lpos = count / (cmcsb._cmcmx);	/* compute prompt end coordinates */
  *cpos = count % (cmcsb._cmcmx);
					/* get # of buffer chars to count */
  cp = cmcsb._cmbfp;			/* point to buffer start */
  while (cp < ptr) {			/* loop through chars */
    c = (cc = *cp++) & CC_CHR;		/* get next char */
    if (cc & CC_NEC)
      continue;				/* non-echoed char... no count */
    else if (c == NEWLINE) {
      (*lpos)++;			/* newline... move to next line */
      *cpos = 0;			/* and reset column counter */
    }
    else if (c == TAB) {		/* TAB character */
      *cpos = 8 + 8*(*cpos / 8);	/* move col to next multiple of 8 */
    }
    else {
      if ((c == DELETE) || (c < SPACE)) /* other control char */
        *cpos += 2;			/* count up-arrow and print char */
      else				/* normal printing char */
        (*cpos)++;			/* count it */
    }
    if (*cpos >= cmcsb._cmcmx) {	/* wrap if necessary */
      (*lpos)++;			/* to next line */
      *cpos = 0;			/* column zero */
    }
  }
}

int*
disp_forward_char(n)
int n;
{
    int *p = cmcsb._cmcur;

    for(; n > 0; n--,p++)
	if (!(*p & (CC_NEC | CC_HID)))
	    cmechx(*p & CC_CHR);
    return(p);
}
