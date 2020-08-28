/*
** This module implements driver code for the DEC PDP-10 processor.
*/

#include "disass.h"

evf_init pdp10_init;
evf_help pdp10_help;

struct entryvector pdp10_vector = {
  "pdp10",			/* Name */
  "DEC PDP10",			/* One-liner */
  pdp10_init,			/* Init routine */
  pdp10_help,			/* Help routine */
};

/*
** Global variables, from module common:
*/

extern char statuschar;

extern bool delayblank;

/**********************************************************************/

/* variables: */

static longword lhw, rhw;

/**********************************************************************/

static void get2hw(void) {
  byte b;

  lhw = getbyte() << 10;
  lhw += (getbyte() << 2);
  b = getbyte();
  rhw = (b & 077) << 12;
  rhw += (getbyte() << 4);
  rhw += (getbyte() & 017);
}

static char* opname[] = {	/* Instruction names: */
  "z", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0... */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 20... */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 40... */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 60... */

  "ujen",   0,        0,        0,			/* 100... */
  "jsys",   0,        0,        0,			/* 104... */
  0, 0, 0, 0, 0, 0, 0, 0,				/* 110... */
  0, 0, 0, 0, 0, 0, 0, 0,				/* 120... */
  0,        0,        0,        "ibp",			/* 130... */
  "ildb",   "ldb",    "idbp",   "dpb",			/* 134... */
  0, 0, 0, 0, 0, 0, 0, 0,				/* 140... */
  0, 0, 0, 0, 0, 0, 0, 0,				/* 150... */
  0, 0, 0, 0, 0, 0, 0, 0,				/* 160... */
  0, 0, 0, 0, 0, 0, 0, 0,				/* 170... */

  "move",   "movei",  "movem",  "moves",		/* 200... */
  "movs",   "movsi",  "movsm",  "movss",		/* 204... */
  "movn",   "movni",  "movnm",  "movns",		/* 210... */
  "movm",   "movmi",  "movmm",  "movms",		/* 214... */
  "imul",   "imuli",  "imulm",  "imulb",		/* 220... */
  "mul",    "muli",   "mulm",   "mulb",			/* 224... */
  "idiv",   "idivi",  "idivm",  "idivb",		/* 230... */
  "div",    "divi",   "divm",   "divb",			/* 234... */
  "ash",    "rot",    "lsh",    "jffo",			/* 240... */
  "ashc",   "rotc",   "lshc",   0,			/* 244... */
  "exch",   "blt",    "aobjp",  "aobjn",		/* 250... */
  "jrst",   "jfcl",   "xct",    "map",			/* 254... */
  "pushj",  "push",   "pop",    "popj",			/* 260... */
  "jsr",    "jsp",    "jsa",    "jra",			/* 264... */
  "add",    "addi",   "addm",   "addb",			/* 270... */
  "sub",    "subi",   "subm",   "subb",			/* 274... */

  "cai",    "cail",   "caie",   "caile",		/* 300... */
  "caia",   "caige",  "cain",   "caig",			/* 304... */
  "cam",    "caml",   "came",   "camle",		/* 310... */
  "cama",   "camge",  "camn",   "camg",			/* 314... */
  "jump",   "jumpl",  "jumpe",  "jumple",		/* 320... */
  "jumpa",  "jumpge", "jumpn",  "jumpg",		/* 324... */
  "skip",   "skipl",  "skipe",  "skiple",		/* 330... */
  "skipa",  "skipge", "skipn",  "skipg",		/* 334... */
  "aoj",    "aojl",   "aoje",   "aojle",		/* 340... */
  "aoja",   "aojge",  "aojn",   "aojg",			/* 344... */
  "aos",    "aosl",   "aose",   "aosle",		/* 350... */
  "aosa",   "aosge",  "aosn",   "aosg",			/* 354... */
  "soj",    "sojl",   "soje",   "sojle",		/* 360... */
  "soja",   "sojge",  "sojn",   "sojg",			/* 364... */
  "sos",    "sosl",   "sose",   "sosle",		/* 370... */
  "sosa",   "sosge",  "sosn",   "sosg",			/* 374... */

  "setz",   "setzi",  "setzm",  "setzb",		/* 400... */
  "and",    "andi",   "andm",   "andb",			/* 404... */
  "andca",  "andcai", "andcam", "andcab",		/* 410... */
  "setm",   "setmi",  "setmm",  "setmb",		/* 414... */
  "andcm",  "andcmi", "andcmm", "andcmb",		/* 420... */
  "seta",   "setai",  "setam",  "setab",		/* 424... */
  "xor",    "xori",   "xorm",   "xorb",			/* 430... */
  "ior",    "iori",   "iorm",   "iorb",			/* 434... */
  "andcb",  "andcbi", "andcbm", "andcbb",		/* 440... */
  "eqv",    "eqvi",   "eqvm",   "eqvb",			/* 444... */
  "setca",  "setcai", "setcam", "setcab",		/* 450... */
  "orca",   "orcai",  "orcam",  "orcab",		/* 454... */
  "setcm",  "setcmi", "setcmm", "setcmb",		/* 460... */
  "orcm",   "orcmi",  "orcmm",  "orcmb",		/* 464... */
  "orcb",   "orcbi",  "orcbm",  "orcbb",		/* 470... */
  "seto",   "setoi",  "setom",  "setob",		/* 474... */

  "hll",    "hlli",   "hllm",   "hlls",			/* 500... */
  "hrl",    "hrli",   "hrlm",   "hrls",			/* 504... */
  "hllz",   "hllzi",  "hllzm",  "hllzs",		/* 510... */
  "hrlz",   "hrlzi",  "hrlzm",  "hrlzs",		/* 514... */
  "hllo",   "hlloi",  "hllom",  "hllos",		/* 520... */
  "hrlo",   "hrloi",  "hrlom",  "hrlos",		/* 524... */
  "hlle",   "hllei",  "hllem",  "hlles",		/* 530... */
  "hrle",   "hrlei",  "hrlem",  "hrles",		/* 534... */
  "hrr",    "hrri",   "hrrm",   "hrrs",			/* 540... */
  "hlr",    "hlri",   "hlrm",   "hlrs",			/* 544... */
  "hrrz",   "hrrzi",  "hrrzm",  "hrrzs",		/* 550... */
  "hlrz",   "hlrzi",  "hlrzm",  "hlrzs",		/* 554... */
  "hrro",   "hrroi",  "hrrom",  "hrros",		/* 560... */
  "hlro",   "hlroi",  "hlrom",  "hlros",		/* 564... */
  "hrre",   "hrrei",  "hrrem",  "hrres",		/* 570... */
  "hlre",   "hlrei",  "hlrem",  "hlres",		/* 574... */

  "trn",    "tln",    "trne",   "tlne",			/* 600... */
  "trna",   "tlna",   "trnn",   "tlnn",			/* 604... */
  "tdn",    "tsn",    "tdne",   "tsne",			/* 610... */
  "tdna",   "tsna",   "tdnn",   "tsnn",			/* 614... */
  "trz",    "tlz",    "trze",   "tlze",			/* 620... */
  "trza",   "tlza",   "trzn",   "tlzn",			/* 624... */
  "tdz",    "tsz",    "tdze",   "tsze",			/* 630... */
  "tdza",   "tsza",   "tdzn",   "tszn",			/* 634... */
  "trc",    "tlc",    "trce",   "tlce",			/* 640... */
  "trca",   "tlca",   "trcn",   "tlcn",			/* 644... */
  "tdc",    "tsc",    "tdce",   "tsce",			/* 650... */
  "tdca",   "tsca",   "tdcn",   "tscn",			/* 654... */
  "tro",    "tlo",    "troe",   "tloe",			/* 660... */
  "troa",   "tloa",   "tron",   "tlon",			/* 664... */
  "tdo",    "tso",    "tdoe",   "tsoe",			/* 670... */
  "tdoa",   "tsoa",   "tdon",   "tson",			/* 674... */
};

static char* cliname[] = {	/* Calli names: */
  "reset",  "ddtin",  "setddt", "ddtout",		/* 000... */
  "devchr", "ddtgt",  "getchr", "ddtrl",		/* 004... */
  "wait",   "core",   "exit",   "utpclr",		/* 010... */
  "date",   "login",  "aprenb", "logout",		/* 014... */
  "switch", "reassi", "timer",  "mstime",		/* 020... */
  "getppn", "trpset", "trpjen", "runtim",		/* 024... */
  "pjob",   "sleep",  "setpov", "peek",			/* 030... */
  "getlin", "run",    "setuwp", "remap",		/* 034... */
  "getseg", "gettab", "spy",    "setnam",		/* 040... */
  "tmpcor", "dskchr", "sysstr", "jobstr",		/* 044... */
  "struuo", "sysphy", "frechn", "devtyp",		/* 050... */
  "devsts", "devppn", "seek",   "rttrp",		/* 054... */
  "lock",   "jobsts", "locate", "where",		/* 060... */
  "devnam", "ctljob", "gobstr", 0,			/* 064... */
  0,        "hpq",    "hiber",  "wake",			/* 070... */
  "chgppn", "setuuo", "devgen", "othusr",		/* 074... */
  "chkacc", "devsiz", "daemon", "jobpek",		/* 100... */
  "attach", "daefin", "frcuuo", "devlnm",		/* 104... */
  "path.",  "meter.", "mtchr.", "jbset.",		/* 110... */
  "poke.",  "trmno.", "trmop.", "resdv.",		/* 114... */
  "unlok.", "disk.",  "dvrst.", "dvurs.",		/* 120... */
  "xttsk.", "cal11.", "mtaid.", "iondx.",		/* 124... */
  "cnect.", "mvhdr.", "erlst.", "sense.",		/* 130... */
  "clrst.", "piini.", "pisys.", "debrk.",		/* 134... */
  "pisav.", "pirst.", "ipcfr.", "ipcfs.",		/* 140... */
  "ipcfq.", "page.",  "suset.", "compt.",		/* 144... */
  "sched.", "enq.",   "deq.",   "enqc.",		/* 150... */
  "tapop.", "filop.", "cal78.", "node.",		/* 154... */
  "errpt.", "alloc.", "perf.",  "diag.",		/* 160... */
  "dvphy.", "gtntn.", "gtxtn.", "acct.",		/* 164... */
  "dte.",   "devop.", "spprm.", "merge.",		/* 170... */
  "utrp.",  "pijbi.", "snoop.", "tsk.",			/* 174... */
  "kdp.",   "queue.", "recon.", "pitmr.",		/* 200... */
  "acclg.", "nsp.",   "ntman.", "dnet.",		/* 204... */
  "save.",  "cmand.", "piblk.", "scs.",			/* 210... */
  "seblk.", "ctx.",   "piflg.", "ipcfm.",		/* 214... */
  "llmop.", "latop.", "knibt.", "chtrn.",		/* 220... */
  "ethnt.", "entvc.", "netop.", "ddp.",			/* 224... */
  "segop.",						/* 230... */
};

static char* ttcname[16] = {	/* ttcall names: */
  "inchrw", "outchr", "inchrs", "outstr",		/* 0... */
  "inchwl", "inchsl", "getlch", "setlch",		/* 4... */
  "rescan", "clrbfi", "clrbfo", "skpinc",		/* 10... */
  "skpinl", "ioneou", 0,        0,			/* 14... */
};

static bool propcode(word op)
{
  if (op > (sizeof(opname) / sizeof(char*))) {
    return(false);
  }
  if (opname[op] != nil) {
    casestring(opname[op]);
    return(true);
  }
  return(false);
}

static long a2l(address* a)
{
  return(a_a2l(a) / 5);
}

static address* l2a(long l)
{
  return(a_l2a(l * 5));
}

/*
** number () ...
*/

static void number(longword n)
{
  bufoctal(n, 0);
}

/*
** startline() ...
*/

static void startline(bool nonempty)
{
  if (listformat) {
    if (nonempty) {
      bufchar(statuschar);
      bufchar(' ');
      bufoctal(a2l(istart), 6);
      bufstring(": ");
      bufoctal(lhw, 6);
      bufstring(",,");
      bufoctal(rhw, 6);
    }
    tabto(26);
    bufmark();
  }

  if (nonempty) {
    stdlabel();
  }
}

/*
** restline() ...
*/

static void restline(void)
{
  /* not needed yet */
}

/*
** doinstr() tries to output the current item as an instruction.
*/

static void doinstr(void)
{
  byte ac;
  address* a;

  startline(true);

  if (propcode(lhw >> 9)) {
    tabdelim();
    if ((ac = (lhw >> 5) & 017) != 0) {
      bufoctal(ac, 1);
      argdelim(",");
    }
    if (lhw & 020) {
      bufchar('@');
    }
    a = l2a(rhw);
    if (l_exist(a)) {
      bufstring(l_find(a));
    } else {
      bufoctal(rhw, 1);
    }
    if ((ac = lhw & 017) != 0) {
      bufchar('(');
      bufoctal(ac, 1);
      bufchar(')');
    }
  } else {
    casestring("exp");
    tabdelim();
    bufoctal(lhw, 6);
    bufoctal(rhw, 6);
  }
  pb_deadend = true;		/* Until I figure out a way... */
}

/*
** doptr() tries to output the current item as a pointer.
*/

static void doptr(void)
{
  address* a;

  startline(true);

  if (lhw != 0) {
    casestring("xwd");
    tabdelim();
    bufoctal(lhw, 0);
    bufchar(',');
  } else {
    casestring("exp");
    tabdelim();
  }
  a = l2a(rhw);
  if (l_exist(a)) {
    bufstring(l_find(a));
  } else {
    bufoctal(rhw, 0);
  }
}

static void xx(byte b)
{
  if ((b >= 32) && (b < 0177)) {
    bufchar('"');
    if (b == '"') bufchar(b);
    bufchar(b);
    bufchar('"');
  } else {
    bufoctal(b, 0);
  }
}

static void dotext(void)
{
  byte b1, b2, b3, b4, b5;

  b1 =  lhw >> 11;
  b2 = (lhw >> 4) & 0177;
  b3 = (lhw << 3) & 0177;
  b3 += rhw >> 15;
  b4 = (rhw >> 10) & 0177;
  b5 = (rhw >> 1) & 0177;

  startline(true);

  casestring("byte (7) ");
  xx(b1); bufchar(',');
  xx(b2); bufchar(',');
  xx(b3); bufchar(',');
  xx(b4); bufchar(',');
  xx(b5);
}

/*
** doword() is the default handler, it print the data as a 36-bit word.
*/

static void doword(void)
{
  startline(true);

  casestring("exp");
  tabdelim();
  if (lhw != 0) {
    bufoctal(lhw, 0);
    bufoctal(rhw, 6);
  } else {
    bufoctal(rhw, 0);
  }
}

static void checkunmap(address* a)
{
  if (!mapped(a) && l_exist(a)) {
    bufstring(l_find(a));
    bufstring("==");
    number(a2l(a));
    if (c_exist(a)) {
      tabto(32);
      bufchar(';');
      bufstring(c_find(a));
    }
    bufnewline();
  }
}

/**********************************************************************/

void pdp10_spec(address* a, int func)
{
  if (func == SPC_BEGIN) {
    casestring("title");
    tabdelim();
    casestring(".main");
    bufblankline();
    foreach(checkunmap);
    bufblankline();
  }

  if (func == SPC_ORG) {
    bufblankline();
    casestring("org");
    tabdelim();
    bufstring(a_a2str(a));
    bufblankline();
  }

  if (func == SPC_END) {
    bufblankline();
    casestring("end");
  }
}

void pdp10_peek(stcode prefer)
{
  /* We wait with expanding data, labels etc. until we know length... */

  pb_length = 1;		/* Length is in native units. */

  get2hw();

  if (d_exist(istart)) {
    bufblankline();
    startline(false);
    bufdescription(istart, ";");
    bufblankline();
  }

  if ((prefer == st_none) && e_exist(istart)) {
    pb_length = e_length(istart);
    startline(true);
    bufstring(e_find(istart));
  } else {
    switch (pb_status) {
    case st_none:
      /* st_none is handled as instruction: */
    case st_inst:
      doinstr();
      break;
    case st_ptr:
      doptr();
      break;
    case st_text:
      dotext();
      break;
    default:
      doword();
      break;
    }
  }

  stdcomment(32, ";");

  restline();
}

/**********************************************************************/

/*
** Address to string translator:
*/

char* pdp10_a2s(address* a)
{
  static char work[10];
  sprintf(work, "%o", a2l(a));
  return(work);
}

/*
** String to address parser:
*/

static address* pdp10_s2a(char* p)
{
  longword n;			/* At least 18 bits needed. */
  char c;

  n = 0;
  while ((c = *p++) != (char) 0) {
    if ((c < '0') || (c > '7')) {
      return(nil);
    }
    n = (n << 3) + (c - '0');
  }
  n = n & 0777777;
  return(l2a(n));
}

/*
** Canonicalize a label:
*/

char* pdp10_lcan(char* name)
{
  static char work[10];

  return(canonicalize(name, work, 6));
}

/*
** Check label for valid syntax:
*/

bool pdp10_lchk(char* name)
{
  return(checkstring(name, ".$%", "0123456789.$%"));
}

/*
** Autogenerate a label at specified address:
*/

void pdp10_lgen(address* addr)
{
  /* genlabel(addr); */
}

/*
** Return list of starting addresses:
*/

address* pdp10_auto(void)
{
  return(nil);
}

/*
** Init routine, tell the world that we are big endian and set up
** callback functions.
*/

void pdp10_init(void)
{
  /* Set up our functions: */

  spf_peek(pdp10_peek);
  spf_spec(pdp10_spec);
  spf_a2s( pdp10_a2s);
  spf_s2a( pdp10_s2a);
  spf_lcan(pdp10_lcan);
  spf_lchk(pdp10_lchk);
  spf_lgen(pdp10_lgen);
  spf_auto(pdp10_auto);

  /* set up our variables: */

  pv_bpa = 5;			/* Bytes per Address unit. */
  pv_abits = 18;		/* Number of address bits. */
  pv_bigendian = true;		/* We are big-endian. */
}

/*
** Help handler:
*/

bool pdp10_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help text for PDP10 processors.\n\
");
    return(true);
  }
  return(false);
}
