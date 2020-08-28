/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *set_rcsid = "$Header: /amd/watsun/w/src1/sun4.bin/cucca/mm/RCS/set.H,v 2.4 90/10/04 18:26:38 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * set.h - defines for "set" command options
 */

#define VAR_BOOLEAN      1               /* */
#define VAR_INTEGER      2               /* */
#define VAR_FILE         3               /* */
#define VAR_ADDRLIST     4               /* */
#define VAR_COMMAND      5               /* */
#define VAR_CMDARGS      6               /* */
#define VAR_CMDKEYS      7               /* */
#define VAR_KEYLIST      8               /* */
#define VAR_TEXT         9               /* */
#define VAR_QUOTEDSTR    10              /* */
#define VAR_FILES        11              /* */
#define VAR_USERNAME     12              /* */
#define VAR_KEYWORDS     13              /* */
#define VAR_OFILE        14              /* */
#define VAR_MAYBE        15              /* */
#define VAR_PROMPT       16              /* */
#define VAR_ALIAS        17              /* */
#define VAR_DIRECTORY    18              /* */

/*
 * The lines below must be kept sorted by name, where "_" sorts
 * before alpha chars.
 */
#define SET_ALIASES_USE_GROUPS 0         /* */
#define SET_APPEND_NEW_MAIL 1            /* */
#define SET_APPEND_SIGNATURE 2           /* */
#define SET_AUTO_CREATE_FILES 3          /* */
#define SET_AUTO_STARTUP_GET 4           /* */
#define SET_AUTOWRAP_COLUMN 5            /* */
#define SET_BROWSE_CLEAR_SCREEN 6        /* */
#define SET_BROWSE_PAUSE 7               /* */
#define SET_CHECK_INTERVAL 8             /* */
#define SET_CLEAR_SCREEN 9               /* */
#define SET_CONTINUOUS_CHECK 10          /* */
#define SET_CONTROL_D_AUTOMATIC_SEND 11  /* */
#define SET_CONTROL_E_EDITOR 12          /* */
#define SET_CONTROL_L_CONFIRM 13         /* */
#define SET_CONTROL_N_ABORT 14           /* */
#define SET_CRT_FILTER   15              /* */
#define SET_DEFAULT_BCC_LIST 16          /* */
#define SET_DEFAULT_CC_LIST 17           /* */
#define SET_DEFAULT_FCC_LIST 18          /* */
#define SET_DEFAULT_FROM 19              /* */
#define SET_DEFAULT_MAIL_TYPE 20         /* */
#define SET_DEFAULT_READ_COMMAND 21      /* */
#define SET_DEFAULT_REPLY_TO 22          /* */
#define SET_DEFAULT_SEND_COMMAND 23      /* */
#define SET_DIRECTORY_FOLDERS 24         /* */
#define SET_DISPLAY_FLAGGED_MESSAGES 25  /* */
#define SET_DISPLAY_OUTGOING_MESSAGE 26  /* */
#define SET_DONT_PRINT_HEADERS 27        /* */
#define SET_DONT_TYPE_HEADERS 28         /* */
#define SET_EDITOR       29              /* */
#define SET_ESCAPE_AUTOMATIC_SEND 30     /* */
#define SET_EXPUNGE_ON_BYE 31            /* */
#define SET_FAST_INIT_FILE 32            /* */
#define SET_FINGER_COMMAND 33            /* */
#define SET_GNUEMACS_MMAIL 34            /* */
#define SET_HANDLE_CHANGED_MODTIME 35    /* */
#define SET_HEADER_OPTIONS_FILE 36       /* */
#define SET_HELP_FILE    37              /* */
#define SET_HELP_DIR     38              /* */
#define SET_INCOMING_MAIL 39             /* */
#define SET_KEYWORDS     40              /* */
#define SET_LIST_INCLUDE_HEADERS 41      /* */
#define SET_LIST_ON_SEPARATE_PAGES 42    /* */
#define SET_MAIL_ALIASES 43              /* */
#define SET_MAIL_DIRECTORY 44            /* */
#define SET_MAIL_FILE    45              /* */
#define SET_MMAIL_PATH   46              /* */
#define SET_MODIFY_READ_ONLY 47          /* */
#define SET_MOVEMAIL_PATH 48             /* */
#define SET_NEW_FILE_MODE 49             /* */
#define SET_ONLY_PRINT_HEADERS 50        /* */
#define SET_ONLY_TYPE_HEADERS 51         /* */
#define SET_PERSONAL_NAME 52             /* */
#define SET_PRINT_FILTER 53              /* */
#define SET_PROMPT_FOR_BCC 54            /* */
#define SET_PROMPT_FOR_CC 55             /* */
#define SET_PROMPT_FOR_FCC 56            /* */
#define SET_PROMPT_RCPT_ALWAYS 57        /* */
#define SET_READ_PROMPT  58              /* */
#define SET_REPLY_ALL    59              /* */
#define SET_REPLY_INCLUDE_ME 60          /* */
#define SET_REPLY_INDENT 61              /* */
#define SET_REPLY_INITIAL_DISPLAY 62     /* */
#define SET_REPLY_INSERT 63              /* */
#define SET_SAVED_MESSAGES_FILE 64       /* */
#define SET_SEND_PROMPT  65              /* */
#define SET_SEND_VERBOSE 66              /* */
#define SET_SENDMAIL_BACKGROUND 67       /* */
#define SET_SENDMAIL_VERBOSE 68          /* */
#define SET_SPELLER      69              /* */
#define SET_SUSPEND_ON_EXIT 70           /* */
#define SET_SUSPEND_ON_QUIT 71           /* */
#define SET_TEMP_DIRECTORY 72            /* */
#define SET_TERSE_TEXT_PROMPT 73         /* */
#define SET_TOP_LEVEL_PROMPT 74          /* */
#define SET_USE_CRT_FILTER_ALWAYS 75     /* */
#define SET_USE_EDITOR_ALWAYS 76         /* */
#define SET_USE_INVALID_ADDRESS 77       /* */
#define SET_USER_HEADERS 78              /* */
#define SET_USER_LEVEL   79              /* */
#define SET_USER_NAME    80              /* */
