/* Since I want prototype defs... */

#include "ccmd.h"

extern void cmdone(void);
extern void cmerjnp(int);
extern void cmermsg(char*, int);
extern char* cmini(void);
extern void cmtake(void (*)(void));
extern void cmxprintf(char*, ...);
extern void confirm(void);
extern void noise(char*);
extern void parse(fdb*, pval*, fdb**);
extern void prompt(char*);

#ifndef MSDOS
#define _pvfunc _pvkey
#endif
