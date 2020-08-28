/*
** Main header file for the comnd package.
*/

#include <setjmp.h>
#include <time.h>

typedef unsigned int bool;
#  define false 0
#  define true 1

/* Break character descriptor: */

typedef bool (breakfunc)(int);	/* Function to check breakness of a char. */

typedef struct breakset {
  unsigned char * bitmask;
  unsigned int bitlen;
  breakfunc * testfunc;
  int flags;
#    define BF_xxx 0x0001	/* Flag for something. */
} breakset;

/* Function Descriptor Block: */

typedef struct cmfdb {
  int function;			/* Function code. */
  int flags;			/* Flags for this parse. */
#    define CM_SDH 0x0001	/*   Supress Default Help. */
#    define CM_WLD 0x0002	/*   Allow wildcards. */
  struct cmfdb * next;		/* Link to next fdb in chain. */
  void * data;			/* Data (pointer, whatever). */
  char * help;			/* Help text. */
  char * defanswer;		/* Default answer. */
  breakset * brk;		/* Break mask. */
} cmfdb;

/* Types of things we parse, functions in fdbs, and func. spec. flags: */

enum {
  _CMKEY,			/* Keyword. */
  _CMNUM,			/* Number. */
#    define NUM_US   0x0100	/*   Number is unsigned.  No + or - allowed. */
#    define NUM_UNIX 0x0200	/*   Allow unix format 0..., 0x... and 0b... */
  _CMNOI,			/* Noise word. */
  _CMFIL,			/* Filespec. */
  _CMFLD,			/* Arbitrary field. */
  _CMCFM,			/* Confirm. */
  _CMTXT,			/* Text. */
  _CMTAD,			/* Date-time. */
  _CMQST,			/* Quoted string. */
  _CMTOK,			/* Token. */

  _CMUSR,			/* User name. */
  _CMGRP,			/* Group name. */

  _CMMAC,			/* MAC address. */
  _CMIP4,			/* IP4 address. */
  _CMIP6,			/* IP6 address. */
};

/* Keyword tables/descriptors: */

typedef void (cmfunc)(void);	/* Dispatch function type. */

typedef struct cmkeyword {
  char* key;			/* The actual keyword */
  int flags;			/* Flags: */
#    define KEY_NOR 0x0001	/*   No Recognize. */
#    define KEY_INV 0x0002	/*   Invisible. */
#    define KEY_ABR 0x0004	/*   Abbrev.  Data has full key. */
#    define KEY_EMO 0x0008	/*   Exact Match Only. */
#    define KEY_NOC 0x0010	/*   No Complete. */
#    define KEY_DYN 0x8000	/*   Dynamic key string. (internal flag) */
  void* data;			/* Non-function data. */
  cmfunc* func;			/* Command handler. */
  char* descr;			/* Short description, for help. */
} cmkeyword;

typedef struct cmkeytab {
  int count;			/* Number of active entrys. */
  cmkeyword* keys;		/* Pointer to keyword table. */
  int flags;			/* Flags: */
#    define KT_MWL 0x0001	/*   Multi-Word Line help. */
#    define KT_LTR 0x0002	/*   Left-To-Right listing. */
#    define KT_EMO 0x0008	/*   Force KEY_EMO for all keys. */
#    define KT_NOC 0x0010	/*   Force KEY_NOC for all keys. */
#    define KT_DYN 0x8000	/*   Dynamic table. (internal flag) */
  int size;			/* Max size of table, if dynamic. */
} cmkeytab;

/* return values: */

typedef struct {
  cmfdb* used;			/* The FDB that parsed successfully. */
  cmkeyword* kw;		/* Pointer to keyword struct that matched. */

  struct _pvnum {		/* Number: */
    int sign;			/*   +1 or -1. */
    unsigned long long magnitude; /* Magnitude. */
    long long number;		/*   Computed number. */
  } num;

  struct _pvmac {		/* MAC address: */
    unsigned char addr[6];	/*   Actual address. */
  } mac;

  struct _pvip4 {		/* IPv4 address: */
    unsigned char addr[4];	/*   Actual address. */
    int length;			/*   Prefix length. */
  } ip4;

  struct _pvip6 {		/* IPv6 address: */
    unsigned char addr[16];	/*   Actual address. */
    int length;			/*   Prefix length. */
  } ip6;

  struct _pvtbl {		/* Table lookup etc: */
    bool  ambig;		/*   Ambigous. */
    bool  exact;		/*   Exact match. */
    int   offset;		/*   Offset to first match. */
    int   count;		/*   Count of matches. */
    char* mptr;			/*   Ptr to continuation. */
    int   mlen;			/*   Len of continuation. */
  } tbl;
} cmparseval;

/* Routines: */

extern void  cm_pkey(char* help, int flags, cmkeyword* keys, int kflags);
extern void  cm_pcmd(char* help, int flags, cmkeyword* keys, int kflags);
extern void  cm_pkey_kt(char* help, int flags, cmkeytab* kt);
extern void  cm_pcmd_kt(char* help, int flags, cmkeytab* kt);

extern void  cm_pnum(char* help, int flags, int radix);
extern void  cm_pnoi(char* help, int flags, char* noise);
extern void  cm_pfil(char* help, int flags);
extern void  cm_pwrd(char* help, int flags);
extern void  cm_pfld(char* help, int flags, void* foo);
extern void  cm_pcfm(char* help, int flags);
extern void  cm_ptxt(char* help, int flags);
extern void  cm_ptad(char* help, int flags);
extern void  cm_pqst(char* help, int flags);
extern void  cm_ptok(char* help, int flags, char* token);
extern void  cm_pusr(char* help, int flags);
extern void  cm_pgrp(char* help, int flags);
extern void  cm_pmac(char* help, int flags);
extern void  cm_pip4(char* help, int flags);
extern void  cm_pip6(char* help, int flags);

extern void  cm_dispatch(void);

extern void  cm_noise(char* message);
extern void  cm_confirm(void);

extern char* cm_password(char* prompt);
extern char* cm_texti(char* prompt);
extern void  cm_parse(cmfdb* chain);

extern cmfdb* cm_chain(cmfdb* first, ...);
extern cmfdb* cm_fdb(int type, char* help, int flags, void* data);
extern cmkeytab* cm_ktab(cmkeyword* kw, int flags);

extern bool  cm_tbadd(cmkeytab* kt, char* name, int offset);
extern bool  cm_tbdel(cmkeytab* kt, char* name);
extern bool  cm_tbluk(cmkeytab* kt, char* name);

extern void  cm_init(void);
extern void  cm_exit(void);

extern void  cm_setprompt(char* prompt);
extern void  cm_setinput(int (*reader)(void));

#define cm_seterror() { extern jmp_buf cm_erbuf; setjmp(cm_erbuf); }
#define cm_setreparse() { extern jmp_buf cm_rpbuf; setjmp(cm_rpbuf); }
#define cm_prompt(prompt) cm_seterror(); cm_setprompt(prompt); cm_setreparse();

/* Visible variables: */

extern cmparseval pval;
extern char* atombuffer;
