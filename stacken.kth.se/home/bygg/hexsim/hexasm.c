/*
 *  Simple assembler for the hexa processor.
 */

/*
 *  syntax:
 *
 *    label:  expr.....
 *
 *    symbol=expr....
 *
 *            opcode {num} reg,@addr(expr)
 *
 * this is an expression, formed by taking all the sub-expressions on the
 * line and ORing them together.  After this is done, compression from 8
 * to 4 bytes will take place if it can be done.
 *
 *         "{num}"  means take num, shift it into the number field, and
 *                  turn on the L bit.  Just {} is OK, sets L.
 *         "reg,"   means take the reg number, and shift it into the AC
 *                  field.  Check for overflow?  YES, reg field is 4 bits.
 *         "@"      means turn on the I bit.
 *         "addr"   means an address or a number.
 *         "(expr)" means eval the inner expr, and swap its halfwords.
 *                  this can also look like "(reg, sf)", in this case
 *                  sf is the scale factor.  legal values are 1, 2, 4, 8,
 *                  16, 32, 64 or 128 mapping to 0..7 in the XSF field.
 *
 * The character ";" (unless in a string constant) makes the rest of the
 * current line a comment.
 *
 * Operators in expressions:
 *
 *    *   multiplication.
 *    /   division.
 *    +   addition, or unary +.
 *    -   subtraction, or unary -.
 *    &   bitwise and.
 *    |   bitwise or.
 *    ~   bitwise not. (unary)
 *    <>  acts as parenteses.
 *
 * Operands can be names of labels and symbols, the value "." meaning
 * the current location counter, or numeric constants.  The latter can
 * be prefixed with 0, 0x or 0b for octal, hexadecimal or binary.
 *
 * Known pseudo-ops:
 *
 *            .byte   <value>       Reserve one unit of storage, and
 *            .short  <value>       put the given value there.
 *            .half   <value>       (should accept a list)
 *            .word   <value>
 *
 *            .blkb   <size>        Reserve <size> units of storage.
 *            .blkw   <size>
 *
 *            .ascii  "string"      Assemble a string constant, with
 *            .asciz  "string"      or without terminating null char.
 *                                  Any non-blank char be used as the
 *                                  delimiter, and the string constant
 *                                  can contain newlines.
 *
 *            .align  <arg>         Align the location counter on the
 *                                  next 16, 32 or 64 bit boundary,
 *                                  i.e. the argument can be 2, 4 or 8.
 *
 *            .phase  <arg>         Description missing.
 *            .dephase
 *
 *            .pass2                If pass 1, go to pass2 now, from here.
 *
 *            .error  <message>     Well, what do you think?
 *
 *            .if     <expr>        Conditional assembly.
 *            .elsif  <expr>
 *            .else
 *            .endif
 *
 *            .end                  This signals the end of the file,
 *                                  the rest (to physical EOF) will be
 *                                  ignored as a comment.
 *
 * The following should be implemented:
 *
 *            .lit                  Expand literals so far.
 *
 *            .abs                  Use absolute addresses.
 *
 *            .psect
 *
 *            .define               It would be nice to have macros...
 */

/*
 *  All the headers we need.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <inttypes.h>

#include <unistd.h>

#include <ctype.h>

/*
 *  Now, our data types:
 */

typedef uint64_t hexaword;

struct opcode {
  char*    op_name;
  uint8_t  op_code;
  uint8_t  op_accode;
  uint16_t op_eacode;
};

typedef struct pseudo_op {
  char* pop_name;
  void (*pop_handler)(struct pseudo_op* p);

  int pop_data;			/* Function-specific data. */

#   define PD_COND_IF     1	/* .if conditional. */
#   define PD_COND_ELSIF  2	/* .elsif conditional. */
#   define PD_COND_ELSE   3	/* .else conditional. */
#   define PD_COND_ENDIF  4	/* .endif conditional. */

  uint16_t pop_flags;
#  define PF_COND        0x000f	/* Conditional op, like .if or .else */
#    define PF_COND_EQ   0x0001	/*   Condition == */
#    define PF_COND_NE   0x0002	/*   Condition != */
#    define PF_COND_GT   0x0003	/*   Condition >  */
#    define PF_COND_GE   0x0004	/*   Condition >= */
#    define PF_COND_LT   0x0005	/*   Condition <  */
#    define PF_COND_LE   0x0006	/*   Condition <= */
#  define PF_LOAD        0x00f0	/* Load expression. */
#    define PF_LOAD_NONE 0x0000	/*   Don't load anything. */
#    define PF_LOAD_EXPR 0x0010	/*   Load expression. */
#    define PF_LOAD_SYM  0x0020	/*   Load 1 if next symbol is known. */
#    define PF_LOAD_P2   0x0040	/*   Load pass2 flag. */
#    define PF_LOAD_ONE  0x0080	/*   Load 1, always. */
} pop;

typedef struct symbol {
  struct symbol* sym_prev;
  struct symbol* sym_next;

  char     sym_name[8];
  hexaword sym_value;
  void*    sym_data;
  uint32_t sym_flags;
#   define SF_DEF      0x00000001 /* Defined. */
#   define SF_DEF2     0x00000002 /* Defined in pass2. */
#   define SF_REF      0x00000004 /* Referenced. */
#   define SF_FUSE     0x00000008 /* Future symbol used. */
#   define SF_BUILTIN  0x00000010 /* Built-in symbol. */

  uint8_t  sym_type;
#   define ST_FUTURE   1	/* Future. */
#   define ST_PSEUDO   2	/* Pseudo-op. */
#   define ST_OPCODE   3	/* Normal opcode. */
#   define ST_LABEL    4	/* Label. (foo:) */
#   define ST_VALUE    5	/* Value. (sym=4711) */

} symbol;

/*
 *  Token types, returned from token scanner:
 */

enum {
  TK_CHAR,			/* Random character. */
  TK_WORD,			/* Word, as in string-of-chars. */
  TK_NUMBER,			/* Numerical constant. */
  TK_LABEL,			/* Word: */
  TK_SYMBOL,			/* Word= */
  TK_OPCODE,			/* Known opcode. */
  TK_PSEUDO,			/* Pseudo-op. */
  TK_EOL,			/* EOL here. */

  TK_EXCLAM,                    /* "!" */
  TK_DQUOTE,                    /* '"' */
  TK_HASH,                      /* "#" */
                                /* "$" */
                                /* "%" */
  TK_AMP,			/* "&" */
  TK_SQUOTE,                    /* "'" */
  TK_LPAREN,			/* "(" */
  TK_RPAREN,			/* ")" */
  TK_STAR,			/* "*" */
  TK_PLUS,			/* "+" */
  TK_COMMA,			/* "," */
  TK_2COMMA,			/* ",," */
  TK_MINUS,			/* "-" */
  TK_DOT,			/* "." */
  TK_SLASH,			/* "/" */
  TK_COLON,			/* ":" */
  TK_SEMI,			/* ";" */
  TK_LANGEL,			/* "<" */
  TK_EQUAL,			/* "=" */
  TK_RANGEL,			/* ">" */
  TK_QMARK,                     /* "?" */
  TK_ATSIGN,			/* "@" */
  TK_LBRACK,			/* "[" */
  TK_BSLASH,                    /* "\" */
  TK_RBRACK,			/* "]" */
                                /* "^" */
                                /* "_" */
  TK_LCURL,			/* "{" */
  TK_VBAR,			/* "|" */
  TK_RCURL,			/* "}" */
  TK_TILDE,                     /* "~" */
};

/*
 *  Errors.
 */

enum {
  E_NOERROR,			/* No error. */
  E_ILLCHAR,			/* Illegal character. */
  E_ILLARG,			/* Illegal argument. */
  E_ILLNUMBER,			/* Illegal character in number. */
  E_NODIGITS,			/* No digits given. */
  E_UNDEF,			/* Undefined symbol. */

  E_ESTRING,			/* Random string error message. */

  E_NOTEXTD,			/* No text delimiter. */

  E_EXPNUMBER,			/* Number expected. */
  E_EXPRPAREN,			/* Right paren expected. */
  E_EXPRANGEL,			/* Right angel bracket expected. */
  E_EXPLBRACK,			/* Left square bracket expected. */
  E_EXPRBRACK,			/* Right square bracket expected. */
  E_EXPRCURLY,			/* Right curly bracket expected. */
  E_EXPSCALEF,			/* Scale factor expected. */
  E_EXPRESSION,			/* Expression expected. */
  E_EXPSYMBOL,			/* Symbol expected. */

  E_EXPEOL,			/* End-of-line expected. */
};

/*
 *  Save/restore conditional assembly state:
 */

struct cond_save {
  struct cond_save* cs_next;
  int cs_active;
  int cs_virgin;
};

/*
 *  Variables:
 */

hexaword loc;			/* Location counter. */
int pass2;			/* Non-zero if pass 2. */
int endflag;			/* Set non-zero on .end pseudo-op. */

/* Conditional assembly: */

int caactive = 1;		/* Inside active .if?  (or global) */
int cavirgin = 1;		/* Current level has not been active. */
int calevel = 0;		/* Current level. */

struct cond_save* castack;	/* Saved levels. */

/* Scanner/lexer: */

int eofflag;			/* End-of-file seen. */
int eolnflag;			/* End-of-line seen. */
int pushback;			/* Last char needed again? */
char lastchar;			/* Last char read. */

/*
 *  The following are used for error message handling.
 */

#define LINBUFSIZ 132		/* Standard line length. */

char linebuf[LINBUFSIZ];	/* Line buffer, for error handling. */
int linepos;			/* Next pos to store, and line length. */
int errpos;			/* Positioin where error started. */
int errcode;			/* Code for error that occured. */
void* errinfo;			/* Extra info, depending on code. */
int errcount;			/* Total count of errors seen. */

int lineno;			/* Line number. */

int tkagain;			/* Re-use last token? */
int lasttoken;			/* Last token read. */
symbol* lastsym;		/* Last symbol read. */
hexaword lastnum;		/* Last number read. */

int fuseflag;			/* Future symbol used. */

FILE* srcfile;			/* Source file. */
FILE* objfile;			/* Output file. */

symbol* symtab = NULL;		/* Symbol table. */

int debugflag = 0;		/* Print debugging info. */

/*
 *  Not quite so variable, i.e. constants:
 */

const hexaword LHMASK  = 0xffffffff00000000ULL;
const hexaword RHMASK  = 0x00000000ffffffffULL;
const hexaword SIGNBIT = 0x8000000000000000ULL;
const hexaword RHSIGN  = 0x0000000080000000ULL;
const hexaword ZERO    = 0x0000000000000000ULL;

const hexaword OPMASK  = 0xff00000000000000ULL;
const hexaword ACMASK  = 0x00f0000000000000ULL;
const hexaword LBIT    = 0x0008000000000000ULL;
const hexaword NUMASK  = 0x0007ff0000000000ULL;
const hexaword IBIT    = 0x0000008000000000ULL;
const hexaword SFMASK  = 0x0000007000000000ULL;
const hexaword XRMASK  = 0x0000000f00000000ULL;

/*
 *  Tables we need:
 */

void popalign(pop* p);
void popcond(pop* p);
void popdata(pop* p);
void popend(pop* p);
void poperror(pop* p);
void popopdef(pop* p);
void poppass2(pop* p);
void popphase(pop* p);
void popprint(pop* p);
void popspace(pop* p);
void poptext(pop* p);

struct pseudo_op poptab[] = {
  { ".align",   popalign, 0, 0 },
  { ".ascii",   poptext,  0, PF_COND },	/* These two are conditional since */
  { ".asciz",   poptext,  1, PF_COND },	/* they can be multi-line. */
  { ".blkb",    popspace, 1, 0 },
  { ".blkw",    popspace, 8, 0 },
  { ".byte",    popdata,  1, 0 },
  { ".dephase", popphase, 0, 0 },
  { ".end",     popend,   0, 0 },
  { ".error",   poperror, 0, 0 },
  { ".half",    popdata,  4, 0 },
  { ".opdef",   popopdef, 0, 0 },
  { ".pass2",   poppass2, 0, 0 },
  { ".phase",   popphase, 1, 0 },
  { ".printx",  popprint, 0, 0 },
  { ".short",   popdata,  2, 0 },
  { ".word",    popdata,  8, 0 },
  /*
   *  Keep the conditional operators separate, for the time beeing.
   */
  { ".if",      popcond,  PD_COND_IF,    PF_COND_NE + PF_LOAD_EXPR },
  { ".if1",     popcond,  PD_COND_IF,    PF_COND_EQ + PF_LOAD_P2 },
  { ".if2",     popcond,  PD_COND_IF,    PF_COND_NE + PF_LOAD_P2 },
  { ".ifdef",   popcond,  PD_COND_IF,    PF_COND_NE + PF_LOAD_SYM },
  { ".ifndef",  popcond,  PD_COND_IF,    PF_COND_EQ + PF_LOAD_SYM },
  { ".ifeq",    popcond,  PD_COND_IF,    PF_COND_EQ + PF_LOAD_EXPR },
  { ".ifne",    popcond,  PD_COND_IF,    PF_COND_NE + PF_LOAD_EXPR },
  { ".ifge",    popcond,  PD_COND_IF,    PF_COND_GE + PF_LOAD_EXPR },
  { ".ifgt",    popcond,  PD_COND_IF,    PF_COND_GT + PF_LOAD_EXPR },
  { ".ifle",    popcond,  PD_COND_IF,    PF_COND_LE + PF_LOAD_EXPR },
  { ".iflt",    popcond,  PD_COND_IF,    PF_COND_LT + PF_LOAD_EXPR },
  { ".elsif",   popcond,  PD_COND_ELSIF, PF_COND_NE + PF_LOAD_EXPR },
  { ".else",    popcond,  PD_COND_ELSE,  PF_COND_NE + PF_LOAD_ONE },
  { ".endif",   popcond,  PD_COND_ENDIF, PF_COND },
  { NULL, NULL, 0, 0 },
};

struct opcode opctab[] = {
  { "arithi",   0x08,    0,   0 },
  { "floati",   0x0A,    0,   0 },
  { "jsp",      0x10,    0,   0 },
  { "jsr",      0x11,    0,   0 },
  { "jsys",     0x12,    0,   0 },
  { "jrst",     0x13,    0,   0 },
  { "portal",   0x13,    1,   0 },
  { "hrri",     0x18,    0,   0 },
  { "hrr",      0x19,    0,   0 },
  { "hrli",     0x1A,    0,   0 },
  { "hrl",      0x1B,    0,   0 },
  { "hlri",     0x1C,    0,   0 },
  { "hlr",      0x1D,    0,   0 },
  { "hlli",     0x1E,    0,   0 },
  { "hll",      0x1F,    0,   0 },
  { "movei",    0x20,    0,   0 },
  { "movea",    0x21,    0,   0 },
  { "movni",    0x22,    0,   0 },
  { "dmove",    0x23,    0,   0 },
  { "move",     0x24,    0,   0 },
  { "move32",   0x25,    0,   0 },
  { "move16",   0x26,    0,   0 },
  { "move8",    0x27,    0,   0 },
  { "setzm",    0x28,    0,   0 },
  { "setzm",    0x29,    0,   0 },
  { "exch",     0x2A,    0,   0 },
  { "dmovem",   0x2B,    0,   0 },
  { "movem",    0x2C,    0,   0 },
  { "mov32m",   0x2D,    0,   0 },
  { "mov16m",   0x2E,    0,   0 },
  { "mov8m",    0x2F,    0,   0 },
  { "pushi",    0x30,    0,   0 },
  { "pusha",    0x31,    0,   0 },
  { "pushm",    0x32,    0,   0 },
  { "pushj",    0x33,    0,   0 },
  { "push",     0x34,    0,   0 },
  { "push32",   0x35,    0,   0 },
  { "push16",   0x36,    0,   0 },
  { "push8",    0x37,    0,   0 },
  { "popm",     0x3A,    0,   0 },
  { "popj",     0x3B,    0,   0 },
  { "pop",      0x3C,    0,   0 },
  { "pop32",    0x3D,    0,   0 },
  { "pop16",    0x3E,    0,   0 },
  { "pop8",     0x3F,    0,   0 },
  { "cai",      0x40,    0,   0 },
  { "cail",     0x41,    0,   0 },
  { "caie",     0x42,    0,   0 },
  { "caile",    0x43,    0,   0 },
  { "caia",     0x44,    0,   0 },
  { "caige",    0x45,    0,   0 },
  { "cain",     0x46,    0,   0 },
  { "caig",     0x47,    0,   0 },
  { "cam",      0x48,    0,   0 },
  { "caml",     0x49,    0,   0 },
  { "came",     0x4A,    0,   0 },
  { "camle",    0x4B,    0,   0 },
  { "cama",     0x4C,    0,   0 },
  { "camge",    0x4D,    0,   0 },
  { "camn",     0x4E,    0,   0 },
  { "camg",     0x4F,    0,   0 },
  { "jump",     0x50,    0,   0 },
  { "jumpl",    0x51,    0,   0 },
  { "jumpe",    0x52,    0,   0 },
  { "jumple",   0x53,    0,   0 },
  { "jumpa",    0x54,    0,   0 },
  { "jumpge",   0x55,    0,   0 },
  { "jumpn",    0x56,    0,   0 },
  { "jumpg",    0x57,    0,   0 },
  { "skip",     0x58,    0,   0 },
  { "skipl",    0x59,    0,   0 },
  { "skipe",    0x5A,    0,   0 },
  { "skiple",   0x5B,    0,   0 },
  { "skipa",    0x5C,    0,   0 },
  { "skipge",   0x5D,    0,   0 },
  { "skipn",    0x5E,    0,   0 },
  { "skipg",    0x5F,    0,   0 },
  { "aoj",      0x60,    0,   0 },
  { "aojl",     0x61,    0,   0 },
  { "aoje",     0x62,    0,   0 },
  { "aojle",    0x63,    0,   0 },
  { "aoja",     0x64,    0,   0 },
  { "aojge",    0x65,    0,   0 },
  { "aojn",     0x66,    0,   0 },
  { "aojg",     0x67,    0,   0 },
  { "soj",      0x68,    0,   0 },
  { "sojl",     0x69,    0,   0 },
  { "soje",     0x6A,    0,   0 },
  { "sojle",    0x6B,    0,   0 },
  { "soja",     0x6C,    0,   0 },
  { "sojge",    0x6D,    0,   0 },
  { "sojn",     0x6E,    0,   0 },
  { "sojg",     0x6F,    0,   0 },
  { "aos",      0x70,    0,   0 },
  { "aosl",     0x71,    0,   0 },
  { "aose",     0x72,    0,   0 },
  { "aosle",    0x73,    0,   0 },
  { "aosa",     0x74,    0,   0 },
  { "aosge",    0x75,    0,   0 },
  { "aosn",     0x76,    0,   0 },
  { "aosg",     0x77,    0,   0 },
  { "sos",      0x78,    0,   0 },
  { "sosl",     0x79,    0,   0 },
  { "sose",     0x7A,    0,   0 },
  { "sosle",    0x7B,    0,   0 },
  { "sosa",     0x7C,    0,   0 },
  { "sosge",    0x7D,    0,   0 },
  { "sosn",     0x7E,    0,   0 },
  { "sosg",     0x7F,    0,   0 },
  { "addi",     0x80,    0,   0 },
  { "add",      0x81,    0,   0 },
  { "subi",     0x82,    0,   0 },
  { "sub",      0x83,    0,   0 },
  { "addci",    0x84,    0,   0 },
  { "addc",     0x85,    0,   0 },
  { "subci",    0x86,    0,   0 },
  { "subc",     0x87,    0,   0 },
  { "xori",     0x88,    0,   0 },
  { "xor",      0x89,    0,   0 },
  { "andi",     0x8A,    0,   0 },
  { "and",      0x8B,    0,   0 },
  { "iori",     0x8C,    0,   0 },
  { "ior",      0x8D,    0,   0 },
  { "eqvi",     0x8E,    0,   0 },
  { "eqv",      0x8F,    0,   0 },
  { "imuli",    0x90,    0,   0 },
  { "imul",     0x91,    0,   0 },
  { "muli",     0x92,    0,   0 },
  { "mul",      0x93,    0,   0 },
  { "uimuli",   0x94,    0,   0 },
  { "uimul",    0x95,    0,   0 },
  { "umuli",    0x96,    0,   0 },
  { "umul",     0x97,    0,   0 },
  { "idivi",    0x98,    0,   0 },
  { "idiv",     0x99,    0,   0 },
  { "divi",     0x9A,    0,   0 },
  { "div",      0x9B,    0,   0 },
  { "uidivi",   0x9C,    0,   0 },
  { "uidiv",    0x9D,    0,   0 },
  { "udivi",    0x9E,    0,   0 },
  { "udiv",     0x9F,    0,   0 },
  { "maddi",    0xA0,    0,   0 },
  { "madd",     0xA1,    0,   0 },
  { "msubi",    0xA2,    0,   0 },
  { "msub",     0xA3,    0,   0 },
  { "umaddi",   0xA4,    0,   0 },
  { "umadd",    0xA5,    0,   0 },
  { "umsubi",   0xA6,    0,   0 },
  { "umsub",    0xA7,    0,   0 },
  { "lsh",      0xA8,    0,   0 },
  { "lshc",     0xA9,    0,   0 },
  { "ash",      0xAA,    0,   0 },
  { "ashc",     0xAB,    0,   0 },
  { "rot",      0xAC,    0,   0 },
  { "rotc",     0xAD,    0,   0 },
  { "faddi",    0xB0,    0,   0 },
  { "fadd",     0xB1,    0,   0 },
  { "fsubi",    0xB2,    0,   0 },
  { "fsub",     0xB3,    0,   0 },
  { "fmuli",    0xB4,    0,   0 },
  { "fmul",     0xB5,    0,   0 },
  { "fdivi",    0xB6,    0,   0 },
  { "fdiv",     0xB7,    0,   0 },
  { "fmaddi",   0xB8,    0,   0 },
  { "fmadd",    0xB9,    0,   0 },
  { "fmsubi",   0xBA,    0,   0 },
  { "fmsub",    0xBB,    0,   0 },
  { "trn",      0xC0,    0,   0 },
  { "trne",     0xC1,    0,   0 },
  { "trna",     0xC2,    0,   0 },
  { "trnn",     0xC3,    0,   0 },
  { "trz",      0xC4,    0,   0 },
  { "trze",     0xC5,    0,   0 },
  { "trza",     0xC6,    0,   0 },
  { "trzn",     0xC7,    0,   0 },
  { "trc",      0xC8,    0,   0 },
  { "trce",     0xC9,    0,   0 },
  { "trca",     0xCA,    0,   0 },
  { "trcn",     0xCB,    0,   0 },
  { "tro",      0xCC,    0,   0 },
  { "troe",     0xCD,    0,   0 },
  { "troa",     0xCE,    0,   0 },
  { "tron",     0xCF,    0,   0 },
  { "tln",      0xD0,    0,   0 },
  { "tlne",     0xD1,    0,   0 },
  { "tlna",     0xD2,    0,   0 },
  { "tlnn",     0xD3,    0,   0 },
  { "tlz",      0xD4,    0,   0 },
  { "tlze",     0xD5,    0,   0 },
  { "tlza",     0xD6,    0,   0 },
  { "tlzn",     0xD7,    0,   0 },
  { "tlc",      0xD8,    0,   0 },
  { "tlce",     0xD9,    0,   0 },
  { "tlca",     0xDA,    0,   0 },
  { "tlcn",     0xDB,    0,   0 },
  { "tlo",      0xDC,    0,   0 },
  { "tloe",     0xDD,    0,   0 },
  { "tloa",     0xDE,    0,   0 },
  { "tlon",     0xDF,    0,   0 },
  {  NULL,         0,    0,   0 },
};

/*
 *  Symbol table routines:
 */

symbol* sym_lookup(char* name)
{
  char tname[8];		/* Our fixed-lenght name. */
  symbol* sym;

  strncpy(tname, name, 8);

  for (sym = symtab; sym != NULL; sym = sym->sym_next) {
    if (memcmp(sym->sym_name, tname, 8) == 0)
      return sym;
  }

  if (!caactive)		/* If inside non-active conditional, - */
    return NULL;		/*  return NULL to next level. */

  sym = malloc(sizeof(*sym));
  memset(sym, 0, sizeof(*sym));
  memcpy(sym->sym_name, tname, 8);
  sym->sym_type = ST_FUTURE;
  sym->sym_prev = NULL;
  sym->sym_next = symtab;
  symtab = sym;

  return sym;
}

symbol* sym_build(char* name, uint8_t type, hexaword value, void* data)
{
  symbol* sym;

  sym = sym_lookup(name);
  if (sym != NULL) {
    sym->sym_type = type;
    sym->sym_value = value;
    sym->sym_data = data;
    sym->sym_flags = SF_BUILTIN;
  }
  return sym;
}

void sym_init(void)
{
  int i;

  hexaword w, a;

  for (i = 0; poptab[i].pop_name != NULL; i++) {
    sym_build(poptab[i].pop_name, ST_PSEUDO, 0, &poptab[i]);
  }

  for (i = 0; opctab[i].op_name != NULL; i++) {
    w = opctab[i].op_code;
    w <<= 56;
    a = opctab[i].op_accode;
    a <<= 52;
    w += a;
    w += opctab[i].op_eacode;

    sym_build(opctab[i].op_name, ST_OPCODE, w, NULL);
  }
}

char* sym_name(symbol* sym)
{
  static char pname[9];

  memcpy(pname, sym->sym_name, 8);
  pname[8] = 0;
  return pname;
}

void sym_dump(void)
{
  symbol* sym;
  char pname[9];
  char tc;

  if (symtab != NULL) {
    printf("\nSymbol table:\n\n");
    /*     "  ssssssss  vvvvvvvvvvvvvvvv   t   ffffffff */
    printf("   symbol            value    type   flags\n\n");
  }

  for (sym = symtab; sym != NULL; sym = sym->sym_next) {
    if (sym->sym_flags & SF_BUILTIN)
      continue;
    switch (sym->sym_type) {
      case ST_FUTURE: tc = 'F'; break;
      case ST_PSEUDO: tc = 'P'; break;
      case ST_OPCODE: tc = 'O'; break;
      case ST_LABEL:  tc = 'L'; break;
      case ST_VALUE:  tc = 'S'; break;
      default:        tc = '?'; break;
    }
    memcpy(pname, sym->sym_name, 8);
    pname[8] = 0;
    printf("  %-8s  %16" PRIx64 "   %c  %8x\n",
	   pname, sym->sym_value, tc, sym->sym_flags);
  }
}

/*
 *  Emit data.
 */

void emit(uint8_t b)
{
  if (objfile != NULL)
    fputc(b, objfile);
}

void emitword(hexaword w)
{
  if (pass2) {
    emit((w >> 56) & 0xff);
    emit((w >> 48) & 0xff);
    emit((w >> 40) & 0xff);
    emit((w >> 32) & 0xff);
    emit((w >> 24) & 0xff);
    emit((w >> 16) & 0xff);
    emit((w >> 8) & 0xff);
    emit((w >> 0) & 0xff);
  }
  loc += 8;
}

void emithalf(hexaword w)
{
  if (pass2) {
    emit((w >> 24) & 0xff);
    emit((w >> 16) & 0xff);
    emit((w >> 8) & 0xff);
    emit((w >> 0) & 0xff);
  }
  loc += 4;
}

void emitshort(hexaword w)
{
  if (pass2) {
    emit((w >> 8) & 0xff);
    emit((w >> 0) & 0xff);
  }
  loc += 2;
}

void emitbyte(hexaword w)
{
  if (pass2) {
    emit(w & 0xff);
  }
  loc += 1;
}

/**********************************************************************/

/*
 *  Print the void* argument as a character, handing non-printables
 *  and doing the ugly conversion needed to make gcc shut TF up.
 */

void errchar(void* ptr)
{
  char c;

  c = (char) ((char*) ptr - (char*) 0);
  if (isprint(c)) {
    printf("'%c'", c);
  } else {
    printf("0x%x", c);
  }
}

/*
 *  Print out an error code as an error message.
 */

void errmessage(int code, void* extra)
{
  /*
   *  First, we can be in either verbose error mode, terse mode, or
   *  quiet mode.  If verbose, we should give a detailed message,
   *  including the source line, with markup on the token where the
   *  error occured.  If terse, just give a single line with the
   *  line number and the error message.  If quiet mode we should not
   *  even have been called.
   */

  if (code == E_NOERROR)	/* This should not happen.*/
    return;

  printf("Line %d: ", lineno);

  switch (code) {
  case E_ILLCHAR:
    printf("Illegal character (");
    errchar(extra);
    printf(").");
    break;
  case E_ILLARG:
    printf("Illegal argument.");
    break;
  case E_ILLNUMBER:
    printf("Illegal character (");
    errchar(extra);
    printf(") in number.");
    break;
  case E_NODIGITS:
    printf("No digits given in number.");
    break;
  case E_UNDEF:
    printf("Undefined symbol (%s).", sym_name(extra));
    break;
  case E_ESTRING:
    printf("%s", (char*) extra);
    break;
  case E_NOTEXTD:
    printf("No text delimiter.");
    break;
  case E_EXPNUMBER:
    printf("Number expected.");
    break;
  case E_EXPRPAREN:
    printf("Right paren expected.");
    break;
  case E_EXPRANGEL:
    printf("Right angel bracket expected.");
    break;
  case E_EXPLBRACK:
    printf("Left square bracket expected.");
    break;
  case E_EXPRBRACK:
    printf("Right square bracket expected.");
    break;
  case E_EXPRCURLY:
    printf("Right curly bracket expected.");
    break;
  case E_EXPSCALEF:
    printf("Scale factor expected.");
    break;
  case E_EXPRESSION:
    printf("Expression expected.");
    break;
  case E_EXPSYMBOL:
    printf("Symbol expected.");
    break;
  case E_EXPEOL:
    printf("End of line or comment expected.");
    break;
  default:
    printf("Error message missing, sort of.");
    /*
     *  This is an internal error, we should complain.
     */
    break;
  }
  printf("\n");
}

/*
 *  Handle errors.  We should remember them, together with information
 *  on where on the current line they occured, and print them out
 *  (with that line) after the whole line is processed.
 */

void error2(int code, void* extra)
{
  if (debugflag || pass2) {
    lineno += 1;
    errmessage(code, extra);
    lineno -= 1;
    errcount += 1;
  }
}

void error(int code)
{
  error2(code, NULL);
}

void errorchar(int code, char c)
{
  error2(code, ((void*) (((char*) 0) + c)));
}

void errorstr(char* str)
{
  error2(E_ESTRING, str);
}

/*
 *  If there were any errors on the current line, dump them out
 *  together with the last line and information on where on the
 *  line the error(s) occured.
 */

void errdump(void)
{
  /* The message should look somewhat like:

  line 13:
          movei 1,<42 + kaka>
                        ^---
  Error - undefined symbol

  */
}

/**********************************************************************/

/*
 *  Init our global variables:
 */

void f_init(void)
{
  pushback = 0;
  tkagain = 0;
  eolnflag = 0;
  eofflag = 0;

  lineno = 0;
}

/*
 *  Get next character.
 */

char f_rchar(void)
{
  int c;

  if (pushback) {
    pushback = 0;
    return lastchar;
  }

  if (eolnflag || eofflag)
    return '\n';

  c = fgetc(srcfile);

  if (c == EOF) {
    eofflag = 1;
    c = '\n';
  }

  if (c == '\n')
    eolnflag = 1;

  lastchar = c;

  return c;
}

/*
 *  Unread the last character.
 */

void f_pushback(void)
{
  pushback = 1;
}

/*
 *  Peek at the next char.
 */

char f_peek(void)
{
  char c = f_rchar();

  f_pushback();
  return c;
}

/*
 *  Skip spaces in source.
 */

void f_skipblanks(void)
{
  char c = ' ';

  while ((c == ' ') || (c == '\t'))
    c = f_rchar();

  f_pushback();
}

/*
 *  Skip the rest of this line (if any).
 */

void f_skipline(void)
{
  char c;
  
  c = f_rchar();
  while (c != '\n') {
    c = f_rchar();
  }
}

/*
 *  Skip the rest of this line (if any), and move to the next one.
 */

void f_nextline(void)
{
  f_skipline();

  lineno += 1;			/* Finally, count this line. */

  if (pass2)
    errdump();			/* Any errors, dump them. */

  eolnflag = 0;
  (void) f_peek();		/* Trigger eof flag. */
}

/*
 *  Scan a number.
 */

int radixchar(char c, int radix)
{
  int i;

  if (isdigit(c))
    i = c - '0';
  else if (islower(c))
    i = c + 10 - 'a';
  else if (isupper(c))
    i = c + 10 - 'A';
  else
    return -1;

  if (i >= radix)
    return -1;

  return i;
}

hexaword f_number(char c)
{
  hexaword w;
  int radix = 10;
  int digits = 0;
  int fail = 0;
  int i;

  if (c == '0') {
    radix = 8;
    digits = 1;
    c = f_rchar();
    switch (c) {
    case 'b':
      radix = 2;
      digits = 0;
      c = f_rchar();
      break;
    case 'x':
      radix = 16;
      digits = 0;
      c = f_rchar();
      break;
    }
  }

  w = 0ull;

  while (isalnum(c)) {
    i = radixchar(c, radix);
    digits += 1;
    if (i < 0)
      fail = c;
    else {
      w *= radix;
      w += i;
    }
    c = f_rchar();
  }
  f_pushback();

  if (fail != 0)
    errorchar(E_ILLNUMBER, fail);

  if (digits == 0)
    error(E_NODIGITS);

  return w;
}

/*
 *  We have a double quote, parse something like "abc" into a word.
 */

hexaword f_qtxt(void)
{
  hexaword w;
  char c;
  int count;

  w = 0ull;
  count = 0;

  for (;;) {
    c = f_rchar();
    if (c == '\n') {
      errorstr("End of line in text constant");
      return w;			/* No need for more errors here... */
    }
    if (c == '"') {
      c = f_rchar();
      if (c != '"')
	goto done;
    }
    if (count++ < 8) {
      w <<= 8;
      w |= c;
    }
  }

 done:
  f_pushback();

  if (count == 0)
    errorstr("Empty text constant");
  if (count > 8)
    errorstr("Too long text constant");

  return w;
}

/*
 *  Scan a symbol (name), make sure it is in the symbol table,
 *  and return type (if we know it) or TK_WORD if it is the first
 *  time we see it.
 */

int f_symbol(char c)
{
  char tname[9];
  int i;

  tname[0] = c;
  i = 1;

  c = f_rchar();

  while (isalnum(c)) {
    if (i < 8)
      tname[i++] = c;
    c = f_rchar();
  }

  /* dont push back the next char yet... */

  tname[i] = 0;

  lastsym = sym_lookup(tname);

  if (lastsym == NULL) {
    lasttoken = TK_WORD;
    goto done;
  }

  if (c == ':') {
    if (pass2) {
      /*
       *  Should check that the label still has the same value...
       */
    }
    lastsym->sym_value = loc;
    lastsym->sym_flags |= (pass2? SF_DEF2 : SF_DEF);
    lastsym->sym_type = ST_LABEL;
    lasttoken = TK_LABEL;
    goto done;
  }

  f_pushback();

  switch (lastsym->sym_type) {
  case ST_PSEUDO:
    lasttoken = TK_PSEUDO;
    break;
  case ST_OPCODE:
    lasttoken = TK_OPCODE;
    break;
  case ST_LABEL:
    lasttoken = TK_LABEL;
    break;
  case ST_VALUE:
    lasttoken = TK_SYMBOL;
    break;
  default:
    lasttoken = TK_WORD;
    break;
  }

 done:
  return lasttoken;
}

/*
 *  Token scanning.  Returns next lexical element type.
 */

int f_token(void)
{
  char c;

  if (tkagain) {
    tkagain = 0;
    return lasttoken;
  }

  f_skipblanks();
  c = f_rchar();
  
  switch (c) {
  case '!': lasttoken = TK_EXCLAM; goto done;
  case '"': lasttoken = TK_DQUOTE; goto done;
  case '#': lasttoken = TK_HASH; goto done;
    /* "$" */
    /* "%" */
  case '&': lasttoken = TK_AMP; goto done;
  case '\'':lasttoken = TK_SQUOTE; goto done;
  case '(': lasttoken = TK_LPAREN; goto done;
  case ')': lasttoken = TK_RPAREN; goto done;
  case '*': lasttoken = TK_STAR; goto done;
  case '+': lasttoken = TK_PLUS; goto done;
    /* "," */
  case '-': lasttoken = TK_MINUS; goto done;
    /* "." */
  case '/': lasttoken = TK_SLASH; goto done;
  case ':': lasttoken = TK_COLON; goto done;
  case ';': lasttoken = TK_SEMI; goto done;
  case '<': lasttoken = TK_LANGEL; goto done;
  case '=': lasttoken = TK_EQUAL; goto done;
  case '>': lasttoken = TK_RANGEL; goto done;
  case '?': lasttoken = TK_QMARK; goto done;
  case '@': lasttoken = TK_ATSIGN; goto done;
  case '[': lasttoken = TK_LBRACK; goto done;
  case '\\':lasttoken = TK_BSLASH; goto done;
  case ']': lasttoken = TK_RBRACK; goto done;
    /* "^" */
    /* "_" */
  case '{': lasttoken = TK_LCURL; goto done;
  case '|': lasttoken = TK_VBAR; goto done;
  case '}': lasttoken = TK_RCURL; goto done;
  case '~': lasttoken = TK_TILDE; goto done;

  case '\n': lasttoken = TK_EOL; goto done;

  case ',':			/* Can be "," or ",," */
    c = f_rchar();
    if (c == ',') {
      lasttoken = TK_2COMMA;
    } else {
      f_pushback();
      lasttoken = TK_COMMA;
    }
    goto done;
  case '.':			/* Can be "." or pseudo-op. */
    c = f_peek();
    if (isalpha(c))
      return f_symbol('.');
    lasttoken = TK_DOT;
    goto done;
  default:			/* Unknown so far. */
    break;
  }

  if (isalpha(c) || (c == '.')) {
    return f_symbol(c);
  }

  if (isdigit(c)) {
    lastnum = f_number(c);
    lasttoken = TK_NUMBER;
    goto done;
  }

  lasttoken = TK_CHAR;

 done:
  return lasttoken;
}

/*
 *  Cause this token to be returned again.
 */

void f_untoken(void)
{
  tkagain = 1;
}

/*
 *  Return non-zero if the given token is one that can start an expression.
 */

int expstart(int token)
{
  switch (token) {
  case TK_NUMBER:
  case TK_SYMBOL:
  case TK_LABEL:
  case TK_WORD:
  case TK_OPCODE:
  case TK_DQUOTE:
  case TK_LPAREN:
  case TK_PLUS:
  case TK_MINUS:
  case TK_DOT:
  case TK_LANGEL:
  case TK_TILDE:
    return 1;
  }

  return 0;
}

/**********************************************************************/

/*
 *  Parse an expression.  When we come here the first token is already
 *  scanned in.
 */

hexaword f_expr(void);
hexaword f_opcode(void);

hexaword f_factor(void)
{
  hexaword w = 0ull;

  switch (lasttoken) {
  case TK_DOT:
    w = loc;
    break;
  case TK_NUMBER:
    w = lastnum;
    break;
  case TK_SYMBOL:
  case TK_LABEL:
    w = lastsym->sym_value;
    if (lastsym->sym_flags & SF_FUSE)
      fuseflag = 1;
    break;
  case TK_WORD:			/* Not-yet-defined symbol. */
    w = 0;
    lastsym->sym_flags |= SF_FUSE;
    fuseflag = 1;		/* Propagate. */
    if (pass2) {
      error2(E_UNDEF, lastsym);
    }
    break;    
  case TK_LANGEL:
    f_token();
    w = f_expr();
    if (lasttoken != TK_RANGEL) {
      error(E_EXPRANGEL);
    }
    break;
  case TK_LPAREN:
    f_token();
    w = f_expr();
    w = ((w & RHMASK) << 32) | ((w & LHMASK) >> 32);
    if (lasttoken != TK_RPAREN) {
      error(E_EXPRPAREN);
    }
    break;
  case TK_DQUOTE:
    w = f_qtxt();
    break;
  default:
    error(E_EXPRESSION);
    break;
  }
  f_token();

  return w;
}

hexaword f_term(void)
{
  hexaword w;

  w = f_factor();

  for (;;) {
    switch (lasttoken) {
    case TK_STAR:
      f_token();
      w *= f_factor();
      break;
    case TK_SLASH:
      f_token();
      w /= f_factor();
      break;
    case TK_AMP:
      f_token();
      w &= f_factor();
      break;
    default:
      goto done;
    }
  }

 done:

  return w;
}

hexaword f_expr(void)
{
  hexaword w;

  switch (lasttoken) {
  case TK_OPCODE:
    return f_opcode();
  case TK_PLUS:
    f_token();
    w = f_term();
    break;
  case TK_MINUS:
    f_token();
    w = - f_term();
    break;
  case TK_TILDE:
    f_token();
    w = ~ f_term();
    break;
  default:
    w = f_term();
    break;
  }

  for (;;) {
    switch (lasttoken) {
    case TK_PLUS:
      f_token();
      w += f_term();
      break;
    case TK_MINUS:
      f_token();
      w -= f_term();
      break;
    case TK_VBAR:
      f_token();
      w |= f_term();
      break;
    default:
      goto done;
    }
  }

 done:
  return w;
}

/*
 *  We got an opcode starting an expression.  This means that we should
 *  parse for the fields following in an instruction.
 */

hexaword f_opcode(void)
{
  hexaword w, item;

  w = lastsym->sym_value;	/* Pick up opcode. */

  (void) f_token();		/* Next token, please. */
  if (lasttoken == TK_LCURL) {	/* Number field coming? */
    f_token();
    item = f_expr();
    if (lasttoken == TK_RCURL) {
      f_token();
    } else {
      error(E_EXPRCURLY);
    }
    /* if item > 0x7ff, error */
    item &= 0x7ff;		/* Mask down to field size. */
    item |= 0x800;		/* Turn on L bit. */
    item <<= 40;		/* Shift into position. */
    w |= item;			/* Include in instruction word. */
  }

  if (lasttoken == TK_ATSIGN) {
    w |= IBIT;
    f_token();
    item = f_expr();
    goto noreg;
  }

  if (lasttoken == TK_LPAREN)
    goto ixreg;

  if (!expstart(lasttoken))
    return w;

  item = f_expr();

  if (lasttoken == TK_COMMA) {
    f_token();			/* Eat it. */
    item &= 15;			/* Mask down to field size. */
    item <<= 52;		/* Shift into position. */
    w |= item;
    if (lasttoken == TK_ATSIGN) {
      w |= IBIT;
      f_token();
    }
    if (lasttoken == TK_LPAREN)
      goto ixreg;
    if (!expstart(lasttoken))
      return w;

    item = f_expr();
  }

 noreg:

  item &= RHMASK;		/* ... */
  w |= item;

 ixreg:

  if (lasttoken == TK_LPAREN) {
    f_token();
    item = f_expr();
    item = ((item & RHMASK) << 32) | ((item & LHMASK) >> 32);
    w |= item;
    if (lasttoken == TK_COMMA) {
      f_token();
      if (lasttoken != TK_NUMBER) {
	error(E_EXPSCALEF);
      } else {
	if (lastnum != 0) {
	  item = 0;
	  while ((lastnum & 1) == 0) {
	    item += 1;
	    lastnum >>= 1;
	  }
	  item <<= 36;
	  w |= item;
	}
      }
      f_token();
    }
    if (lasttoken != TK_RPAREN) {
      error(E_EXPRPAREN);
    }
  }

  return w;
}

/**********************************************************************/

/*
 *  Handlers for various pseudo-ops.
 */

/*
 *  Handle .byte/.short/.half/.word here.
 */

void popdata(pop* p)
{
  hexaword w;

  for (;;) {
    f_token();
    if (!expstart(lasttoken)) {
      error(E_EXPRESSION);
      goto done;
    }
    w = f_expr();
    switch (p->pop_data) {
    case 1:
      emitbyte(w);
      break;
    case 2:
      emitshort(w);
      break;
    case 4:
      emithalf(w);
      break;
    case 8:
      emitword(w);
      break;
    default:
      /*
       *  This is an internal error.
       */
      break;
    }
    switch (lasttoken) {
    case TK_COMMA:		/* Comma -- one more. */
      break;
    case TK_EOL:		/* End-of-line is OK. */
    case TK_SEMI:		/* Comment start is OK. */
      goto done;
    default:
      error(E_EXPEOL);
      goto done;
    }
  }

 done:
  f_skipline();
}

/*
 *  Handle .blkb, .blks, .blkh and .blkw here.
 */

void popspace(pop* p)
{
  hexaword w;

  f_token();
  w = f_expr();

  /*
   *  Should check for future symbols used.
   */

  switch (lasttoken) {
  case TK_EOL:
  case TK_SEMI:
    break;			/* These are OK. */
  default:
    error(E_EXPEOL);
    break;
  }
  f_skipline();

  w *= p->pop_data;

  loc += w;

  if (pass2) {
    while (w-- > 0)
      emit(0);
  }
}

/*
 *  Handle .ascii and .asciz here.
 */

void poptext(pop* p)
{
  char c, delim;

  f_skipblanks();

  delim = f_rchar();
  if (delim == '\n') {
    error(E_NOTEXTD);
    return;
  }

  c = f_rchar();
  while (c != delim) {
    if (c == '\n') {
      if (eofflag) {
	errorstr("End of file inside text constant");
	return;
      }
      f_nextline();
    }
    if (pass2 && caactive)
      emit(c);
    loc += 1;
    c = f_rchar();
  }
  
  if (p->pop_data) {
    if (pass2 && caactive)
      emit(0);
    loc += 1;
  }

  /*
   *  Should verify no more input on this line here.
   */

  f_skipline();
}

/*
 *  Handle .align, allowed arguments right now are 2, 4 and 8.
 */

void popalign(pop* p)
{
  char c;
  hexaword w;
  int amount;

  f_skipblanks();
  
  c = f_rchar();

  if (!isdigit(c)) {
    error(E_EXPNUMBER);
    f_skipline();
    return;
  }

  w = f_number(c);

  amount = 0;

  switch (w) {
  case 2:
    if (loc & 1)
      amount = 1;
    break;
  case 4:
    if (loc & 3)
      amount = 4 - (loc & 3);
    break;
  case 8:
    if (loc & 7)
      amount = 8 - (loc & 7);
    break; 
  default:
    error(E_ILLARG);
    break;
  }

  loc += amount;

  while (amount-- > 0) {
    if (pass2)
      emit(0);
  }

  /*
   *  should be no more non-comment input here.
   */

  f_skipline();
}

/*
 *  Handle .opdef pseudo-op.
 */

void popopdef(pop* p)
{
  symbol* sym;

  f_token();
  switch (lasttoken) {
  case TK_WORD:
  case TK_SYMBOL:
  case TK_OPCODE:
  case TK_LABEL:
    break;
  default:
    error(E_EXPSYMBOL);
    goto bad;
  }

  sym = lastsym;
  f_token();
  if (lasttoken != TK_LBRACK) {
    error(E_EXPLBRACK);
    goto bad;
  }

  f_token();
  
  sym->sym_value = f_expr();
  sym->sym_type = ST_OPCODE;
  sym->sym_flags |= (pass2? SF_DEF2 : SF_DEF);

  if (lasttoken != TK_RBRACK) {
    error(E_EXPRBRACK);
    goto bad;
  }

  f_token();
  switch (lasttoken) {
  case TK_EOL:			/* End-of-line is OK. */
  case TK_SEMI:			/* Comment start is OK. */
    break;
  default:
    error(E_EXPEOL);
    break;
  }

 bad:
  f_skipline();
}

/*
 *  Handle .pass2 pseudo-op.
 */

void poppass2(pop* p)
{
  pass2 = 1;
}

/*
 *  Handle .phase/.dephase pseudo-ops.
 */

void popphase(pop* p)
{
  static int inphase = 0;
  static hexaword oldloc, newloc;

  f_token();			/* Get next token. */

  if (p->pop_data) {
    if (inphase) {
      errorstr(".phase already active");
      goto done;
    }
    inphase = 1;

    if (expstart(lasttoken))
      newloc = f_expr();
    else
      newloc = 0;

    oldloc = loc;
    loc = newloc;

    switch (lasttoken) {
    case TK_EOL:		/* End-of-line is OK. */
    case TK_SEMI:		/* Comment start is OK. */
      break;
    default:
      error(E_EXPEOL);
      break;
    }
  } else {
    if (!inphase) {
      errorstr(".phase not active");
      goto done;
    }
    inphase = 0;

    loc += (oldloc - newloc);

    switch (lasttoken) {
    case TK_EOL:		/* End-of-line is OK. */
    case TK_SEMI:		/* Comment start is OK. */
      break;
    default:
      error(E_EXPEOL);
      break;
    }
  }

 done:
  f_skipline();
}

/*
 *  Handle conditional pseudo-ops, like .if and .elsif and friends.
 */

void save_cond_level(void)
{
  struct cond_save* cs;

  cs = malloc(sizeof(*cs));
  /*
   *  Should check for NULL here.
   */

  calevel += 1;

  cs->cs_next = castack;
  cs->cs_active = caactive;
  cs->cs_virgin = cavirgin;

  castack = cs;

  cavirgin = caactive;
}

void restore_cond_level(void)
{
  struct cond_save* cs;

  cs = castack;

  if (cs == NULL) {
    /*
     *  Very fatal error.
     */
    return;
  }

  calevel -= 1;

  caactive = cs->cs_active;
  cavirgin = cs->cs_virgin;
  castack = cs->cs_next;

  free(cs);
}

void popcond(pop* p)
{
  hexaword cond;

  f_token();
  cond = 0;

  switch (p->pop_flags & PF_LOAD) {
  case PF_LOAD_EXPR:
    if (expstart(lasttoken))
      cond = f_expr();
    else
      errorstr("conditional expression missing");
    break;
  case PF_LOAD_SYM:
    /*
     *  This is missing for the moment.
     */
    errorstr("symbol check code missing");
    f_skipline();
    cond = 0;
    break;
  case PF_LOAD_P2:
    cond = pass2;
    break;
  case PF_LOAD_ONE:
    cond = 1;
    break;
  }

  switch (lasttoken) {
  case TK_EOL:
  case TK_SEMI:
    break;
  default:
    error(E_EXPEOL);
    break;
  }
  
  switch (p->pop_data) {
  case PD_COND_IF:
    save_cond_level();
    /* fallthru */
  case PD_COND_ELSIF:
  case PD_COND_ELSE:
    caactive = 0;
    if (cavirgin) {
      switch (p->pop_flags & PF_COND) {
      case PF_COND_EQ:
	if (cond == 0) caactive = 1;
	break;
      case PF_COND_NE:
	if (cond != 0) caactive = 1;
	break;
      case PF_COND_GT:
	if (cond != 0 && !(cond & SIGNBIT)) caactive = 1;
	break;
      case PF_COND_GE:
	if (!(cond & SIGNBIT)) caactive = 1;
	break;
      case PF_COND_LT:
	if (cond & SIGNBIT) caactive = 1;
	break;
      case PF_COND_LE:
	if (cond == 0 || cond & SIGNBIT) caactive = 1;
	break;
      }
      if (caactive == 1)
	cavirgin = 0;
    }
    break;
  case PD_COND_ENDIF:
    if (calevel > 0)
      restore_cond_level();
    break;
  }
  f_skipline();
}

/*
 *  Handle .end pseudo-op.
 */

void popend(pop* p)
{
  endflag = 1;

  eofflag = 1;			/* Fake out reading the rest. */
  eolnflag = 1;
}

/*
 *  Handle .error pseudo-op.
 */

void poperror(pop* p)
{
# define EBSIZE 80
  /*
   *   XXX  this really really needs to be an allocated buffer.
   */
  static char buf[EBSIZE + 1];
  char c;
  int pos;

  f_skipblanks();
  
  c = f_rchar();
  if (c == '\n') {
    errorstr("user defined error");
    return;
  }

  pos = 0;
  while (pos < EBSIZE && c != '\n') {
    buf[pos++] = c;
    c = f_rchar();
  }
  buf[pos] = 0;
  errorstr(buf);

  f_skipline();
}

/*
 *  Handle .printx pseudo-op.
 */

void popprint(pop* p)
{
  char c;

  f_skipblanks();

  while ((c = f_rchar()) != '\n')
    putchar(c);
  putchar('\n');
}

/**********************************************************************/

/*
 *  We got something that starts an expression.  Parse it and emit a
 *  single 64-bit word.
 */

void doexpr(void)
{
  hexaword w;
  
  w = f_expr();

  emitword(w);
}

/*
 *  We got an opcode, parse an expression (with instruction sematics),
 *  possibly shortening it to 32 bits, and emit the result.
 */

void doopcode(void)
{
  hexaword w, rh;

  fuseflag = 0;

  w = f_opcode();

  if (fuseflag || w & NUMASK) {
    w |= LBIT;
  } else if (!(w & LBIT)) {
    rh = w & 0xfffffc00;
    if (rh != 0 && rh != 0xfffffc00)
      w |= LBIT;
  }

  if (w & LBIT)
    emitword(w);
  else {
    w |= (w << 40) & NUMASK;
    w >>= 32;
    emithalf(w);
  }
}

/*
 *  Here to perform a symbol assignment.
 */

void dosymbol(symbol* sym)
{
  (void) f_token();
  sym->sym_value = f_expr();
  sym->sym_type = ST_VALUE;
  sym->sym_flags |= (pass2? SF_DEF2 : SF_DEF);
  switch (lasttoken) {
  case TK_EOL:			/* End-of-line is OK. */
  case TK_SEMI:			/* Comment start is OK. */
    break;
  default:
    error(E_EXPEOL);
    break;
  }

  f_skipline();
}

/**********************************************************************/

/*
 *  Assemble one source file.  When we come here, the input and output
 *  files should be open, the initial symbol tables should be set up,
 *  and the global variable pass2 should be set if this is the second
 *  pass over the input, the one when we actually generate data.
 */

void assemble(void)
{
  struct pseudo_op* p;

  if (debugflag)
    printf("assemble: pass %d\n", pass2? 2:1);

  while (!eofflag) {
    /*
     *  If we are inside a non-active conditional, do a limited scan.
     */

    if (!caactive) {
      switch (f_token()) {
      case TK_PSEUDO:
	if (debugflag)
	  printf("  inact: %s\n", sym_name(lastsym));
	p = lastsym->sym_data;
	if (p->pop_flags & PF_COND)
	  (*p->pop_handler)(p);
	break;
      default:
	if (debugflag)
	  printf("  inactive: 'other' token %d\n", lasttoken);
	f_nextline();
	break;
      }
      continue;
    }

    switch (f_token()) {
    case TK_WORD:
      /*
       *  Check the next character for ":" or "=", and set type and
       *  value of the symbol accordingly.
       *  If none of these, it is a yet unknown symbol we cannot
       *  handle, and we should complain.
       */
      if (debugflag)
	printf("  word:      %s value %" PRIx64 " type %u flags %u\n",
	       sym_name(lastsym),
	       lastsym->sym_value, lastsym->sym_type, lastsym->sym_flags);
      if (f_rchar() == '=') {
	dosymbol(lastsym);
      } else {
	/* error, since we don't know this symbol here. */
	if (pass2)
	  error2(E_UNDEF, lastsym);
	f_skipline();
      }
      break;
    case TK_NUMBER:
      /* Can start expression. */
      if (debugflag)
	printf("  number:    %" PRIx64 " (%" PRIu64 ")\n", lastnum, lastnum);
      doexpr();
      break;
    case TK_LABEL:
      if (debugflag)
	printf("  label:     %s value %" PRIx64 " type %u flags %u\n",
	       sym_name(lastsym),
	       lastsym->sym_value, lastsym->sym_type, lastsym->sym_flags);
      break;
    case TK_SYMBOL:
      /* Can start expression. */
      if (debugflag)
	printf("  symbol:    %s value %" PRIx64 " type %u flags %u\n",
	       sym_name(lastsym),
	       lastsym->sym_value, lastsym->sym_type, lastsym->sym_flags);
      if (f_rchar() == '=') {
	dosymbol(lastsym);
      } else {
	f_pushback();
	doexpr();
      }
      break;
    case TK_OPCODE:
      /* Starts a word, instruction semantics. */
      if (debugflag)
	printf("  opcode:    %s\n", sym_name(lastsym));
      doopcode();
      /*
       *  We should check for comment or EOL here.
       */
      f_skipline();		/* Should be no more valid stuff. */
      break;
    case TK_PSEUDO:
      if (debugflag)
	printf("  pseudo-op: %s\n", sym_name(lastsym));
      p = lastsym->sym_data;
      (*p->pop_handler)(p);
      break;
    case TK_LPAREN:
      /* Can start expression. */
      if (debugflag)
	printf("  left paren:\n");
      doexpr();
      break;
    case TK_ATSIGN:
      /* Can start expression. */
      if (debugflag)
	printf("  atsign:\n");
      doexpr();
      break;
    case TK_SEMI:
      if (debugflag)
	printf("  comment:\n");
      f_skipline();
      break;
    case TK_EOL:
      if (debugflag)
	printf("  end-of-line:\n");
      f_nextline();
      break;
    case TK_DOT:
      if (debugflag)
	printf("  .\n");
      doexpr();
      break;
    case TK_CHAR:
    default:
      errorchar(E_ILLCHAR, lastchar);
      break;
    }
  }
}

/*
 *  Do one file.  When we come here the output (object) file should
 *  already be open.
 */

void dofile(char* filename)
{
  srcfile = fopen(filename, "r");
  if (srcfile == NULL) {
    perror("dofile");
    return;
  }
  
  errcount = 0;

  loc = 0;
  pass2 = 0;
  endflag = 0;
  f_init();
  assemble();

  if (pass2)			/* Seen .pass2? */
    goto done;

  rewind(srcfile);

  loc = 0;
  pass2 = 1;
  endflag = 0;
  f_init();
  assemble();

 done:

  /*
   *  Should check for .end seen and possibly some other things here.
   */

  fclose(srcfile);
}

/*
 *  Ye olde standard usage() function.
 */

static void usage(void)
{
  printf("Usage string...\n");
}

/*
 *  Main entry point.  Parse options here.
 */

int main(int argc, char* argv[])
{
  int dumpsym = 0;
  int c;
  char* src = NULL;
  char* obj = NULL;

  while ((c = getopt(argc, argv, "DSo:s:")) != -1) {
    switch (c) {
    case 'D':
      debugflag = 1;
      break;
    case 'S':
      dumpsym = 1;
      break;
    case 'o':
      obj = optarg;
      break;
    case 's':
      src = optarg;
      break;
    default:
      usage();
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;

  /*
   *  Use further arguments, one or two, as input/output file names.
   */
  
  sym_init();

  if (obj == NULL)
    obj = "hex.out";

  objfile = fopen(obj, "wb");
  if (objfile == NULL)
    perror("fopen (object file)");

  if (src == NULL) {
    if (argc > 0)
      src = argv[0];
    else {
      printf("no input file given\n");
      return 1;
    }
  }

  dofile(src);

  if (objfile != NULL)
    fclose(objfile);

  /*
   *  If we are in verbose mode or terse mode, print a short (one line)
   *  summary.
   */
  
  if (errcount == 1) {
    printf("One error detected.\n");
  } else {
    switch (errcount) {
    case 0: printf("No"); break;
    case 2: printf("Two"); break;
    case 3: printf("Three"); break;
    case 4: printf("Four"); break;
    case 5: printf("Five"); break;
    case 6: printf("Six"); break;
    case 7: printf("Seven"); break;
    case 8: printf("Eight"); break;
    case 9: printf("Nine"); break;
    default:
      printf("Too many (%d)", errcount);
      break;
    }
    printf(" errors detected.\n");
    if (errcount > 9)
      printf("Please make fewer.\n");
  }

  /*
   *  If asked, dump all user-defined symbols.
   */

  if (dumpsym)
    sym_dump();

  if (errcount > 0)
    return 1;

  return 0;
}
