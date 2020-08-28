/* routines etc. in common.c: */

/*
** Data types:
*/

typedef struct {
  byte opcode;
  byte length;
  byte itype;
  byte flags;
  char* expand;
} dispblock;

#define arnold 0xff		/* terminator. */

/* The thing below is temporary: */

#define  casing_default    0	/*   Default, as "normally" used. */
#define  casing_lower      1    /*   Lower case.   "foobar" */
#define  casing_upper      2    /*   Upper case.   "FOOBAR" */
#define  casing_initial    3    /*   Initial case. "Foobar" */

extern int       casing;	/* To be DELETED RSN. */

/*
** Callback function for watching refs:
*/

typedef void (wbcallback)(int handle, address* addr, bool exitcall);

/*
** Variables:
*/

extern address*  pc;		/* Virtual program counter. */
extern address*  istart;	/* Start of current object. */

extern bool      updateflag;	/* Updating things? */
extern bool      listformat;	/* Listing format? */
extern bool      unixflag;	/* Unix format? */

extern bool      delayblank;	/* Want object terminated with a blank line? */

extern struct entryvector* processor;

extern int       pv_bpa;	/* Bytes per Address unit. */
extern int       pv_bpl;	/* Bytes per line. */
extern int       pv_abits;	/* Number of address bits. */
extern bool      pv_bigendian;	/* Big- or little endian? */
extern char*     pv_cstart;	/* Comment start string. */

extern stcode    defobjstatus;	/* Default object typeout mode, if unknown. */

extern int       argindex;	/* Item number on line. */
extern stcode    argstatus;	/* Override status for this item. */
extern int       argradix;	/* Override radix for this item. */

extern address*  pb_touch;	/* Touchy, aren't we? */
extern int       pb_length;	/* Computed length of object, in addr units. */
extern stcode    pb_prefer;	/* Prefered status. */
extern stcode    pb_actual;	/* Actual status. */
extern stcode    pb_status;	/* ... */
extern address*  pb_detour;	/* Where we detour, if we do. */
extern bool      pb_deadend;	/* Instruction stream ends here. */

/*
** routines:
*/

extern dispblock* finddisp(byte b, dispblock tbl[]);

extern bool partmatch(char* string, char* pattern);

extern void setproc(struct entryvector* proc);
extern bool pcheck(void);

extern byte getbyte(void);
extern word getword(void);
extern longword getlong(void);

extern void pushpc(void);
extern void poppc(void);
extern void setpc(address* addr);

extern void setdot(address* addr);
extern address* getdot(int numdots);

extern void ls_dots(void);
extern void ls_highlights(void);
extern void ls_notes(void);
extern void ls_patterns(void);
extern void ls_register(regindex index);
extern void ls_symbols(void);

extern void reference(address* addr);
extern void endref(void);
extern int  setwatch(address* addr, wbcallback* callback, bool exitflag);
extern void clearwatch(int handle);

extern int scanref(address* addr);

extern long sextb(byte b);
extern long sextw(word w);
extern long sextl(longword l);

extern char st2char(stcode s);
extern stcode char2st(char c);

extern void starthex(int count);
extern void stdlabel(void);
extern void stdcomment(int column, char* cstart);
extern void stddescription(void);

extern void startline(bool nonempty);
extern void restline(void);

extern void checkblank(void);

extern char* scantext(int maxlength);
extern bool printable(byte b);

extern void argstep(void);
extern void argdelim(char* delim);
extern void opdelim(char* delim);
extern void spacedelim(void);
extern void tabdelim(void);

extern bool overrun(void);

extern void peek(address* a, int flags, stcode prefer);
extern void spec(address* a, int func);
extern address* autolist(void);

extern int follow_code(address* a);

extern int pe_size(address* a, pattern* p);

extern char* a_a2str(address* a);
extern address* a_str2a(char* s);

extern char* l_canonical(char* name);
extern bool  l_check(char* name);
extern void  l_generate(address* addr);

extern char* canonicalize(char* src, char* dst, int len);
extern bool  checkstring(char* string, char* first, char* rest);

extern char* charname(byte b);
extern void  mkccomment(byte b);

extern void com_rcre(regindex index);
extern void com_rdel(regindex index);

extern void com_scre(symindex index);
extern void com_sdel(symindex index);
extern void com_sset(symindex index);

/* Processor functions: */

extern evf_exit* pf_exit;	/* Exit handler. */
extern evf_peek* pf_peek;	/* Main workhorse. */
extern evf_spec* pf_spec;	/* Special functions. */

extern evf_a2s*  pf_a2s;	/* Address-to-string translator. */
extern evf_s2a*  pf_s2a;	/* String-to-address translator. */

extern evf_lcan* pf_lcan;	/* Canonicalize label. */
extern evf_lchk* pf_lchk;	/* Check label for valid syntax. */
extern evf_lgen* pf_lgen;	/* Make up a label at spec. addr. */

extern evf_rcre* pf_rcre;	/* Create register. */
extern evf_rdel* pf_rdel;	/* Delete register. */

extern evf_scre* pf_scre;	/* Create symbol. */
extern evf_sdel* pf_sdel;	/* Delete symbol. */
extern evf_sset* pf_sset;	/* Assign symbol. */

extern evf_auto* pf_auto;	/* Return auto points. */
extern evf_cchk* pf_cchk;	/* Check if char is printable. */

/* functions to set pf_???: */

extern void spf_exit(evf_exit* handler);
extern void spf_peek(evf_peek* handler);
extern void spf_spec(evf_spec* handler);

extern void spf_a2s (evf_a2s*  handler);
extern void spf_s2a (evf_s2a*  handler);

extern void spf_lcan(evf_lcan* handler);
extern void spf_lchk(evf_lchk* handler);
extern void spf_lgen(evf_lgen* handler);

extern void spf_rcre(evf_rcre* handler);
extern void spf_rdel(evf_rdel* handler);

extern void spf_scre(evf_scre* handler);
extern void spf_sdel(evf_sdel* handler);
extern void spf_sset(evf_sset* handler);

extern void spf_auto(evf_auto* handler);
extern void spf_cchk(evf_cchk* handler);
