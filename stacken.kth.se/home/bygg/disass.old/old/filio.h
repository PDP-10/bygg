/* routines in filio.c: */

extern bool wf_bopen(char* filename);
extern bool wf_ropen(char* filename);
extern bool wf_wopen(char* filename);
extern bool wf_close(void);

extern bool iocheck(bool flag);

extern void wf_wchar(char c);
extern void wf_newline(void);
extern void wf_waddr(address* a);
extern void wf_whex(longword l);
extern void wf_w2hex(byte b);
extern void wf_wstr(char* s);

extern void wf_rskip(int count);
extern int  wf_rblock(byte* buffer, int size);

extern char wf_rchar(void);
extern void wf_pushback(void);
extern bool wf_ateof(void);
extern bool wf_is2hex(void);

extern address*  wf_raddr(void);
extern longword  wf_rhex(void);
extern char*     wf_rname(void);
extern char*     wf_rstr(void);
