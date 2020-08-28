/*
** This module implements RPN calculator functions for disass.
*/

#include "comnd.h"
#include "disass.h"

extern pval parseval;		/* We might as well use the ones in the */
extern fdb* used;		/* main module. */

#define stacksize 4

static longword stack[stacksize] = {0x00000000};

static longword startradix = 10;

#define qsize 50

typedef void (quefunc)(longword);

struct queueblock {
  quefunc* f;
  longword a;
};

struct queueblock qdata[qsize];
  
static int qpos;

static void init_queue(void)
{
  qpos = 0;
}

static void do_queue(void)
{
  int i;
  for (i = 0; i < qpos; i += 1) {
    (*qdata[i].f)(qdata[i].a);
  }
  qpos = 0;
}

static void queue(f, arg) void (*f)(); longword arg;
{
  if (qpos < qsize) {
    qdata[qpos].f = f;
    qdata[qpos].a = arg;
  }
  qpos += 1;
}

void push(longword l)
{
  int i;

  for (i = stacksize - 1; i > 0; i -= 1) {
    stack[i] = stack[i - 1];
  }
  stack[0] = l;
}

longword pop(void)
{
  int i;
  longword l;

  l = stack[0];
  for (i = 0; i < stacksize - 1; i += 1) {
    stack[i] = stack[i + 1];
  }
  return(l);
}

void typebinary(longword l)
{
  int b;
  b = l & 1;
  l = l >> 1;
  if (l != 0) {
    typebinary(l);
  }
  if (b == 0) {
    bufchar('0');
  } else {
    bufchar('1');
  }
}

void typeout(longword l, longword radix)
{
  switch (radix) {
    case 2:  typebinary(l); break;
    case 8:  bufoctal(l, 1); break;
    case 10: bufdecimal(l, 1); break;
    case 16: bufhex(l, 1); break;
  }
  bufnewline();
}

void qf_pstack(longword radix)
{
  int i;

  for (i = stacksize - 1; i >= 0; i--) {
    typeout(stack[i], radix);
  }
}

void qf_show(longword radix)
{
  typeout(stack[0], radix);
}

void qf_add(longword l)
{
  l = pop();
  stack[0] += l;
}

void qf_and(longword l)
{
  l = pop();
  stack[0] &= l;
}

void qf_mul(longword l)
{
  l = pop();
  stack[0] *= l;
}

void qf_div(longword l)
{
  l = pop();
  stack[0] /= l;
}

void qf_or(longword l)
{
  l = pop();
  stack[0] |= l;
}

void qf_sub(longword l)
{
  l = pop();
  stack[0] -= l;
}

void qf_lshift(longword l)
{
  UNUSED(l);

  stack[0] = stack[0] << 1;
}

void qf_rshift(longword l)
{
  UNUSED(l);

  stack[0] = stack[0] >> 1;
}

static void execute(f) void (*f)(void); {(*f)();}

static void rpn_bin(void);
static void rpn_oct(void);
static void rpn_dec(void);
static void rpn_hex(void);
static void rpn_pstack(void);

static keywrd cmds[] = {
  { "bin",      0,      (keyval) rpn_bin },
  { "oct",      0,      (keyval) rpn_oct },
  { "dec",      0,      (keyval) rpn_dec },
  { "hex",      0,      (keyval) rpn_hex },
  { "pstack",   0,      (keyval) rpn_pstack },
};

static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
     
static fdb number = { _CMNUM, NUM_US, nil, (pdat) 10, nil, nil, nil };
static fdb fdbkeyw = { _CMKEY, 0, nil, (pdat) &cmdtab, nil, nil, nil };
static fdb fplus  = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "+",
		 "Operator, one of the following: + - * / & | < >", nil, nil };
static fdb fminus = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "-", nil, nil, nil };
static fdb ftimes = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "*", nil, nil, nil };
static fdb fslash = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "/", nil, nil, nil };
static fdb famper = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "&", nil, nil, nil };
static fdb fvbar  = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "|", nil, nil, nil };
static fdb flsh   = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "<", nil, nil, nil };
static fdb frsh   = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) ">", nil, nil, nil };
static fdb fequal = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "=", nil, nil, nil };
static fdb fconfirm = { _CMCFM, 0, nil, nil, nil, nil, nil };

static void rpn_bin(void)
{
  number._cmdat = (pdat) 2;
}

static void rpn_oct(void)
{
  number._cmdat = (pdat) 8;
}

static void rpn_dec(void)
{
  number._cmdat = (pdat) 10;
}

static void rpn_hex(void)
{
  number._cmdat = (pdat) 16;
}

static void rpn_pstack(void)
{
  queue(qf_pstack, (int) number._cmdat);
}

void rpn(void)
{
  number._cmdat = (pdat) startradix;
  init_queue();
  while (true && !false) {
    parse(fdbchn(&fdbkeyw, &number,
		 &fplus, &fminus, &ftimes, &fslash,
		 &famper, &fvbar,
		 &flsh, &frsh, &fequal,
		 &fconfirm, nil),
	  &parseval, &used);
    if (used == &number) queue(push, parseval._pvint);
    if (used == &fdbkeyw) execute(parseval._pvfunc);
    if (used == &fplus) queue(qf_add, 0);
    if (used == &fminus) queue(qf_sub, 0);
    if (used == &ftimes) queue(qf_mul, 0);
    if (used == &fslash) queue(qf_div, 0);
    if (used == &famper) queue(qf_and, 0);
    if (used == &fvbar) queue(qf_or, 0);
    if (used == &flsh) queue(qf_lshift, 0);
    if (used == &frsh) queue(qf_rshift, 0);
    if (used == &fequal) queue(qf_show, (int) number._cmdat);
    if (used == &fconfirm) break;
  }
  do_queue();
  startradix = (longword) number._cmdat;
}
