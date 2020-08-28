/*
 *  Routines in buf.c:
 */

extern void bufxset(int flag);
extern void bufchar(char c);
extern void bufstring(char* txt);
extern void bufnumber(int number);
extern void bufhex(unsigned int number, int n);
extern void bufhw(hexaword hw);
extern void bufdec(hexaword hw, int n);
extern void bufnewline(void);
