/*
** This module implements RPN calculator functions for disass.
*/

#include "comnd.h"
#include "disass.h"

#define stacksize 4

static longword stack[stacksize] = {0x00000000};

static int startradix = 10;

#define qsize 50

typedef void (quefunc)(longword);

struct queueblock {
  quefunc* f;
  longword a;
};

struct queueblock qdata[qsize];
  
static int qpos;

longword rpn_rstack(int pos)
{
  if ((pos >= 0) && (pos < stacksize)) {
    return stack[pos];
  }
  return 0;
}

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
  if (qpos > 0) {
    qpos = 0;
    wc_dots();
    wc_rpn();
  }
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
  return l;
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

void typeout(longword l, int radix)
{
  switch (radix) {
    //    case 2:  typebinary(l); break;
    case 2:  bufbinary(l, 1); break;
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
    bufchar("XYZT"[i]);
    bufstring(": ");
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

static void rpn_bin(void);
static void rpn_oct(void);
static void rpn_dec(void);
static void rpn_hex(void);
static void rpn_pstack(void);

static cmkeyword cmds[] = {
  { "bin",      0, 0, rpn_bin },
  { "oct",      0, 0, rpn_oct },
  { "dec",      0, 0, rpn_dec },
  { "hex",      0, 0, rpn_hex },
  { "pstack",   0, 0, rpn_pstack },
};

static cmkeytab cmdtab = { (sizeof(cmds)/sizeof(cmkeyword)), cmds };
     
static cmfdb number = { _CMNUM, NUM_US+NUM_UNIX, NULL, (void*) 10 };
static cmfdb fdbkey = { _CMKEY, 0, NULL,  &cmdtab };
static cmfdb fplus  = { _CMTOK, CM_SDH, NULL, "+",
		 "Operator, one of the following: + - * / & | < >" };
static cmfdb fminus = { _CMTOK, CM_SDH, NULL, "-" };
static cmfdb ftimes = { _CMTOK, CM_SDH, NULL, "*" };
static cmfdb fslash = { _CMTOK, CM_SDH, NULL, "/" };
static cmfdb famper = { _CMTOK, CM_SDH, NULL, "&" };
static cmfdb fvbar  = { _CMTOK, CM_SDH, NULL, "|" };
static cmfdb flsh   = { _CMTOK, CM_SDH, NULL, "<" };
static cmfdb frsh   = { _CMTOK, CM_SDH, NULL, ">" };
static cmfdb fequal = { _CMTOK, CM_SDH, NULL, "=" };
static cmfdb fconfirm = { _CMCFM, };

static void rpn_bin(void)
{
  number.data = (void*) 2;
}

static void rpn_oct(void)
{
  number.data = (void*) 8;
}

static void rpn_dec(void)
{
  number.data = (void*) 10;
}

static void rpn_hex(void)
{
  number.data = (void*) 16;
}

static void rpn_pstack(void)
{
  queue(qf_pstack, VP2I(number.data));
}

void rpn(void)
{
  number.data = I2VP(startradix);
  init_queue();
  for (;;) {
    cm_parse(cm_chain(&fdbkey,
		      &fplus, &fminus, &ftimes, &fslash,
		      &famper, &fvbar,
		      &flsh, &frsh, &fequal,
		      &fconfirm, &number,
		      NULL));
    if (pval.used == &number) queue(push, (int) pval.num.number);
    if (pval.used == &fdbkey) cm_dispatch();
    if (pval.used == &fplus)  queue(qf_add, 0);
    if (pval.used == &fminus) queue(qf_sub, 0);
    if (pval.used == &ftimes) queue(qf_mul, 0);
    if (pval.used == &fslash) queue(qf_div, 0);
    if (pval.used == &famper) queue(qf_and, 0);
    if (pval.used == &fvbar)  queue(qf_or, 0);
    if (pval.used == &flsh)   queue(qf_lshift, 0);
    if (pval.used == &frsh)   queue(qf_rshift, 0);
    if (pval.used == &fequal) queue(qf_show, VP2I(number.data));
    if (pval.used == &fconfirm) break;
  }
  do_queue();
  startradix = VP2I(number.data);
}
