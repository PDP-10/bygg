/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *parse_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/parse.h,v 2.1 90/10/04 18:25:26 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * parse.h - various definitions needed by modules which do command parsing
 */

#include "ccmd.h"

/*
 * cmseter() and cmsetrp() are redefined to return non-zero whenever the
 * associated longjmps have been taken, making them a little more useful.
 * also, a the error handler for hard errors is redefined so we can catch
 * EOF specially -- cmseteof() must be called at a higher-level than the
 * first call to cmseter!
 */

extern int ccmd_error();

#undef cmseter
#define cmseter() \
  ((cmcsb._cmerh = ccmd_error), setjmp(cmerjb))

#undef cmsetrp
#define cmsetrp() \
  ((cmcsb._cmrph = cmrpjmp), setjmp(cmrpjb))

/*
 * cmseteof() must be called before cmseter(), since we replace ccmd's
 * default error handler with our own, and ours does a longjmp through
 * eofjmpb when EOF is encountered.
 */
extern jmp_buf eofjmpb;
#define cmseteof() \
  setjmp(eofjmpb)

/* global variables declared in mm.c */
extern pval pv;
extern fdb *used;
extern buffer atmbuf;

/* function descriptor blocks declared in cmds.c */
extern fdb mm_top_fdb_abbr, mm_top_fdb_inv, mm_top_fdb_1, mm_top_fdb_2, 
  mm_top_fdb_3, mm_top_fdb_4, mm_top_fdb_5, mm_top_fdb_6, mm_top_fdb_7, 
  mm_send_fdb_abbr, mm_send_fdb_inv, mm_send_fdb_1, mm_send_fdb_2, 
  mm_send_fdb_3, mm_send_fdb_4, mm_send_fdb_5, 
  mm_read_fdb_abbr, mm_read_fdb_inv, mm_read_fdb_1, mm_read_fdb_2, 
  mm_read_fdb_3, mm_read_fdb_4, mm_read_fdb_5, mm_read_fdb_6, mm_read_fdb_7, 
  hdr_cmd_fdb, disp_cmd_fdb, erase_cmd_fdb, reply_to_fdb, include_fdb;

/* fdbs declared in set.c */
extern fdb set_cmd_fdb;

/* fdbs declared in parse.c */
extern fdb shell_fdb, cfm_fdb;

/* fdbs declared in seq.c */
extern fdb seq_fdb;

/* fdbs declared in parsemsg.c */
extern fdb header_fbd;

/*
 * routines declared in parse.c
 */

time_t p_date(), key2time();
int try_parse (), cmargs (), yesno (), p_num (), pop_input ();
void noise (), confirm (), confirmit (), cmerr (), stack_input ();
void brkch (), unbrk ();
char *parse_text (), *parse_quoted (), *parse_directory (),
    *parse_input_file (), *parse_output_file (), *parse_username (),
    *parse_keyword (), **parse_keylist ();

/*
 * other external declarations
 */

extern int (*mm_cmds[])();

/*
 * some useful macros
 */

#define interactive	(cmcsb._cmflg | CM_TTY)

/* save_parse_context
 *
 * this macro must be invoked as the last storage declaration in the
 * enclosing procedure; it declares and initializes jmp_bufs used to save
 * the caller's parsing context, so restore_parse_context() can restore
 * them before returning
 */

#define save_parse_context() \
    jmp_buf erhjmp, rphjmp; \
    bcopy(cmerjb,erhjmp,sizeof(erhjmp)), bcopy(cmrpjb,rphjmp,sizeof(rphjmp))

#define restore_parse_context() \
    bcopy(erhjmp,cmerjb,sizeof(erhjmp)), bcopy(rphjmp,cmrpjb,sizeof(rphjmp))

extern keytab formattab;

typedef struct setkey {
    keytab *keytab;
    string current;
} setkey;

extern setkey default_mail_type;

#define NOVICE 0
#define EXPERT 1
