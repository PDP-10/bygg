/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *cmds_rcsid = "$Header: /amd/watsun/w/src1/sun4.bin/cucca/mm/RCS/cmds.H,v 2.1 90/10/04 18:23:39 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * cmds.h - indices into the command dispatch table
 *
 * The value of the symbols below must correspond to the offset of
 * the corresponding command in the cmd_fn[] array!
 */

/*
 * These are also indices into the help string index.  Help topics
 * which are not commands should be #def'd at the end BEFORE NUMTOPICS
 */

#define CMD_ALIAS        0               /* */
#define CMD_ANSWER       1               /* */
#define CMD_BACKTRACK    2               /* */
#define CMD_BCC          3               /* */
#define CMD_BLANK        4               /* */
#define CMD_BROWSE       5               /* */
#define CMD_BUG          6               /* */
#define CMD_CC           7               /* */
#define CMD_CD           8               /* */
#define CMD_CHECK        9               /* */
#define CMD_CONTINUE     10              /* */
#define CMD_COPY         11              /* */
#define CMD_COUNT        12              /* */
#define CMD_CREATE_INIT  13              /* */
#define CMD_DAYTIME      14              /* */
#define CMD_DEBUG        15              /* */
#define CMD_MEMDEBUG     16              /* */
#define CMD_DEFINE       17              /* */
#define CMD_DELETE       18              /* */
#define CMD_DISPLAY      19              /* */
#define CMD_ECHO         20              /* */
#define CMD_EDIT         21              /* */
#define CMD_ERASE        22              /* */
#define CMD_EXAMINE      23              /* */
#define CMD_EXIT         24              /* */
#define CMD_EXPUNGE      25              /* */
#define CMD_FCC          26              /* */
#define CMD_FINGER       27              /* */
#define CMD_FLAG         28              /* */
#define CMD_FOLLOW       29              /* */
#define CMD_FORWARD      30              /* */
#define CMD_FROM         31              /* */
#define CMD_GET          32              /* */
#define CMD_HEADERS      33              /* */
#define CMD_HELP         34              /* */
#define CMD_INSERT       35              /* */
#define CMD_IN_REPLY_TO  36              /* */
#define CMD_JUMP         37              /* */
#define CMD_KEYWORD      38              /* */
#define CMD_KILL         39              /* */
#define CMD_LIST         40              /* */
#define CMD_LITERAL      41              /* */
#define CMD_MARK         42              /* */
#define CMD_MOVE         43              /* */
#define CMD_NEXT         44              /* */
#define CMD_PREVIOUS     45              /* */
#define CMD_PRINT        46              /* */
#define CMD_PROFILE      47              /* */
#define CMD_PUSH         48              /* */
#define CMD_PWD          49              /* */
#define CMD_QUIT         50              /* */
#define CMD_QQUIT        51              /* */
#define CMD_READ         52              /* */
#define CMD_REMAIL       53              /* */
#define CMD_REMOVE       54              /* */
#define CMD_REPLY        55              /* */
#define CMD_REPLY_TO     56              /* */
#define CMD_RESTORE_DRAFT 57             /* */
#define CMD_REVIEW       58              /* */
#define CMD_ROUTE        59              /* */
#define CMD_SAVE_DRAFT   60              /* */
#define CMD_SEND         61              /* */
#define CMD_SET          62              /* */
#define CMD_SHOW         63              /* */
#define CMD_SMAIL        64              /* */
#define CMD_SORT         65              /* */
#define CMD_SPELL        66              /* */
#define CMD_STATUS       67              /* */
#define CMD_SUBJECT      68              /* */
#define CMD_SUSPEND      69              /* */
#define CMD_TAKE         70              /* */
#define CMD_TEXT         71              /* */
#define CMD_TO           72              /* */
#define CMD_TYPE         73              /* */
#define CMD_UNANSWER     74              /* */
#define CMD_UNDELETE     75              /* */
#define CMD_UNFLAG       76              /* */
#define CMD_UNKEYWORD    77              /* */
#define CMD_UNMARK       78              /* */
#define CMD_USER_HEADER  79              /* */
#define CMD_VERSION      80              /* */
#define CMD_WHO          81              /* */
#define CMD_WRITE        82              /* */
#define CMD_Z            83              /* */

#define CMD_HEADER       84              /* */
#define CMD_EXPAND       85              /* */

#define CMD_SENDER       86              /* */
#define CMD_ALL          87              /* */

#define CMD_INCLUDE      88              /* */
#define CMD_NOINCLUDE    89              /* */

#define HLP_CCMD         90              /* */
#define HLP_MESSAGE_SEQUENCE 91          /* */
#define HLP_MMINIT       92              /* */
#define HLP_MM           93              /* */
#define HLP_SHELL        94              /* */

#define HLP_TYPE_BABYL   95              /* */
#define HLP_TYPE_START HLP_TYPE_BABYL
#define HLP_TYPE_MBOX    96              /* */
#define HLP_TYPE_MH      97              /* */
#define HLP_TYPE_MTXT    98              /* */
#define HLP_TYPE_POP2    99              /* */
#define HLP_TYPE_POP3    100             /* */

#define NUMTOPICS        101             /* */
