/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#include "mm.h"
#include "parse.h"

extern setkey user_level;

/*
 * banner messages for different modes
 */
static char *top_level_banner = "\n\
[ H=headers  R=read  REV=review  S=send   Q=quit  BYE  ?=Hints  HELP ]\n\
";

static char *send_banner = "\n\
[ D=display  S=send  TE=text  ED=edit  TY=type   Q=quit  ?=Hints  HELP ]\n\
";

static char *read_banner = "\n\
[ D=delete  H=header  R=reply  TY=type  PRI=print   Q=quit  ?=Hints  HELP ]\n\
";

/*
 * print out a banner message if the userlevel is novice.   the banner
 * depends on the current mode.
 */

novice_banner(mode)
int mode;
{
    if (strcmp(user_level.current, "novice") != 0)
	return;
    switch(mode) {
    case MM_TOP_LEVEL:
	cmxprintf("%s", top_level_banner);
	break;
    case MM_SEND:
	cmxprintf("%s", send_banner);
	break;
    case MM_READ:
	cmxprintf("%s", read_banner);
	break;
    default:break;
    }
}
