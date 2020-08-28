/* routines in buffer.c: */

extern void buftop(void);
extern void buffile(char* filename);
extern void bufpipe(char* pipename);
extern void bufdone(void);
extern void bufclose(void);

extern void bufctx(void);

extern void bufshutup(bool silent);
extern void bufxset(bool bufx);
extern bool bufquit(void);

extern void bufchar(char c);
extern void bufstring(char* s);
extern void bufaltstr(char* s);

extern void casechar(char c);
extern void casestring(char* s);
extern void casealtstr(char* s);

extern void bufnewline(void);
extern void bufblankline(void);
extern void tabto(int pos);
extern void bufmark(void);

extern void bufnumber(longword number);
extern void bufoctal(longword number, int digits);
extern void bufdecimal(longword number, int digits);
extern void bufhex(longword number, int digits);

extern void bufsize(longword size);

extern void bufdescription(address* addr, char* cstart);

extern void bufaddress(address* a);
extern void bufsymbol(symindex index);
extern void bufpattern(patindex index);
