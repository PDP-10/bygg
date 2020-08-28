/* Common definitions for the grand disassembler */

// #define UNUSED(x) (x) = (x)
#define UNUSED(x)

/*
** Common files to include:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <inttypes.h>

/*
** Common data types used by us:
*/

#ifndef false
  typedef unsigned int bool;
#  define false 0
#  define true 1
#endif

typedef uint_least8_t  byte;
typedef uint_least16_t word;
typedef uint_least32_t longword;
typedef uint_least64_t quadword;

#define PRIdb PRIdLEAST8
#define PRIdw PRIdLEAST16
#define PRIdl PRIdLEAST32
#define PRIdq PRIdLEAST64

#define PRIib PRIiLEAST8
#define PRIiw PRIiLEAST16
#define PRIil PRIiLEAST32
#define PRIiq PRIiLEAST64

#define PRIob PRIoLEAST8
#define PRIow PRIoLEAST16
#define PRIol PRIoLEAST32
#define PRIoq PRIoLEAST64

#define PRIub PRIuLEAST8
#define PRIuw PRIuLEAST16
#define PRIul PRIuLEAST32
#define PRIuq PRIuLEAST64

#define PRIxb PRIxLEAST8
#define PRIxw PRIxLEAST16
#define PRIxl PRIxLEAST32
#define PRIxq PRIxLEAST64

/*

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long longword;

*/


/*
** status codes. these may be saved across runs, in .sav files, therefore
** DONT REORDER THEM, or your old .sav files will be useless!  This may
** not be true, but for the time being...
**
** Right now we save these as symbolic characters, and reordering is not
** a problem.
**
** Chars used: "- . A B C D F I J L M O P Q T W"
*/

typedef byte stcode;		/* Eight bits to hold the info. */

#define st_none     0		/* '-' Unknown status */
#define st_cont     1		/* '.' Continuation of previous byte */
#define st_inst     2		/* 'I' Instruction starts here */
#define st_byte     3		/* 'B' byte (8 bits) variable */
#define st_word     4		/* 'W' word (16 bits) variable */
#define st_long     5		/* 'L' long (32 bits) variable */
#define st_quad     6		/* 'Q' quadword (64 bits) variable */
#define st_octa     7		/* 'O' octaword (128 bits) variable */
#define st_char     8		/* 'C' Single character */
#define st_text     9		/* 'T' Text string */
#define st_ptr     10		/* 'P' Pointer to code or data */
#define st_mask    11		/* 'M' Mask (register mask) */
#define st_float   12		/* 'F' Floating point. */
#define st_double  13		/* 'D' Double floating. */
#define st_asciz   14		/* 'A' ASCIZ string */
#define st_jump    15		/* 'J' "jump", i.e. last delay slot */

/*
** a counter is a large unsigned value. We use the same data type as
** for a longword (32 bits) for this.
*/

typedef longword counter;

/*
** an object index (objindex) is a scalar variable.
*/

typedef longword objindex;

/*
** a regindex is a number identifying a register.
*/

typedef longword regindex;

/*
** a pattern is a ...
*/

typedef struct pattern {
  struct pattern* next;
  stcode status;
  byte radix;
  signed char sign;
  int length;
} pattern;

/*
** a patindex is a number identifying a pattern.
*/

typedef longword patindex;

/*
** a symindex is a number identifying a symbol.
*/

typedef longword symindex;

/*
** a winindex is a number identifying a window.
*/

typedef longword winindex;

/*
** Addresses are represented as pointers to a structure holding all the
** address bits, and possibly more information.  This makes it possible
** to represent a 64-bit address on any system, for instance.  The in-
** side of the address block is not something that most modules have
** to know about.
*/

struct address;

typedef struct address address;

typedef void (addresshandler)(address*);

/*
** The value block is used to store "values" in several formats.
*/

typedef struct value {
  byte type;
  byte padding[3];
  longword idata;
  void* pdata;
} value;

#define vty_none 0
#define vty_addr 1
#define vty_long 2
#define vty_llong 3

/*
** Type of processor-dependent functions:
*/

typedef void     (evf_init)(void);		/* Init module. */
typedef void     (evf_exit)(void);		/* Exit module. */
typedef bool     (evf_help)(int);		/* Give help. */
typedef void     (evf_peek)(stcode);		/* Peek around. */

typedef char*    (evf_a2s )(address*);		/* Address-2-string xlat. */
typedef address* (evf_s2a )(char*);		/* String-2-address xlat. */

typedef char*    (evf_lcan)(char*);		/* Label canonicalisation. */
typedef bool     (evf_lchk)(char*);		/* Label validity check. */
typedef void     (evf_lgen)(address*);		/* Label generator. */

typedef void     (evf_rcre)(regindex);		/* Hook for create register. */
typedef void     (evf_rdel)(regindex);		/* Hook for delete register. */
typedef void     (evf_rset)(regindex);		/* Hook for assign register. */

typedef void     (evf_scre)(symindex);		/* Hook for create symbol. */
typedef void     (evf_sdel)(symindex);		/* Hook for delete symbol. */
typedef void     (evf_sset)(symindex);		/* Hook for assign symbol. */

typedef address* (evf_auto)(void);		/* Return auto points. */
typedef bool     (evf_cchk)(byte);		/* Check for printable char. */

typedef void     (evf_startline)(bool);
typedef void     (evf_restline)(void);
typedef void     (evf_docomment)(void);
typedef void     (evf_dodescr)(void);

typedef void     (evf_doobject)(void);

typedef void     (evf_begin)(void);
typedef void     (evf_org)(address*);
typedef void     (evf_end)(void);

/*
** Types of help that processor dependent help routine can give:
*/

#define hty_general    0	/* General help. */
#define hty_syntax     1	/* Explain syntax diffs. */
#define hty_registers  2	/* Explain register usage. */

/*
** entry vector for machine dependent modules:
*/

struct entryvector {
  char* name;			/* Name of this processor. */
  char* descr;			/* One-line description. */
  evf_init* init;		/* Init routine. */
  evf_help* help;		/* Help text routine. */
};

/*
** Expand flags for peek routines:
**
** The text returned can have the following fields:
**
** addr: data    label:   opcode arg, arg      ;comment
**
*/

#define EX_addr    0x0001	/* Include address. */
#define EX_data    0x0002	/* Include data. */
#define EX_label   0x0004	/* Include label. */
#define EX_code    0x0008	/* Include code. */
#define EX_comm    0x0010	/* Include comments. */

#define EX_UPD     0x0100	/* Update status, generate labels, ... */

#define EX_ASM     (EX_label + EX_code + EX_comm)
#define EX_LIST    (EX_addr + EX_data + EX_ASM)

/*
** Finally, include all other useful files:
*/

#include "addr.h"
#include "buffer.h"
#include "filio.h"
#include "loader.h"
#include "memory.h"
#include "parser.h"
#include "rpn.h"

#include "common.h"
#include "sysdep.h"
#include "xwin.h"
