/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/rd.c,v 2.0.1.2 1997/10/21 19:33:32 howie Exp $";
#endif

/*
 * rd.c:
 * structure of routines for reading mail files
 */

#include "mm.h"
#include "parse.h"
#define MSGOPS
#include "rd.h"

/* 0 */
extern int babyl_close(), babyl_rdmsg(), babyl_wrmsg(), babyl_probe(),
  babyl_open();
/* 1 */
extern int mbox_close(), mbox_rdmsg(), mbox_wrmsg(), mbox_probe(), 
  mbox_open();
/* 2 */
extern int mh_close(), mh_rdmsg(), mh_wrmsg(), mh_probe(), mh_open();
/* 3 */
extern int mtxt_close(), mtxt_rdmsg(), mtxt_wrmsg(), mtxt_probe(),
  mtxt_open();
/* 4 */
extern int pop2_close(), pop2_rdmsg(), pop2_wrmsg(), pop2_probe(),
  pop2_open();
/* 5 */
extern int pop3_close(), pop3_rdmsg(), pop3_wrmsg(), pop3_probe(),
  pop3_open();

/*
 * this MUST stay in alphabetical order
 * to line up with the keyword table below
 */
msg_handler msg_ops[] = {
					/* babyl (RMAIL) file  */
    { "babyl", babyl_open, babyl_close, babyl_rdmsg, babyl_wrmsg, 
	  babyl_probe },
					/* unix mailbox */
    { "mbox", mbox_open, mbox_close, mbox_rdmsg, mbox_wrmsg, mbox_probe },
					/* mh type (folders) */
    { "mh", mh_open, mh_close, mh_rdmsg, mh_wrmsg, mh_probe },
					/* MM-20 mail.txt */
    { "mtxt", mtxt_open, mtxt_close, mtxt_rdmsg, mtxt_wrmsg, mtxt_probe },
					/* pop2 client */
    { "pop2", pop2_open, pop2_close, pop2_rdmsg, pop2_wrmsg, pop2_probe },
					/* pop 3 client.   not written yet */
    { "pop3", pop3_open, pop3_close, pop3_rdmsg, pop3_wrmsg, pop3_probe },
};
num_msg_ops = sizeof(msg_ops) / sizeof(msg_handler);

/*
 * make sure to update keywrd formatkeys[], which should be in
 * formattab.c
 */
