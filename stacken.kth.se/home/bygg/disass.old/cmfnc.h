/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Andrew Lowry
*/
/* cmfnc.h
**
** This file contains function-specific symbols and declarations needed
** by application programs that wish to make use of the ccmd package.
** It is generated by m4 by means of the cmfnc.h4 file in conjunction with
** individual configuration files (as in cm???.cnf).  The initial portion
** of the file, including this comment, is copied verbatim from the file
** cmfnc.top.
**
** Function codes are given names like _CMTTT, where TTT is the three-
** letter abbreviation for the type, and are assigned small positive
** integer values.
**
** Function-specific error codes are given names of the form TTTxEEE,
** where TTT is the three-letter abbreviation for the parse type, and
** EEE is an abbreviation for the error.  The values assigned to these
** symbols are in two parts.  The left half contains the function code
** for the parse function, and the right half contains an integer
** in the range 0 to n-1, where there are n errors defined for the parse
** type.  Generic error codes for the ccmd package as a whole are given
** names like CMxEEE, and have zeros in the left half.
**
** Flag values are generally given names like TTT_FFF, where FFF is a
** mnemonic for the flag.  Other constants are given names like _TTTCCC,
** where CCC is a mnemonic for the constant.
**/

/* Macro to build error code values from the function code and error
** sub-code values.
**/

#define CMERR(fcode,ecode)	((fcode << 8) | ecode)



/* Generic ccmd package error codes, all with zero in left half */

#define CMxOK	CMERR(0,0)
#define CMxRPT	CMERR(0,1)
#define CMxNOP	CMERR(0,2)
#define CMxUNKF	CMERR(0,3)
#define CMxBOVF	CMERR(0,4)
#define CMxAOVF	CMERR(0,5)
#define CMxEOF	CMERR(0,6)
#define CMxINC	CMERR(0,7)
#define CMxNFDB	CMERR(0,8)
#define CMxGO	CMERR(0,9)
#define CMxDFR	CMERR(0,10)
#define CMxNOAR	CMERR(0,11)
#define CMxNDEF	CMERR(0,12)
#define CMxIOVF	CMERR(0,13)
#define CMxIO	CMERR(0,14)
#define CMxBUFS	CMERR(0,15)
#define CMxPMT	CMERR(0,16)
#define CMxAGN	CMERR(0,17)
#define CMxNOAE	CMERR(0,18)
#define CMxBEL	CMERR(0,19)
#define CMxBEG	CMERR(0,20)
#define CMxSOF	CMERR(0,21)
#define CMxSUF	CMERR(0,22)
#define CMxPRE	CMERR(0,23)

/* Configuration information for cmcfm */

#define _CMCFM 1

/* Macro to prevent loading of cmcfm support */

#define CFM_STUB ftspec ft_cfm = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_cfm = { 0, NULL };


/* cfm error codes */

#define CFMxNOC	CMERR(1,0)

/* Configuration information for cmkey */

#define _CMKEY 2

/* Macro to prevent loading of cmkey support */

#define KEY_STUB ftspec ft_key = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_key = { 0, NULL };


/* keyword parse errors */

#define KEYxNM	CMERR(2,0)
#define KEYxAMB	CMERR(2,1)
#define KEYxABR	CMERR(2,2)

/*
 * KEYWRD structure specifies one entry in a keyword table.  KEYTAB
 * structure describes a table of keywords.
 */

#ifdef notdef
/*
 * we want to return "anything" as the value of a keyword,
 * so we need to know what is the 'largest' type in a machine independent 
 * way.   I can't figure out a way to do this at compile time, so we'll use a 
 * generated  file.
 */

#include "cmkeyval.h"		/* this defines the keyval type */
#endif

#ifdef KEYVAL_TYPE
typedef KEYVAL_TYPE keyval;
#else
typedef long keyval;
#endif


typedef struct KEYWRD {
	char *	_kwkwd;	/* keyword string */
	short	_kwflg;	/* flags (see below) */
	keyval	_kwval;	/* arbitrary value, not used internally */
			/*  except for abbreviations... see KEY_ABR */
			/*  flag below */
} keywrd;

typedef struct KEYTAB {
	int	_ktcnt;	/* number of keywords in table */
	keywrd * _ktwds;/* array of keyword entries */
	int	_ktwid;	/* fixed column width for help */
} keytab;

/* Valid flags in fdb */
#define KEY_EMO 0x0001		/* exact match only */
#define KEY_PTR 0x0002		/* return pointer to keywrd struct */
#define KEY_WID 0x0004		/* fixed width on help */

/* flags that can be present in a keyword entry */

#define KEY_ABR 0x0008		/* keyword is an abbreviation for the */
			/* keyword indexed by this entry's _kwval value */

#define KEY_NOR 0x0010		/* Ignore this keyword (do not recognize */
			/*  any prefix, or even an exact match) */
#define KEY_IGN KEY_NOR
#define KEY_INV 0x0020		/* Invisible keyword (not shown with help) */
#define KEY_MAT 0x0040		/* This keyword matches current input */
			/*  (used internally) */


/* Configuration information for cmnum */

#define _CMNUM 3

/* Macro to prevent loading of cmnum support */

#define NUM_STUB ftspec ft_num = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_num = { 0, NULL };


/* number parsing errors */

#define NUMxRAD	CMERR(3,0)
#define NUMxSGN	CMERR(3,1)
#define NUMxNP	CMERR(3,2)
#define NUMxOV	CMERR(3,3)

/* Parse flags for number parse */

#define NUM_US 0x0001		/* unsigned integer parse */
#define NUM_BNP 0x0002		/* integer can break on non-punctuation */
#define NUM_PO 0x0004		/* parse-only (no conversion to binary) */

/* Configuration information for cmqst */

#define _CMQST 4

/* Macro to prevent loading of cmqst support */

#define QST_STUB ftspec ft_qst = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_qst = { 0, NULL };


/* quoted string error codes */

#define QSTxQC	CMERR(4,0)
#define QSTxBC	CMERR(4,1)
#define QSTxNP	CMERR(4,2)

/* Configuration information for cmnoi */

#define _CMNOI 5

/* Macro to prevent loading of cmnoi support */

#define NOI_STUB ftspec ft_noi = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_noi = { 0, NULL };


/* noise word parse errors */

#define NOIxNP	CMERR(5,0)

/* Configuration information for cmtxt */

#define _CMTXT 6

/* Macro to prevent loading of cmtxt support */

#define TXT_STUB ftspec ft_txt = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_txt = { 0, NULL };

/* text line parse errors */

#define TXTxNP	CMERR(6,0)

/* Configuration information for cmfld */

#define _CMFLD 7

/* Macro to prevent loading of cmfld support */

#define FLD_STUB ftspec ft_fld = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_fld = { 0, NULL };


/* Field (word) parse errors */

#define FLDxNM	CMERR(7,0)

/* don't allow empty fields */
#define FLD_EMPTY 0x0001
/* all unterminated fields */
#define FLD_WAK 0x0002

/* Configuration information for cmswi */

#define _CMSWI 8

/* Macro to prevent loading of cmswi support */

#define SWI_STUB ftspec ft_swi = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_swi = { 0, NULL };


/* switch parsing errors */

#define SWIxNM	CMERR(8,0)
#define SWIxAMB	CMERR(8,1)
#define SWIxABR	CMERR(8,2)
#define SWIxBEG	CMERR(8,3)
#define SWIxEND	CMERR(8,4)

/*
 * SWTCH structure specifies one entry in a switch table.  SWITAB
 * structure describes a table of switches.  (We use the name
 * swtch instead of switch because the latter is a C reserved word.)
 */

typedef struct SWTCH {
	char *	_swswi;		/* switch string (without punctuation)  */
	short	_swflg;		/* flags (see below) */
	int	_swval;		/* arbitrary value, not used internally */
				/*  except for abbreviations... see SWI_ABR */
				/*  flag below */
} swtch;

typedef struct SWITAB {
	int	_stcnt;		/* number of switches in table */
	swtch  * _stwds;	/* array of switch entries  */
} switab;

/* Flags that can be present in a switch entry */

#define SWI_ABR 0x0001			/* switch is an abbreviation for the */
				/* switch indexed by this entry's _swval */
				/* value */
#define SWI_NOR 0x0002			/* Ignore this switch (do not recognize */
				/*  any prefix, or even an exact match) */
#define SWI_INV 0x0004			/* Invisible switch (not shown with help) */
#define SWI_MAT 0x0008			/* This switch matches current input (used */
				/*  internally) */

/* Configuration information for cmtok */

#define _CMTOK 9

/* Macro to prevent loading of cmtok support */

#define TOK_STUB ftspec ft_tok = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_tok = { 0, NULL };

/* token parse errors */

#define TOKxNP	CMERR(9,0)

/* Wake up on match */
#define TOK_WAK 0x0001

/* Configuration information for cmtad */

#define _CMTAD 10

/* Macro to prevent loading of cmtad support */

#define TAD_STUB ftspec ft_tad = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_tad = { 0, NULL };


#include "datime.h"		/* need these symbols to use _CMTAD */

/* time/date parse errors */

#define TADxNTD	CMERR(10,0)
#define TADxTIM	CMERR(10,1)
#define TADxDAT	CMERR(10,2)
#define TADxDT	CMERR(10,3)


/* Configuration information for cmfil */

#define _CMFIL 11

/* Macro to prevent loading of cmfil support */

#define FIL_STUB ftspec ft_fil = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_fil = { 0, NULL };


/* filename parse errors */

#define FILxNM	CMERR(11,0)
#define FILxAMB	CMERR(11,1)
#define FILxNWLD	CMERR(11,2)
#define FILxINV	CMERR(11,3)
#define FILxBAD	CMERR(11,4)
#define FILxPMA	CMERR(11,5)

/*
 * FILBLK structure describes a data block to be passed to the file
 * name parser
 */
typedef struct FILBLK {
	char **pathv;		/* NULL terminated vector of dirs */
	char *exceptionspec;	/* regexp of exceptions */
	char **def_extension;   /* list of extensions to use */
} filblk;

/*
 * flags that can be present in a filename fdb 
 */

#define FIL_OLD 0x0001			/* existing file */
#define FIL_PO 0x0002			/* nonexisting file */
#define FIL_VAL 0x0004			/* "validate" on PO parse */
#define FIL_DIR 0x0008			/* a directory */
#define FIL_RD 0x0010			/* a readable file */
#define FIL_WR 0x0020			/* a writable file */
#define FIL_EXEC 0x0040			/* an executable file */
#define FIL_WLD 0x0080			/* wild cards allowed */
#define FIL_NOPTH 0x0100			/* only display filename in help */
#define FIL_NOEXT 0x0200			/* don't display extention in help */
#define FIL_TYPE 0x0400			/* display the type of file in help */
#define FIL_NODIR 0x0800			/* don't complete on directories */
#ifdef MSDOS
#define FIL_HID 0x1000			/* a hidden file (MSDOS) */
#define FIL_SYS 0x2000			/* a system file (MSDOS) */
#endif /* MSDOS */
#ifdef undef
#define FIL_REGEXP 0x4000			/* regexp's allowed */
#endif 
#ifdef MSDOS
#define FIL_ALL (FIL_DIR|FIL_RD|FIL_WR|FIL_EXEC|FIL_HID|FIL_SYS)
#else
#define FIL_ALL (FIL_DIR|FIL_RD|FIL_WR|FIL_EXEC)
#endif /* MSDOS */

typedef char **pvfil;		/* return a NULL terminated string vector */



/* Configuration information for cmusr */

#define _CMUSR 12

/* Macro to prevent loading of cmusr support */

#define USR_STUB ftspec ft_usr = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_usr = { 0, NULL };


/* username parse errors */
#define USRxNM	CMERR(12,0)
#define USRxAMB	CMERR(12,1)

/* parse a wild username */
#define USR_WILD 0x0001				/* allow wild users */
#define USR_NOUPD 0x0002				/* defer update of user table */
					/* even if /etc/passwd has been  */
					/* updated */
#define USR_UPDONLY 0x0004				/* forced update of user table */
					/* parse will fail. */
					/* allows table to updated with  */
					/* no parse done. */
					/* when using this flag, you */
					/* must trap errors yourself, or */
					/* a "no such user error" will be */
					/* displayed */
#if unix
#include <pwd.h>
#else
struct passwd {
  int x;
};
#endif

/* Configuration information for cmgrp */

#define _CMGRP 13

/* Macro to prevent loading of cmgrp support */

#define GRP_STUB ftspec ft_grp = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_grp = { 0, NULL };


/* groupname parse errors */
#define GRPxNM	CMERR(13,0)
#define GRPxAMB	CMERR(13,1)

#define GRP_WILD 0x0001				/* allow wild groups */
#define GRP_NOUPD 0x0002				/* defer update of group table */
					/* even if /etc/group has been  */
					/* updated */
#define GRP_UPDONLY 0x0004				/* forced update of group table */
					/* parse will fail. */
					/* allows table to updated with  */
					/* no parse done. */

#if unix
#include <grp.h>
#else
struct group {
  int x;
};
#endif

/* Configuration information for cmpara */

#define _CMPARA 14

/* Macro to prevent loading of cmpara support */

#define PARA_STUB ftspec ft_para = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_para = { 0, NULL };


/*
 * para_actions:
 * a structure to hold an action character, and an associated function
 * to call for that action
 */

typedef struct {
  char actionchar;
  char * (* actionfunc)();
} para_actions;

/*
 * para_data:
 * input data to paragraph parser.
 * holds text to install at the beginning of the buffer, and
 * a NULL terminated vector of para_actions.
 */

typedef struct {
  char *buf;
  para_actions *actions;
} para_data;

#define PARAxNM	CMERR(14,0)

/*
 * the PARA_DEF flag is used to specify that the default actions should be
 * set, and then the user specified actions should be installed.
 * Used to make additions to the default actions
 */
#define PARA_DEF 0x0001

#ifndef DEF_EDITOR
#if unix
#define DEF_EDITOR "emacs"
#endif

#ifdef MSDOS
#ifdef RAINBOW
#define DEF_EDITOR "mince"
#else
#define DEF_EDITOR "epsilon"
#endif /*  RAINBOW */
#endif /*  MSDOS */
#endif



/* Configuration information for cmchar */

#define _CMCHAR 15

/* Macro to prevent loading of cmchar support */

#define CHAR_STUB ftspec ft_char = { NULL, NULL, NULL, 0, NULL }; \
    fnerr fe_char = { 0, NULL };


/* Union declaration for parse return values */

typedef union PVAL {
	int _pvint;
	float _pvflt;
	char _pvchr;
	char *_pvstr;
	char **_pvstrvec;
        keyval _pvkey;
        datime _pvtad;
        pvfil _pvfil;
        struct passwd ** _pvusr;
        struct group ** _pvgrp;
        char * _pvpara;
} pval;
