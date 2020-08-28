/*
** This module implements the main parser and the main command handling.
*/

#include "comnd.h"
#include "disass.h"

/*
** global variables:
*/

char* atombuffer;
pval parseval;
fdb* used;
bool helpflag;

/*
** index of window we are to apply commands to:
*/

static winindex argwinindex;

/**********************************************************************/

/*
** Command handler, top loop, all that junk.
**
** Forward declarations of command handlers:
*/

void cmd_clear(void);
void     clear_arguments(void);
void     clear_comments(void);
void     clear_descriptions(void);
void     clear_expansions(void);
void     clear_flags(void);
void     clear_highlights(void);
void     clear_labels(void);
void     clear_noreturns(void);
void     clear_notes(void);
void     clear_patterns(void);
void     clear_registers(void);
void     clear_return(void);
void     clear_status(void);
void     clear_symbols(void);
void cmd_define(void);
void     def_pattern(void);
void     def_register(void);
void     def_symbol(void);
void cmd_delete(void);
void     del_arguments(void);
void     del_comment(void);
void     del_description(void);
void     del_expansion(void);
void     del_flags(void);
void     del_highlight(void);
void     del_label(void);
void     del_note(void);
void     del_pattern(void);
void     del_register(void);
void     del_symbol(void);
void cmd_disassemble(void);
void cmd_do(void);
void     do_auto(void);
void     do_count(void);
void     do_dots(void);
void     do_nothing(void);
void     do_parseaddr(void);
void     do_purge(void);
void     do_rehash(void);
void     do_test(void);
void cmd_exit(void);
void cmd_follow(void);
void cmd_go(void);
void cmd_help(void);
void     help_address(void);
void     help_ccmd(void);
void     help_description(void);
void     help_expansion(void);
void     help_pattern(void);
void     help_processor(void);
void     help_status(void);
void cmd_jump(void);
void cmd_list(void);
void     list_highlights(void);
void     list_notes(void);
void     list_patterns(void);
void     list_processors(void);
void     list_registers(void);
void     list_symbols(void);
void     list_windows(void);
void cmd_locate(void);
void     loc_next(void);
void         lnxt_reference(void);
void cmd_memory(void);
void     mem_copy(void);
void     mem_exclude(void);
void     mem_include(void);
void     mem_move(void);
void     mem_relocate(void);
void     mem_set(void);
void     mem_shuffle(void);
void     mem_xor(void);
void cmd_next(void);
void cmd_peek(void);
void cmd_read(void);
void cmd_restore(void);
void cmd_rpn(void);
void cmd_save(void);
void cmd_set(void);
void     set_arguments(void);
void     set_case(void);
void     set_charset(void);
void     set_color(void);
void     set_comment(void);
void     set_delimiter(void);
void     set_description(void);
void     set_display(void);
void     set_expansion(void);
void     set_flags(void);
void     set_highlight(void);
void     set_label(void);
void     set_note(void);
void     set_noreturn(void);
void     set_objtype(void);
void     set_pager(void);
void     set_processor(void);
void     set_radix(void);
void     set_register(void);
void     set_status(void);
void     set_syntax(void);
void cmd_show(void);
void     show_arguments(void);
void     show_auto(void);
void     show_character(void);
void     show_comment(void);
void     show_counters(void);
void     show_description(void);
void     show_dots(void);
void     show_expansion(void);
void     show_flags(void);
void     show_label(void);
void     show_memory(void);
void     show_note(void);
void     show_noreturn(void);
void     show_pattern(void);
void     show_processor(void);
void     show_register(void);
void     show_return(void);
void     show_status(void);
void     show_symbol(void);
void cmd_step(void);
void cmd_take(void);
void cmd_unset(void);
void     unset_noreturn(void);
void     unset_register(void);
void     unset_symbol(void);
void cmd_window(void);
void     win_address(void);
void     win_close(void);
void     win_current(void);
void     win_lower(void);
void     win_move(void);
void     win_next(void);
void         wnxt_comment(void);
void         wnxt_description(void);
void         wnxt_reference(void);
void         wnxt_screen(void);
void         wnxt_segment(void);
void         wnxt_status(void);
void         wnxt_unknown(void);
void     win_open(void);
void     win_raise(void);
void     win_segment(void);

static keywrd cmds[] = {
  { "clear",    0,      (keyval) cmd_clear },
  { "clea",     KEY_INV+KEY_NOR, (keyval) cmd_clear },
  { "define",   0,      (keyval) cmd_define },
  { "delete",   0,      (keyval) cmd_delete },
  { "disassemble", 0,   (keyval) cmd_disassemble },
  { "do",       0,      (keyval) cmd_do },
  { "exit",	0,	(keyval) cmd_exit },
  { "follow",   0,      (keyval) cmd_follow },
  { "go",       0,      (keyval) cmd_go },
  { "help",	0,	(keyval) cmd_help },
  { "jump",     0,      (keyval) cmd_jump },
  { "list",     0,      (keyval) cmd_list },
  { "locate",   0,      (keyval) cmd_locate },
  { "load",     0,      (keyval) cmd_read },
  { "memory",   0,      (keyval) cmd_memory },
  { "next",     0,      (keyval) cmd_next },
  { "peek",     0,      (keyval) cmd_peek },
  { "quit",	KEY_INV,(keyval) cmd_exit },
  { "read",     0,      (keyval) cmd_read },
  { "restore",	0,	(keyval) cmd_restore },
  { "rpn",      0,      (keyval) cmd_rpn },
  { "save",     0,      (keyval) cmd_save },
  { "set",      0,      (keyval) cmd_set },
  { "show",     0,      (keyval) cmd_show },
  { "step",     0,      (keyval) cmd_step },
  { "take",	0,	(keyval) cmd_take },
  { "unset",    0,      (keyval) cmd_unset },
  { "window",   0,      (keyval) cmd_window },
};

static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };

/* Keyword table for parsing status codes: */

static keywrd stats[] = {
  { "none",       0,     (keyval) st_none },
  { "byte",       0,     (keyval) st_byte },
  { "character",  0,     (keyval) st_char },
  { "double",     0,     (keyval) st_double },
  { "float",      0,     (keyval) st_float },
  { "instruction", 0,    (keyval) st_inst },
  { "long",       0,     (keyval) st_long },
  { "mask",       0,     (keyval) st_mask },
  { "octaword",   0,     (keyval) st_octa },
  { "pointer",    0,     (keyval) st_ptr },
  { "quadword",   0,     (keyval) st_quad },
  { "text",       0,     (keyval) st_text },
  { "word",       0,     (keyval) st_word },
};

static keytab statustab = { (sizeof(stats)/sizeof(keywrd)), stats };

/*
** Declaration of all the processors we know about:
*/

extern struct entryvector m6800_vector;
extern struct entryvector m6801_vector;
extern struct entryvector m6802_vector;
extern struct entryvector m6803_vector;
extern struct entryvector m6808_vector;
extern struct entryvector m6809_vector;
extern struct entryvector m6811_vector;

extern struct entryvector m68000_vector;
extern struct entryvector m68010_vector;
extern struct entryvector m68020_vector;
extern struct entryvector m68030_vector;
extern struct entryvector m68040_vector;
extern struct entryvector cpu32_vector;

extern struct entryvector z80_vector;
extern struct entryvector i8080_vector;
extern struct entryvector i8085_vector;
extern struct entryvector h64180_vector;

extern struct entryvector i8086_vector;
extern struct entryvector i8088_vector;
extern struct entryvector i80186_vector;
extern struct entryvector i80286_vector;
extern struct entryvector i80386_vector;
extern struct entryvector i80486_vector;
extern struct entryvector i80486p_vector;

extern struct entryvector v25_vector;

extern struct entryvector foo42_vector;

extern struct entryvector pdp10_vector;

extern struct entryvector pdp11_vector;

extern struct entryvector vax_vector;

static keywrd proctab[] = {
  { "64180",   0,       (keyval) &h64180_vector },
  { "6800",    0,       (keyval) &m6800_vector },
  { "6801",    0,       (keyval) &m6801_vector },
  { "6802",    0,       (keyval) &m6802_vector },
  { "6803",    0,       (keyval) &m6803_vector },
  { "6808",    0,       (keyval) &m6808_vector },
  { "6809",    0,       (keyval) &m6809_vector },
  { "6811",    0,       (keyval) &m6811_vector },
  { "68000",   0,       (keyval) &m68000_vector },
  { "68010",   0,       (keyval) &m68010_vector },
  { "68020",   0,       (keyval) &m68020_vector },
  { "68030",   0,       (keyval) &m68030_vector },
  { "68040",   0,       (keyval) &m68040_vector },
  { "8080",    0,       (keyval) &i8080_vector },
  { "8085",    0,       (keyval) &i8085_vector },
  { "8086",    0,       (keyval) &i8086_vector },
  { "8088",    0,       (keyval) &i8088_vector },
  { "80186",   0,       (keyval) &i80186_vector },
  { "80286",   0,       (keyval) &i80286_vector },
  { "80386",   0,       (keyval) &i80386_vector },
  { "80486",   0,       (keyval) &i80486_vector },
  { "80486-p", 0,       (keyval) &i80486p_vector },
  { "286",     KEY_INV, (keyval) &i80286_vector },
  { "386",     KEY_INV, (keyval) &i80386_vector },
  { "486",     KEY_INV, (keyval) &i80486_vector },
  { "486-p",   KEY_INV, (keyval) &i80486p_vector },
  { "cpu32",   0,       (keyval) &cpu32_vector },
  { "foo42",   0,       (keyval) &foo42_vector },
  { "pdp10",   0,       (keyval) &pdp10_vector },
  { "pdp11",   0,       (keyval) &pdp11_vector },
  { "v25",     0,       (keyval) &v25_vector },
  { "vax",     0,       (keyval) &vax_vector },
  { "z80",     0,       (keyval) &z80_vector },
};

/*
** Break table for parsing labels:
*/

static brktab labelbrk = {
  {				/* letters and digits in first pos */
    0xff, 0xff, 0xff, 0xff, 0xf3, 0xfd, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f
  }, {				/* letters and digits here too. */
    0xff, 0xff, 0xff, 0xff, 0xf3, 0xfd, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x00, 0x1f
  }
};

/************************************************************************/

/*
** findproc() is a routine that takes a processor name, and returns
** the entryvector that has a matching name field.  If we can't find
** a matching entryvector, we return nil.  This is used to restore
** the processor type from saved files.
*/

struct entryvector* findproc(char* name)
{
  struct entryvector* p;
  int i;

  if (name != nil) {
    for (i = 0; i < (sizeof(proctab)/sizeof(keywrd)); i += 1) {
      p = (struct entryvector*) proctab[i]._kwval;
      if (strcmp(name, p->name) == 0) {
	return(p);
      }
    }
  }
  return(nil);
}

/************************************************************************/

static void execute(f) int (*f)(); {(*f)();}

static bool done = false;

static char* badlabel = nil;
static char* badpattern = nil;

void commandloop(void)
{
  static fdb topfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Command, ",
			  nil, nil };
  while (!done) {
    cmseter();
    buftop();
    if (cmcsb._cmerr == CMxEOF) {
      break;
    }
    prompt("disass> ");
    cmsetrp();
    if (badlabel != nil) {
      free(badlabel);
      badlabel = nil;
    }
    if (badpattern != nil) {
      free(badpattern);
      badpattern = nil;
    }
    cmcsb._cmflg2 &= ~CM_NIN;	/* We have code that turns this flag on. */
    parse(&topfdb, &parseval, &used);
    helpflag = false;
    execute(parseval._pvfunc);
  }
}

void toploop(void)
{
  atombuffer = cmini();
  m_init();			/* Init memory.c */
  cmxprintf("hello, user!  For help type 'HELP'\n");
  commandloop();
  cmdone();			/* restore world */
}

void bug(char* routine, char* message)
{
  /* printf("Bug detected in routine %s:\n -- %s --\n", routine, message); */
}

static bool help(void)
{
  if (helpflag) {
    confirm();
  }
  return(helpflag);
}

static char* copyatom(char* previous)
{
  if (previous != nil) {
    free(previous);
  }
  return(copystring(atombuffer, nil));
}

bool dispatch(fdb* fdblist)
{
  static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };
  fdb* fdblast;

  fdblast = fdblist;
  while (fdblast->_cmlst != nil) {
    fdblast = fdblast->_cmlst;
  }
  if (helpflag) {
    fdblast->_cmlst = &cfmfdb;
  }
  parse(fdblist, &parseval, &used);
  if (used == &cfmfdb) {
    return(false);
  } else {
    execute(parseval._pvfunc);
    return(true);
  }
}

/**********************************************************************/

/*
** paecheck() will check for errors (unknown labels etc.) in routines
** that parses addresses etc.  The reason for this is that we want to
** wait until after the confirm before we report them, otherwise the
** parser behave very strange in the precence of errors.
*/

void paecheck(void)
{
  if (badpattern) {
    bufstring("%The pattern ");
    bufstring(badpattern);
    bufstring(" does not exist.\n");
    free(badpattern);
    badpattern = nil;
    cmerjnp(0);
  }
  if (badlabel) {
    bufstring("%The label ");
    bufstring(badlabel);
    bufstring(" does not exist.\n");
    free(badlabel);
    badlabel = nil;
    cmerjnp(0);
  }
}

/*
** paconfirm() simply does a confirm followed by a call to paecheck().
*/

void paconfirm(void)
{
  confirm();
  paecheck();
}

/*
** notyeterror() simply tells the user the sad fact that whatever
** he wanted to do can't be done since I have not yet written the
** code to do so.
*/

void notyeterror(void)
{
  bufstring("%This command is not yet implemented.  Sorry.\n");
}

/*
** l2s() returns a string representation of the integer argument.  Used
** for building default strings for parse routines.
*/

char* l2s(longword l)
{
  static char buffer[20];

  sprintf(buffer, "%ld", l);
  return(buffer);
}

/*
** parse_number() parses a number.  The radix will default to decimal,
** unless the number is prefixed by either "0" or "0x"  in which case
** it will be octal or hexadecimal.  (This, by the way, should be a
** function of ccmd...)
*/

int parse_number(char* defaultanswer, char* helpstring)
 {
  static fdb countfdb = { _CMNUM, NUM_US+NUM_BNP, nil, (pdat) 10,
			  nil, nil, nil };
  static fdb hexfdb = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "0x",
			nil, nil, nil };
  static fdb binfdb = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "0b",
			nil, nil, nil };
  int number;
  int pos;

  if (helpstring != nil) {
    countfdb._cmffl |= CM_SDH;
  } else {
    countfdb._cmffl &= ~CM_SDH;
  }
  countfdb._cmhlp = helpstring;
  countfdb._cmdef = defaultanswer;
  countfdb._cmdat = (pdat) 10;

  parse(fdbchn(&hexfdb, &binfdb, &countfdb, nil), &parseval, &used);

  if (used == &hexfdb) {
    countfdb._cmdat = (pdat) 16;
    countfdb._cmdef = nil;
    parse(&countfdb, &parseval, &used);
    return(parseval._pvint);
  }

  if (used == &binfdb) {
    countfdb._cmdat = (pdat) 2;
    countfdb._cmdef = nil;
    parse(&countfdb, &parseval, &used);
    return(parseval._pvint);
  }

  if (atombuffer[0] == '0') {
    pos = 0;
    number = 0;
    while (atombuffer[pos] != (char) 0) {
      number = (number << 3) + (atombuffer[pos] - '0');
      pos += 1;
    }
    parseval._pvint = number;
  }
  return(parseval._pvint);
}

/*
** parse_address() parses a (single) address.
*/

address* parse_address(char* dfault, bool primed)
{
  address* addr;

  static fdb wtopfdb = { _CMTOK, CM_SDH+TOK_WAK, nil, (pdat) "^",
			   nil, nil, nil };
  static fdb addrfdb = { _CMQST, CM_SDH, nil, nil,
			  "Address", nil, nil };
  static fdb addr2fdb = { _CMFLD, CM_SDH, nil, nil,
			   "Name of label", nil, &labelbrk };

  addrfdb._cmdef = dfault;
  addr = w_getaddr(0);
  if (addr != nil) {
    parse(fdbchn(&wtopfdb, &addrfdb, &addr2fdb, nil), &parseval, &used);
  } else {
    parse(fdbchn(&addrfdb, &addr2fdb, nil), &parseval, &used);
  }
  
  if (used == &wtopfdb) {	/* Top of window token? */
    return(addr);		/*  Yes. */
  }
  if (used == &addrfdb) {	/* Quoted string? */
    addr = l_lookup(atombuffer); /* Yes, try labels first. */
    if (addr == nil) {
      addr = a_str2a(atombuffer);
    }
  } else {
    addr = a_str2a(atombuffer);	/* No, try parse address first. */
    if (addr == nil) {
      addr = l_lookup(atombuffer);
    }
  }
  if (addr == nil) {		/* If we failed, - */
    badlabel = copyatom(badlabel); /* Make a mental note. */
  }
  return(addr);			/* Return whatever we found. */
}

/*
** parse_TO() parses the keyword "TO" followed by an address.
** The keyword "TO" should already have been primed into the parser
** by the previous routine.
*/

static fdb* fdb_TO(void)
{
  static keywrd cmds[] = {
    { "TO", 0, (keyval) 0 },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb tofdb = { _CMKEY, CM_SDH, nil, (pdat) &(cmdtab), "TO",
			   nil, nil };

  return(&tofdb);
}

address* parse_TO(void)
{
  return(parse_address(nil, false));
}

/*
** test: parse addresses according to:
**
**   addr      = <single-address>
**   addr-ext  = ":" <number> | "-" addr
**   range     = addr [addr-ext]
**   list      = range [ "," list ]
*/

address* parse_range(char* dfault, fdb* nextfield)
{
  static fdb hyphfdb = { _CMTOK, TOK_WAK, nil, (pdat) "-", nil, nil, nil };
  static fdb colonfdb = { _CMTOK, TOK_WAK, nil, (pdat) ":", nil, nil, nil };
  static fdb commafdb = { _CMTOK, TOK_WAK, nil, (pdat) ",", nil, nil, nil };
  static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };
  
  static address* head = nil;

  address* tail;

  if (nextfield == nil) {
    nextfield = &cfmfdb;
  }
  hyphfdb._cmlst = &colonfdb;
  colonfdb._cmlst = &commafdb;
  commafdb._cmlst = nextfield;

  head = a_copy(parse_address(dfault, false), head);
  tail = head;

  while (true) {
    parse(&hyphfdb, &parseval, &used);
    if (used == &hyphfdb) {
      a_range(tail, a_copy(parse_address(nil, false), nil));
      parse(&commafdb, &parseval, &used);
    } else if (used == &colonfdb) {
      a_count(tail, parse_number(nil, nil));
      parse(&commafdb, &parseval, &used);
    }
    if (used == &commafdb) {
      tail = a_copy(parse_address(nil, false), nil);
      a_cons(head, tail);
    } else {			/* next field found. */
      return(head);
    }
  }
}

/*
** parse_regrange() parses a subrange (of the format @address) and
** returns a pointer to that address.  If no address is given nil
** will be returned.  In both cases a confirm will be parsed.
*/

static address* parse_regrange(void)
{
  static fdb atfdb  = { _CMTOK, TOK_WAK, nil, (pdat) "@", nil, nil, nil };
  static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };
  address* addr;

  cmcsb._cmflg2 |= CM_NIN;
  parse(fdbchn(&atfdb, &cfmfdb, nil), &parseval, &used);
  cmcsb._cmflg2 &= ~CM_NIN;

  if (used == &cfmfdb) {
    return(nil);
  }
  addr = parse_range(nil, nil);
  paecheck();
  return(addr);
}

/*
** parse_objname() parses a generic name for an object (whatever you
** like) and returns a pointer to a copy of that string.  The string
** is to be used as read-only.
*/

char* parse_objname(char* helpstring)
{
  static fdb objfdb = { _CMFLD, FLD_EMPTY+CM_SDH, nil, nil, nil, nil, nil };
  static char* objname = nil;

  objfdb._cmhlp = helpstring;
  parse(&objfdb, &parseval, &used);
  objname = copyatom(objname);
  return(objname);
}

/*
** parse_value() parses a value of the specified type.
*/

value* parse_value(int type, char* defaultanswer, char* helpstring)
{
  switch (type) {
    case vty_long: return(v_l2v(parse_number(defaultanswer, helpstring)));
    case vty_addr: return(v_a2v(parse_address(defaultanswer, false)));
  }
  return(nil);			/* BUG */
}

/*
** parse_text() parses a line of text, up to but not including the end of
** the line.
*/

char* parse_text(char* helpstring)
{
  static fdb qstfdb = { _CMQST, CM_SDH, nil, nil, nil, nil, nil };
  static fdb txtfdb = { _CMTXT, CM_SDH, nil, nil, nil, nil, nil };
  static char* txt = nil;

  txtfdb._cmhlp = helpstring;
  parse(fdbchn(&qstfdb, &txtfdb, nil), &parseval, &used);
  txt = copyatom(txt);
  return(txt);
}

/*
** parse_pattern() parses a pattern (a series of status codes) and
** returns a pointer to a string of pattern blocks.  If skipflag is
** true our caller has already primed the parser with the first
** status code.  If nextfield is nil we will parse a simple confirm.
*/

fdb* fdb_pattern(void)
{
  static fdb statusfdb = { _CMKEY, 0, nil, (pdat) &(statustab), "status, ",
			   nil, nil };
  static fdb tokenfdb = { _CMTOK, CM_SDH, nil, (pdat) "#",
			  "# to insert a named pattern", nil, nil };

  return(fdbchn(&statusfdb, &tokenfdb, nil));
}

pattern* parse_pattern(bool skipfirst, fdb* nextfield)
{
  static fdb colonfdb = { _CMTOK, TOK_WAK, nil, (pdat) ":", nil, nil, nil };
  static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };

  static pattern* first = nil;
  pattern* last;
  pattern* pptr;
  bool done;

  p_free(first);
  first = nil;
  last = nil;

  colonfdb._cmlst = &cfmfdb;
  cfmfdb._cmlst = fdb_pattern();

  if (!skipfirst) {
    parse(fdb_pattern(), &parseval, &used); /* Prime the parser. */
  }

  done = false;

  while (!done) {
    switch (used->_cmfnc) {
    case _CMCFM:
      done = true;
      break;
    case _CMKEY:		/* Keyword is status code. */
      if (first == nil) {
	first = p_new();
	last = first;
      } else {
	last->next = p_new();
	last = last->next;
      }
      last->status = parseval._pvkey;
      last->length = 0;
      parse(&colonfdb, &parseval, &used);
      if (used == &colonfdb) {
	last->length = parse_number(nil, "length in bytes");
	parse(&cfmfdb, &parseval, &used);
      }
      break;
    case _CMTOK:		/* Token, "pattern" */
      pptr = p_copy(p_read(p_index(parse_objname("name of pattern"))), nil);
      if (pptr == nil) {
	badpattern = copyatom(badpattern);
      } else {
	if (first == nil) {
	  first = pptr;
	  last = first;
	} else {
	  last->next = pptr;
	}
	while (last->next != nil) {
	  last = last->next;
	}
      }
      parse(&cfmfdb, &parseval, &used);
      break;
    }
  }
  paecheck();
  return(first);
}

/*
** parse_processor() parses the name of a processor (cpu) and returns
** a pointer to the corresponding entryvector.
*/

struct entryvector* parse_processor(char* defaultanswer)
{
  static keytab cmdtab = { (sizeof(proctab)/sizeof(keywrd)), proctab };
  static fdb procfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Processor, ",
			   nil, nil };

  procfdb._cmdef = defaultanswer;
  parse(&procfdb, &parseval, &used);
  return((struct entryvector*) parseval._pvkey);
}

/*
** redconfirm() parses a confirm, possibly with pipe/file redirection.
*/

fdb* fdb_redirect(void)
{
  static fdb cfmfdb = { _CMCFM, CM_SDH, nil, nil,
			"confirm to use terminal", nil, nil };
  static fdb redfdb = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) ">",
			"\">\" to redirect output to a file", nil, nil };
  static fdb pipfdb = { _CMTOK, TOK_WAK+CM_SDH, nil, (pdat) "|",
			"\"|\" to redirect output to a pipe", nil, nil };

  return(fdbchn(&cfmfdb, &redfdb, &pipfdb, nil));
}

void redconfirm(bool primed)
{
  static fdb filfdb = { _CMFIL, FIL_PO+FIL_NODIR, nil, nil, nil,
			"disass.out", nil };
  static fdb cmdfdb = { _CMTXT, CM_SDH, nil, nil,
			"command(s) to pipe output through", nil, nil };
  char* filename;
  static char* pipecmd = nil;

  if (!primed) {
    parse(fdb_redirect(), &parseval, &used);
  }

  if (used->_cmfnc == _CMTOK) {
    switch (used->_cmdat[0]) {
    case '>':
      parse(&filfdb, &parseval, &used);
      filename = *parseval._pvfil;
      confirm();
      buffile(filename);
      break;
    case '|':
      parse(&cmdfdb, &parseval, &used);
      pipecmd = copyatom(pipecmd);
      confirm();
      bufpipe(pipecmd);
      break;
    }
  }
}

/**********************************************************************/

/* Top level command handlers: */

/**********************************************************************/

void cmd_clear(void)
{
  static keywrd cmds[] = {
    { "arguments",    0, (keyval) clear_arguments },
    { "comments",     0, (keyval) clear_comments },
    { "descriptions", 0, (keyval) clear_descriptions },
    { "expansions",   0, (keyval) clear_expansions },
    { "flags",        0, (keyval) clear_flags },
    { "highlights",   0, (keyval) clear_highlights },
    { "labels",       0, (keyval) clear_labels },
    { "noreturns",    0, (keyval) clear_noreturns },
    { "notes",        0, (keyval) clear_notes },
    { "patterns",     0, (keyval) clear_patterns },
    { "registers",    0, (keyval) clear_registers },
    { "status",       0, (keyval) clear_status },
    { "symbols",      0, (keyval) clear_symbols },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb clearfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Attribute, ",
			   nil, nil };

  clearfdb._cmlst = nil;
  if (!dispatch(&clearfdb)) {
    bufstring("\
\n\
The CLEAR command is used to remove all knowledge of certain things.  For\n\
a list of what can be cleared, try 'CLEAR ?'.\n\
\n\
");
  }
}

/**********************************************************************/

void clear_arguments(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR ARGUMENTS\n\
\n\
This command removes all argument lists from the database.\n\
\n\
");
  } else {
    confirm();
    ia_clear();
  }
}  

/**********************************************************************/

void clear_comments(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR COMMENTS\n\
\n\
This command removes all comments from the database.\n\
\n\
");
  } else {
    confirm();
    c_clear();
  }
}

/**********************************************************************/

void clear_descriptions(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR DESCRIPTIONS\n\
\n\
This command removes all descriptions from the database.\n\
\n\
");
  } else {
    confirm();
    d_clear();
  }
}

/**********************************************************************/

void clear_expansions(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR EXPANSIONS\n\
\n\
This command removes all expansions from the database.\n\
\n\
");
  } else {
    confirm();
    e_clear();
  }
}

/**********************************************************************/

void clear_flags(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR FLAGS\n\
\n\
This command removes all flags from the database.\n\
\n\
");
  } else {
    confirm();
    f_clear();
  }
}

/**********************************************************************/

void clear_highlights(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR HIGHLIGHTS\n\
\n\
This command removes all highlight points from the database.\n\
\n\
");
  } else {
    confirm();
    hl_clear();
  }
}

/**********************************************************************/

void clear_labels(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR LABELS\n\
\n\
This command removes all labels from the database.\n\
\n\
");
  } else {
    confirm();
    l_clear();
  }
}

/**********************************************************************/

void clear_noreturns(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR NORETURNS\n\
\n\
This command removes all noreturn flags from the database.\n\
\n\
");
  } else {
    confirm();
    nrf_clear();
  }
}

/**********************************************************************/

void clear_notes(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR NOTES\n\
\n\
This command removes all notes from the database.\n\
\n\
");
  } else {
    confirm();
    n_clear();
  }
}

/**********************************************************************/

void clear_patterns(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR PATTERNS\n\
\n\
This command removes all patterns from the database.\n\
\n\
");
  } else {
    confirm();
    p_clear();
  }
}

/**********************************************************************/

void clear_registers(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR REGISTERS\n\
\n\
This command removes (undefines) all registers from the database.\n\
\n\
");
  } else {
    confirm();
    r_clear();
  }
}

/**********************************************************************/

void clear_status(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR STATUS\n\
\n\
This command is a simple way to set the status of all memory to unknown.\n\
If you want to do this selectively, use the command 'SET STATUS'.\n\
\n\
");
  } else {
    static address* pos = nil;
    int i;

    noise("of all memory");
    confirm();

    m_clear();

#ifdef NOTDEF

    /* we should call a routine in memory.c to do this. */

    i = getslength(1);
    pos = a_copy(getsfirst(1), pos);
    while (i-- > 0) {
      setstatus(pos, st_none, 1);
      a_inc(pos, 1);
    }
    wc_total();
#endif

  }
}

/**********************************************************************/

void clear_symbols(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR SYMBOLS\n\
\n\
This command removes all symbols from the database.\n\
\n\
");
  } else {
    confirm();
    s_clear();
  }
}

/**********************************************************************/

void cmd_define(void)
{
  static keywrd cmds[] = {
    { "pattern",     0,    (keyval) def_pattern },
    { "register",    0,    (keyval) def_register },
    { "symbol",      0,    (keyval) def_symbol },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb cmdfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Attribute, ",
			   nil, nil };
  cmdfdb._cmlst = nil;
  if (!dispatch(&cmdfdb)) {
    bufstring("\
\n\
The DEFINE command is used to define things.\n\
\n\
");
  }
}

/**********************************************************************/

void def_pattern(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DEFINE PATTERN <name> <pattern>\n\
\n\
This command defines a pattern.\n\
\n\
");
  } else {
    char* patname = nil;
    patindex index;
    pattern* pat;

    noise("name");
    (void) parse_objname("name of pattern");
    patname = copyatom(patname);

    noise("pattern");
    pat = parse_pattern(false, nil);

    index = p_define(patname);
    p_write(index, pat);
  }
}

/**********************************************************************/

void def_register(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DEFINE REGISTER <name> <type>\n\
\n\
This command defines a register, and sets its type.\n\
\n\
");
  } else {
    static keywrd cmds[] = {
      { "longword",    0,    (keyval) vty_long },
      { "address",     0,    (keyval) vty_addr },
    };
    static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
    static fdb typfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Type, ",
			  "longword", nil };
    char* regname;
    regindex index;
    int type;

    noise("name");
    regname = parse_objname("name of register");

    noise("type");

    parse(&typfdb, &parseval, &used);
    type = parseval._pvint;

    confirm();

    index = r_index(regname);
    if (index != 0) {
      bufstring("%That register is already defined, with type = ");
      switch (r_type(index)) {
        case vty_long: bufstring("long"); break;
        case vty_addr: bufstring("addr"); break;
        default: bufnumber(r_type(index)); break;
      }
      bufstring("\n");
    } else {
      (void) r_define(regname, type);
    }
  }
}

/**********************************************************************/

void def_symbol(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DEFINE SYMBOL <name> <value>\n\
    or:  SET SYMBOL <name> <value>\n\
\n\
This command defines a symbol.\n\
\n\
");
  } else {
    char* symname;
    char* symvalue;
    symindex index;

    noise("name");
    symname = parse_objname("name of symbol");

    noise("value");
    symvalue = parse_text("value of symbol");

    confirm();

    index = s_define(symname);
    s_write(index, symvalue);
  }
}

/**********************************************************************/

void cmd_delete(void)
{
  static keywrd cmds[] = {
    { "arguments",   0,    (keyval) del_arguments },
    { "comment",     0,    (keyval) del_comment },
    { "description", 0,    (keyval) del_description },
    { "expansion",   0,    (keyval) del_expansion },
    { "flags",       0,    (keyval) del_flags },
    { "highlight",   0,    (keyval) del_highlight },
    { "label",       0,    (keyval) del_label },
    { "note",        0,    (keyval) del_note },
    { "pattern",     0,    (keyval) del_pattern },
    { "register",    0,    (keyval) del_register },
    { "symbol",      0,    (keyval) del_symbol },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb delfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Attribute, ",
			   nil, nil };

  delfdb._cmlst = nil;
  if (!dispatch(&delfdb)) {
    bufstring("\
\n\
The DELETE command is used to remove things like labels, comments, ...\n\
from the database.\n\
\n\
");
  }
}

/**********************************************************************/

void del_arguments(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE ARGUMENT <address>\n\
\n\
This command deletes the argument list for the routine at the given\n\
address.\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);
    paconfirm();

    ia_delete(addr);
  }
}

/**********************************************************************/

void del_comment(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE COMMENT <address>\n\
\n\
This command deletes the comment(s) from the specified address(es).\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);
    paconfirm();

    c_delete(addr);
  }
}

/**********************************************************************/

void del_description(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE DESCRIPTION <address>\n\
\n\
This command deletes the description(s) from the specified address(es).\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);
    paconfirm();

    d_delete(addr);
  }
}

/**********************************************************************/

void del_expansion(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE EXPANSION <address>\n\
\n\
This command deletes the expansion(s) from the specified address(es).\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);
    paconfirm();

    e_delete(addr);
  }
}

/**********************************************************************/

void del_highlight(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE HIGHLIGHT <number>\n\
\n\
This command deletes the specified highlight point.\n\
\n\
");
  } else {
    objindex index;

    index = parse_number(nil, "highlight number");

    confirm();

    hl_delete(index);
    bufstring("Highlight point number ");
    bufnumber(index);
    bufstring(" is no more.\n");
  }
}

/**********************************************************************/

void del_flags(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE FLAGS <address>\n\
\n\
This command deletes the flags from the specified address(es).\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);
    paconfirm();

    f_delete(addr);
  }
}

/**********************************************************************/

void del_label(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE LABEL <address>\n\
\n\
This command deletes the label(s) from the specified address(es).\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);
    paconfirm();

    l_delete(addr);
  }
}

/**********************************************************************/

void del_note(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE NOTE <number>\n\
\n\
This command deletes the given note.\n\
\n\
");
  } else {
    objindex index;

    index = parse_number(nil, "note number");

    confirm();

    n_delete(index);
    bufstring("Note ");
    bufnumber(index);
    bufstring(" is no more.\n");
  }
}

/**********************************************************************/

void del_pattern(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE PATTERN <name>\n\
\n\
This command deletes the specified pattern.\n\
\n\
");
  } else {
    char* name;
    patindex index;

    noise("name");
    name = parse_objname("name of pattern");
    confirm();
    
    bufstring("Pattern ");
    bufstring(name);
    index = p_index(name);
    if (index == 0) {
      bufstring(" does not exist.\n");
    } else {
      p_delete(index);
      bufstring(" is no more.\n");
    }
  }
}

/**********************************************************************/

void del_register(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE REGISTER <register>\n\
\n\
This command deletes (undefines) a register.\n\
If you want to remove all registers, use the command CLEAR REGISTERS.\n\
\n\
");
  } else {
    static fdb regfdb = { _CMFLD, FLD_EMPTY+CM_SDH, nil, nil, 
			  "name of register", nil, nil };
    static char* regname = nil;
    regindex index;

    parse(&regfdb, &parseval, &used);
    regname = copyatom(regname);
    confirm();

    bufstring("Register ");
    bufstring(regname);
    index = r_index(regname);
    if (index == 0) {
      bufstring(" does not exist.\n");
    } else {
      r_delete(index);
      bufstring(" is no more.\n");
    }
  }
}
      
/**********************************************************************/

void del_symbol(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE SYMBOL <name>\n\
\n\
This command deletes the specified symbol.\n\
\n\
");
  } else {
    char* name;
    symindex index;

    noise("name");
    name = parse_objname("name of symbol");
    confirm();
    
    bufstring("The symbol ");
    bufstring(name);
    index = s_index(name);
    if (index == 0) {
      bufstring(" does not exist.\n");
    } else {
      s_delete(index);
      bufstring(" is no more.\n");
    }
  }
}

/**********************************************************************/

void cmd_disassemble(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DISASSEMBLE [format] [output]\n\
\n\
The format can be either SOURCE or LISTING.  The default is SOURCE.\n\
\n\
This command will generate assembly code matching the binary input\n\
data, according to the selected processor.  The output from this\n\
command is normally sent to your terminal, but you can redirect it\n\
to a file, with the command format \"DISASSEMBLE > filename\", or pipe\n\
it to another program, with the format \"DISASSEMBLE | command\".\n\
\n\
");
  } else {
    static keywrd formats[] = {
      { "listing", 0,   (keyval) EX_LIST },
      { "source",  0,   (keyval) EX_ASM },
    };
    static keytab fmttab = { (sizeof(formats)/sizeof(keywrd)), formats };

    static fdb fmtfdb = { _CMKEY, 0, nil, (pdat) &(fmttab),
			  "output format, ", "source", nil };
    int format;
    static address* pos = nil;
    int segment, maxseg;

    format = EX_ASM;

    fmtfdb._cmlst = fdb_redirect();

    parse(&fmtfdb, &parseval, &used);
    if (used == &fmtfdb) {
      format = parseval._pvint;
      redconfirm(false);
    } else {
      redconfirm(true);
    }

    if (pcheck()) {
      if (format == EX_ASM) {
	spec(nil, SPC_BEGIN);
      }
      maxseg = getscount();
      for (segment = 1; segment <= maxseg; segment += 1) {
	pos = a_copy(getsfirst(segment), pos);
	if (format == EX_ASM) {
	  spec(pos, SPC_ORG);
	}
	while (mapped(pos)) {
	  peek(pos, format, st_none);
	  a_inc(pos, pb_length * pv_bpa);
	  if (bufquit()) break;
	}
      }
      if (format == EX_ASM) {
	spec(nil, SPC_END);
      }
    }
    bufclose();
  }
}

/**********************************************************************/

void cmd_do(void)
{
  static keywrd cmds[] = {
    { "auto",          0, (keyval) do_auto },
    { "count",         0, (keyval) do_count },
    { "dots",          0, (keyval) do_dots },
    { "nothing",       0, (keyval) do_nothing },
    { "parse-addr",    0, (keyval) do_parseaddr },
    { "purge",         0, (keyval) do_purge },
    { "rehash",        0, (keyval) do_rehash },
    { "test",          0, (keyval) do_test },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb dofdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Procedure, ",
			   nil, nil };

  dofdb._cmlst = nil;
  if (!dispatch(&dofdb)) {
    bufstring("\
\n\
The DO command runs various 'jobs'.\n\
\n\
");
  }
}

/**********************************************************************/

void do_auto(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO AUTO\n\
\n\
This command will do as many automagical things as possible.\n\
\n\
");
  } else {
    address* addr;
    int count;

    confirm();

    if (pcheck()) {
      addr = autolist();
      if (addr == nil) {
	bufstring("%There are no auto points.\n");
      } else {
	bufshutup(true);
	count = follow_code(addr);
	bufshutup(false);
	bufstring("Identified ");
	bufnumber(count);
	bufstring(" bytes as code.\n");
      }
    }
  }
}

/**********************************************************************/

void do_count(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO COUNT\n\
\n\
This command makes sure that the count of different objects is correct,\n\
by actually counting them all again.\n\
\n\
");
  } else {
    confirm();
    notyeterror();
  }
}

/**********************************************************************/

void do_dots(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO DOTS\n\
\n\
This is a debugging routine.\n\
\n\
");
  } else {
    confirm();

    ls_dots();
  }
}

/**********************************************************************/

void do_nothing(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO NOTHING\n\
\n\
This command does exactly that.\n\
\n\
");
  } else {
    confirm();
  }
}

/**********************************************************************/

void do_parseaddr(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO PARSE-ADDR <addr-spec>\n\
\n\
This command tests multi-address parsing.\n\
\n\
");
  } else {
    address* addr;

    addr = parse_range("1000", nil);
    bufstring("I got: ");
    a_print(addr, false);
    bufstring(" living at ");
    bufhex((longword) &addr, 1);
    bufnewline();
  }
}

/**********************************************************************/

void do_purge(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO PURGE\n\
\n\
This command initiates several things:\n\
  1)  All memory which is not at the start of an object will\n\
      be deprived of labels.\n\
  2)  A general garbage collection will happen.\n\
\n\
");
  } else {
    confirm();
    m_purge();
  }
}

/**********************************************************************/

void do_rehash(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO REHASH\n\
\n\
This command will rehash all labels.\n\
\n\
");
  } else {
    confirm();
    l_rehash();
  }
}

/**********************************************************************/

void do_test(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DO TEST\n\
\n\
This command is used for internal testing.\n\
\n\
");
  } else {
    confirm();
    a_test();			/* addr.c test function. */
    w_test();			/* xwin.c test function. */
    m_test();			/* memory.c test function. */
  }
}

/**********************************************************************/

void cmd_exit(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  EXIT\n\
    or:  QUIT\n\
\n\
This command exits the program.  It does NOT save your work, however.\n\
\n\
");
  } else {
    noise("this program");
    confirm();
    done = true;
  }
}

/**********************************************************************/

void cmd_follow(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  FOLLOW <address>\n\
\n\
This command follows all unknown code from a specified point,\n\
updating status etc.\n\
\n\
");
  } else {
    static address* pos = nil;
    int count;

    noise("code at");

    pos = a_copy(parse_address(a_a2str(getsfirst(1)), false), pos);

    paconfirm();

    if (pcheck()) {
      bufshutup(true);
      count = follow_code(pos);
      bufshutup(false);
      if (count > 0) {
	wc_total();
      }
      bufstring("Identified ");
      bufnumber(count);
      bufstring(" bytes as code.\n");
    }
  }
}

/**********************************************************************/

void cmd_go(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  GO <address>\n\
\n\
This command does the same thing as FOLLOW, plus it generates a label\n\
at the given address.\n\
\n\
");
  } else {
    static address* pos = nil;
    int count;

    noise("to code at");

    pos = a_copy(parse_address(a_a2str(getsfirst(1)), false), pos);

    paconfirm();

    if (pcheck()) {
      if (!l_exist(pos)) {
	l_generate(pos);
      }
      bufshutup(true);
      count = follow_code(pos);
      bufshutup(false);
      if (count > 0) {
	wc_total();
      }
      bufstring("Identified ");
      bufnumber(count);
      bufstring(" bytes as code.\n");
    }
  }
}

/**********************************************************************/

void cmd_help(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  HELP [argument]\n\
\n\
This command is used to get help.  What did you think?\n\
\n\
");
  } else {
    static keywrd concepts[] = {
      { "address",     0, (keyval) help_address },
      { "ccmd",        0, (keyval) help_ccmd },
      { "description", 0, (keyval) help_description },
      { "expansion",   0, (keyval) help_expansion },
      { "pattern",     0, (keyval) help_pattern },
      { "processor",   0, (keyval) help_processor },
      { "status",      0, (keyval) help_status },
    };
    static keytab contab = { (sizeof(concepts)/sizeof(keywrd)), concepts };

    static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };
    static fdb confdb = { _CMKEY, 0, &cfmfdb, (pdat) &(contab), "Concept, ",
			    nil, nil };
    static fdb helpfdb = { _CMKEY, 0, &confdb, (pdat) &(cmdtab), "Command, ",
			   nil, nil };

    parse(&helpfdb, &parseval, &used);

    if (used == &cfmfdb) {	/* Command = HELP <return> */
      bufstring("first level help text.\n");
    } else {
      helpflag = true;
      execute(parseval._pvfunc);
    }
  }
}

/**********************************************************************/

void help_address(void)
{
  confirm();
  bufstring("Help text, address syndax.\n");
}

/**********************************************************************/

void help_ccmd(void)
{
  confirm();
  bufstring("Help text elaborating on ccmd, and what it is.\n");
}

/**********************************************************************/

void help_description(void)
{
  confirm();
  bufstring("\
\n\
A description is a block comment before a specified address, typically\n\
used to describe what a routine does or how a variable is used.  The text\n\
can be several lines long.\n\
\n\
Example:\n\
\n\
;This is the main interrupt routine.\n\
\n\
inthnd:  save_regs              ;Save regs (sic)\n\
\n\
");
}

/**********************************************************************/

void help_expansion(void)
{
  confirm();
  bufstring("\
\n\
Help text elaborating on expansions, and what they are.\n\
\n\
");
}

/**********************************************************************/

void help_pattern(void)
{
  confirm();
  bufstring("\
\n\
Help text elaborating on patterns.\n\
\n\
");
}

/**********************************************************************/

void help_processor(void)
{
  static keywrd keys[] = {
    { "general",     0, (keyval) hty_general },
    { "registers",   0, (keyval) hty_registers },
    { "syntax",      0, (keyval) hty_syntax },
  };
  static keytab typtab = { (sizeof(keys)/sizeof(keywrd)), keys };
  static fdb typfdb = { _CMKEY, 0, nil, (pdat) &(typtab),
			nil, "general", nil };
  struct entryvector* p;
  int helptype;

  p = parse_processor(nil);

  parse(&typfdb, &parseval, &used);
  helptype = parseval._pvint;

  confirm();

  bufnewline();
  if (!(*p->help)(helptype)) {
    bufstring("%That type of help seems to be missing.\n");
  }
  bufnewline();
}

/**********************************************************************/

void help_status(void)
{
  static fdb statusfdb = { _CMKEY, 0, nil, (pdat) &(statustab), "status, ",
			   nil, nil };
  static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };
  stcode status;
  
  parse(fdbchn(&statusfdb, &cfmfdb, nil), &parseval, &used);
  if (used == &cfmfdb) {
    bufstring("\
\n\
Help text elaborating on the concept of status codes.\n\
\n\
");
  } else {
    status = parseval._pvkey;
    confirm();
    switch (status) {
    case st_none:
      bufstring("\
\n\
NONE is the same thing as unknown, i.e. the status of this byte is not\n\
known at the moment.\n\
\n\
");
      break;
    case st_char:
      bufstring("\
\n\
CHARACTER means eight bits of data, will be output as a character if it\n\
is printable, as a byte otherwise.\n\
\n\
");
      break;
    case st_byte:
      bufstring("\
\n\
BYTE means eight bits of data.\n\
\n\
");
      break;
    case st_double:
      bufstring("\
\n\
DOUBLE means a double floating point variable.\n\
\n\
");
      break;
    case st_float:
      bufstring("\
\n\
FLOAT means a floating point variable.\n\
\n\
");
      break;
    case st_inst:
      bufstring("\
\n\
INSTRUCTION is something we will try to output as code.\n\
\n\
");
      break;
    case st_long:
      bufstring("\
\n\
LONG is a 32 bit piece of data, four consecutive bytes.\n\
\n\
");
      break;
    case st_mask:
      bufstring("\
\n\
MASK is a bit mask, length is processor-dependent.\n\
\n\
");
      break;
    case st_octa:
      bufstring("\
\n\
OCTAWORD is 128 bits of data, 16 consecutive bytes, or eight words.\n\
\n\
");
      break;
    case st_ptr:
      bufstring("\
\n\
POINTER is a pointer to something.  The exact length depends on the\n\
selected processor type, and possibly on other things.\n\
\n\
");
      break;
    case st_quad:
      bufstring("\
\n\
QUADWORD is 64 bits, eight bytes, or four words.\n\
\n\
");
      break;
    case st_text:
      bufstring("\
\n\
TEXT is ASCII data, and can be of any length.\n\
\n\
");
      break;
    case st_word:
      bufstring("\
\n\
WORD is two consecutive bytes.\n\
\n\
");
      break;
    }
  }
}

/**********************************************************************/

void cmd_jump(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  JUMP <address>\n\
\n\
This command will set \".\" to the specified address.\n\
\n\
");
  } else {
    address* addr;

    addr = parse_address(nil, false);
    paconfirm();

    setdot(addr);
  }
}

/**********************************************************************/

void cmd_list(void)
{
  static keywrd cmds[] = {
    { "cpus",          0, (keyval) list_processors },
    { "highlights",    0, (keyval) list_highlights },
    { "notes",         0, (keyval) list_notes },
    { "patterns",      0, (keyval) list_patterns },
    { "processors",    0, (keyval) list_processors },
    { "registers",     0, (keyval) list_registers },
    { "symbols",       0, (keyval) list_symbols },
    { "windows",       0, (keyval) list_windows },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb listfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Item, ",
			   nil, nil };
  noise("known");
  listfdb._cmlst = nil;
  if (!dispatch(&listfdb)) {
    bufstring("\
\n\
The LIST command lists various things.\n\
\n\
");
  }
}

/**********************************************************************/

void list_highlights(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LIST HIGHLIGHTS\n\
\n\
This command will list all highligh addresses.\n\
\n\
");
  } else {
    redconfirm(false);
    ls_highlights();
  }
}

/**********************************************************************/

void list_notes(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LIST NOTES\n\
\n\
This command will list all known notes.\n\
\n\
");
  } else {
    redconfirm(false);
    ls_notes();
  }
}

/**********************************************************************/

void list_patterns(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LIST PATTERNS\n\
\n\
This command will list all defined patterns.\n\
\n\
");
  } else {
    patindex index;

    redconfirm(false);
    ls_patterns();
  }
}

/**********************************************************************/

void list_processors(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LIST PROCESSORS\n\
    or:  LIST CPUS\n\
\n\
This command will give you a list of all procesors this program\n\
knows about.\n\
\n\
");
  } else {
    int i;
    struct entryvector* p;

    redconfirm(false);
    for (i = 0; i < (sizeof(proctab)/sizeof(keywrd)); i += 1) {
      if (proctab[i]._kwflg & KEY_INV) {
	continue;
      }
      p = (struct entryvector*) proctab[i]._kwval;
      bufstring(p->name);
      tabto(8);
      bufstring("--  ");
      bufstring(p->descr);
      bufnewline();
    }
  }
}

/**********************************************************************/

void list_registers(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LIST REGISTERS\n\
\n\
This command will give you a list of all currently defined registers.\n\
\n\
");
  } else {
    regindex index;
    address* subrange;
    int type;

    redconfirm(false);

    index = r_next(0);
    if (index == 0) {
      bufstring("%There are no defined registers.\n");
    }
    while (index != 0) {
      type = r_type(index);

      bufstring("Register ");
      bufnumber(index);
      bufstring(", name=");
      bufstring(r_name(index));
      bufstring(", type=");
      switch (type) {
        case vty_long: bufstring("long"); break;
        case vty_addr: bufstring("addr"); break;
        default: bufnumber(type); break;
      }
      if (r_isdef(index, nil)) {
	bufstring(", default value=");
	bufvalue(r_read(index, nil));
      }
      bufnewline();

      subrange = r_subrange(index, nil);
      while (subrange != nil) {
	bufstring("   at address ");
	bufaddress(subrange);
	bufstring(" value=");
	bufvalue(r_read(index, subrange));
	subrange = r_subrange(index, subrange);
	bufnewline();
      }

      index = r_next(index);
    }
  }
}

/**********************************************************************/

void list_symbols(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LIST SYMBOLS\n\
\n\
This command will list all defined symbols.\n\
\n\
");
  } else {
    redconfirm(false);
    ls_symbols();
  }
}

/**********************************************************************/

void list_windows(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LIST WINDOWS\n\
\n\
This command will list all open windows, as well as the information\n\
associated with them.\n\
\n\
");
  } else {
    winindex index;

    redconfirm(false);
    index = w_next(0);
    if (index == 0) {
      bufstring("%There are no open windows.\n");
    }
    while (index != 0) {
      w_printinfo(index);
      index = w_next(index);
    }
  }
}

/**********************************************************************/

void cmd_locate(void)
{
  static keywrd cmds[] = {
    { "next",          0, (keyval) loc_next },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb locfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Item, ",
			   nil, nil };
  noise("known");
  locfdb._cmlst = nil;
  if (!dispatch(&locfdb)) {
    bufstring("\
\n\
The LOCATE command looks for various things.\n\
\n\
");
  }
}

/**********************************************************************/

void loc_next(void)
{
  static keywrd cmds[] = {
    { "reference",   0, (keyval) lnxt_reference },
   /*
    { "comment",     0, (keyval) lnxt_comment },
    { "description", 0, (keyval) lnxt_description },
    { "segment",     0, (keyval) lnxt_segment },
    { "status",      0, (keyval) lnxt_status },
    { "unknown",     0, (keyval) lnxt_unknown },
   */
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };

  static fdb nxtfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Item, ",
			nil, nil };
  nxtfdb._cmlst = nil;
  if (!dispatch(&nxtfdb)) {
    bufstring("\
\n\
The LOCATE NEXT command is used to locate the next occurance of whatever.\n\
\n\
");
  }
}

/**********************************************************************/

void lnxt_reference(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  LOCATE NEXT REFERENCE <address>\n\
\n\
This command locates the next reference to the given address.\n\
\n\
");
  } else {
    address* pos;
    int count;

    pos = parse_range(nil,  nil);
    paecheck();

    count = scanref(pos);

    if (count < 0) {
      bufstring("No match from here.\n");
    } else {
      bufstring("count = ");
      bufnumber(count);
      bufnewline();
    }
  }
}
/**********************************************************************/

void cmd_memory(void)
{
  static keywrd cmds[] = {
    { "copy",     0, (keyval) mem_copy },
    { "exclude",  0, (keyval) mem_exclude },
    { "include",  0, (keyval) mem_include },
    { "move",     0, (keyval) mem_move },
    { "relocate", 0, (keyval) mem_relocate },
    { "set",      0, (keyval) mem_set },
    { "shuffle",  0, (keyval) mem_shuffle },
    { "xor",      0, (keyval) mem_xor },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };

  static fdb memfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Command, ",
			   nil, nil };
  memfdb._cmlst = nil;
  if (!dispatch(&memfdb)) {
    bufstring("\
\n\
The MEMORY command is used to manipulate the memory in varuios ways.\n\
\n\
");
  }
}
 
/**********************************************************************/

void mem_copy(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY COPY (from) <range> TO <address>\n\
\n\
This command copies a block of memory from one address to another.\n\
\n\
");
  } else {
    static address* srcaddr = nil;
    static address* dstaddr = nil;
    
    noise("from");
    srcaddr = a_copy(parse_range(nil, fdb_TO()), srcaddr);
    dstaddr = a_copy(parse_TO(), dstaddr);

    paconfirm();

    m_copy(srcaddr, dstaddr);
  }
}

/**********************************************************************/

void mem_exclude(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY EXCLUDE <address>\n\
\n\
This command is not yet implemented.\n\
\n\
");
  } else {
    address* addr;

    addr = parse_range(nil, nil);
    paecheck();

    m_exclude(addr);
  }
}

/**********************************************************************/

void mem_include(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY INCLUDE <range>\n\
\n\
This command is a simple way of making a block of memory appear at a\n\
given address.  Previously mapped memory will retain the data it contains,\n\
new memory will be initialized to zeroes.\n\
\n\
");
  } else {
    address* addr;
    
    addr = parse_range(nil, nil);
    paecheck();

    m_include(addr);
  }
}

/**********************************************************************/

void mem_move(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY MOVE (from) <range> TO <address>\n\
\n\
This command moves a block of memory from one address to another.\n\
\n\
");
  } else {
    static address* srcaddr = nil;
    static address* dstaddr = nil;

    noise("from");
    srcaddr = a_copy(parse_range(nil, fdb_TO()), srcaddr);
    dstaddr = a_copy(parse_TO(), dstaddr);

    paconfirm();

    m_move(srcaddr, dstaddr);
  }
}

/**********************************************************************/

void mem_relocate(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY RELOCATE (from) <range> TO <destination>\n\
\n\
This command is not yet implemented.\n\
\n\
");
  } else {
    static address* srcaddr = nil;
    static address* dstaddr = nil;

    noise("from");
    srcaddr = a_copy(parse_range(nil, fdb_TO()), srcaddr);
    dstaddr = a_copy(parse_TO(), dstaddr);

    paconfirm();

    m_relocate(srcaddr, dstaddr);
  }
}

/**********************************************************************/

void mem_set(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY SET <arguments>\n\
\n\
This command is not yet implemented.\n\
\n\
");
  } else {
    address* addr;
    int val;

    addr = parse_address(nil, false);
    noise("value");
    val = parse_number("0x00", "byte to set");
    paconfirm();

    m_set(addr, val);
  }
}

/**********************************************************************/

void mem_shuffle(void)
{
  /* shuffle { byte | word | long } [bits] */

  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY SHUFFLE <arguments>\n\
\n\
This command is not yet implemented.\n\
\n\
");
  } else {
    confirm();
    notyeterror();
  }
}

/**********************************************************************/

void mem_xor(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  MEMORY XOR <arguments>\n\
\n\
This command is not yet implemented.\n\
\n\
");
  } else {
    confirm();
    notyeterror();
  }
}

/**********************************************************************/

/* Shorthand for WINDOW NEXT. */

void cmd_next(void)
{
  argwinindex = 0;		/* Shorthand, must apply to "current". */
  win_next();
}

/**********************************************************************/

static char peekformat[20] = "none";

void cmd_peek(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  PEEK [address [type [count]]]\n\
\n\
This command types out a handful (default is 20) of lines, starting\n\
at a given address.  The default address is the first one past what the\n\
previous PEEK command typed.  The default type is the same as last time.\n\
\n\
");
  } else {
    static fdb statfdb = { _CMKEY, 0, nil, (pdat) &(statustab), "format, ",
			    nil, nil };
    static address* pos = nil;
    address* defpeek;
    int count;
    stcode format;

    defpeek = getdot(1);

    if (defpeek == nil) {
      defpeek = getsfirst(1);
    }
    noise("at");
    pos = a_copy(parse_address(a_a2str(defpeek), false), pos);

    noise("format");
    statfdb._cmdef = peekformat;
    parse(&statfdb, &parseval, &used);
    format = parseval._pvkey;

    sprintf(peekformat, "%s", atombuffer);

    noise("count");
    count = parse_number("20", "number of objects");

    paconfirm();		/* REDIRECT? */

    if (!pcheck()) {
      return;
    }

    if (!mapped(pos)) {
      bufstring("%Address ");
      bufaddress(pos);
      bufstring(" is not mapped.\n");
      return;
    }

    while (count-- > 0) {
      if (!mapped(pos)) {
	bufstring("(end of memory)\n");
	break;
      }
      peek(pos, EX_LIST, format);
      a_inc(pos, pb_length * pv_bpa);
      if (bufquit()) {
	break;
      }
    }
    setdot(pos);
    bufclose();
  }
}

/**********************************************************************/

#define fmt_binary      1
#define fmt_hex         2
#define fmt_motorola    3

void cmd_read(void)
{
  static keywrd cmds[] = {
    { "binary",     0, (keyval) fmt_binary },
    { "hex",        0, (keyval) fmt_hex },
    { "motorola",   0, (keyval) fmt_motorola },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static fdb filefdb = { _CMFIL, FIL_PO+FIL_NODIR, nil, nil, nil, nil, nil };
  static fdb typefdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "File format, ",
			  "binary", nil };
  static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };

  char* filename;
  int format;
  address* addr;
  int offset;
  bool ok;
  
  if (!helpflag) {
    parse(&filefdb, &parseval, &used);
    filename = *parseval._pvfil;  
  }

  noise("format");

  if (helpflag) {
    typefdb._cmdef = nil;
    typefdb._cmlst = &cfmfdb;
  } else {
    typefdb._cmdef = "binary";
    typefdb._cmlst = nil;
  }
  parse(&typefdb, &parseval, &used);
  format = parseval._pvint;

  if (used == &cfmfdb) {
    bufstring("\
\n\
Syntax:  READ <filename> <type> [options]\n\
    or:  LOAD <filename> <type> [options]\n\
\n\
This command maps a file into memory.\n\
\n\
");
    return;
  }
  if (help()) {
    switch (format) {
    case fmt_binary:
      bufstring("\
\n\
Syntax:  READ <filename> BINARY [address [offset]]\n\
    or:  LOAD <filename> BINARY [address [offset]]\n\
\n\
This command reads a plain file, and puts it in memory sequentially,\n\
starting at the specified address.  If the address is unspecified, zero\n\
will be used.  If an offset is specified, that many bytes will be skipped\n\
from the input file.  This can be used to skip over file headers etc.\n\
\n\
");
      break;
    case fmt_hex:
      bufstring("\
\n\
Syntax:  READ <filename> HEX\n\
    or:  LOAD <filename> HEX\n\
\n\
This command reads a file with intel hex-records.\n\
\n\
");
      break;
    case fmt_motorola:
      bufstring("\
\n\
Syntax:  READ <filename> MOTOROLA\n\
    or:  LOAD <filename> MOTOROLA\n\
\n\
This command reads a file with motorola S-records.\n\
\n\
");
      break;
    }
    return;
  } else {
    switch (format) {
    case fmt_binary:
      noise("at");
      addr = parse_address("0", false);
      noise("offset");
      offset = parse_number("0", "offset in file");
      paconfirm();
      ok = load_binary(filename, addr, offset);
      break;
    case fmt_hex:
      confirm();
      ok = load_intel(filename);
      break;
    case fmt_motorola:
      confirm();
      ok = load_motorola(filename);
      break;
    }
  }
  if (ok) {
    bufstring("OK.\n");
  } else {
    /* elaborate here */
    bufstring("I/O error or something.\n");
  }
  wc_total();
}

/**********************************************************************/

static char defsavfile[200] = "disass.sav";

void cmd_restore(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  RESTORE <filename>\n\
\n\
This command is used to recover saved status (generated with the SAVE\n\
command) from a file.\n\
\n\
");
  } else {
    static fdb filfdb = { _CMFIL, FIL_RD+FIL_PO+FIL_NODIR,
			  nil, nil, nil, nil, nil };
    char* filename;

    noise("status from");
    filfdb._cmdef = defsavfile;
    parse(&filfdb, &parseval, &used);
    filename = *parseval._pvfil;  
    confirm();

    sprintf(defsavfile, "%s", filename);
    m_restore(filename);
    wc_total();
  }
}

/**********************************************************************/

void cmd_save(void)
{
  /* snytax: save [filename] */

  if (help()) {
    bufstring("\
\n\
Syntax:  SAVE <filename>\n\
\n\
This command saves all status, labels, comments etc. to a file,\n\
enabling you to backup your work, and continue later.\n\
\n\
");
  } else {
    static fdb filfdb = { _CMFIL, FIL_PO+FIL_NODIR, nil, nil, nil, nil, nil };
    char* filename;

    noise("status in");
    filfdb._cmdef = defsavfile;
    parse(&filfdb, &parseval, &used);
    filename = *parseval._pvfil;  
    confirm();
    sprintf(defsavfile, "%s", filename);
    m_save(filename);
  }
}

/**********************************************************************/

void cmd_rpn(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  RPN <expression>\n\
\n\
This command implements a simple RPN calculator.\n\
\n\
");
  } else {
    rpn();
  }
}

/**********************************************************************/

void cmd_set(void)
{
  static keywrd cmd1[] = {
    { "arguments",   0,  (keyval) set_arguments },
    { "comment",     0,  (keyval) set_comment },
    { "description", 0,  (keyval) set_description },
    { "expansion",   0,  (keyval) set_expansion },
    { "flags",	     0,  (keyval) set_flags },
    { "label",       0,  (keyval) set_label },
    { "noreturn",    0,  (keyval) set_noreturn },
    { "status",      0,  (keyval) set_status },
  };
  static keytab cmd1tab = { (sizeof(cmd1)/sizeof(keywrd)), cmd1 };
  static fdb set1fdb = { _CMKEY, 0, nil, (pdat) &(cmd1tab), "Attribute, ",
			  nil, nil };

  static keywrd cmd2[] = {
    { "cpu",         0,  (keyval) set_processor },
    { "highlight",   0,  (keyval) set_highlight },
    { "processor",   0,  (keyval) set_processor },
    { "register",    0,  (keyval) set_register },
    { "symbol",      0,  (keyval) def_symbol },
  };
  static keytab cmd2tab = { (sizeof(cmd2)/sizeof(keywrd)), cmd2 };
  static fdb set2fdb = { _CMKEY, 0, nil, (pdat) &(cmd2tab), "Keyword, ",
			  nil, nil };

  static keywrd cmd3[] = {
    { "case",        0,  (keyval) set_case },
    { "charset",     0,  (keyval) set_charset },
    { "color",       0,  (keyval) set_color },
    { "delimiter",   0,  (keyval) set_delimiter },
    { "display",     0,  (keyval) set_display },
    { "note",        0,  (keyval) set_note },
    { "objtype",     0,  (keyval) set_objtype },
    { "pager",       0,  (keyval) set_pager },
    { "radix",       0,  (keyval) set_radix },
    { "syntax",      0,  (keyval) set_syntax },
  };
  static keytab cmd3tab = { (sizeof(cmd3)/sizeof(keywrd)), cmd3 };
  static fdb set3fdb = { _CMKEY, 0, nil, (pdat) &(cmd3tab), "Variable, ",
			  nil, nil };

  if (!dispatch(fdbchn(&set1fdb, &set2fdb, &set3fdb, nil))) {
    bufstring("\
\n\
help text for the SET command.\n\
\n\
");
  }
}

/**********************************************************************/

void set_arguments(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET ARGUMENTS <address> <pattern>\n\
\n\
This command sets a list of inline argument types for a given function\n\
or procedure at the given address.\n\
\n\
");
  } else {
    address* addr;
    pattern* pat;

    noise("at address");
    addr = parse_address(nil, false);

    noise("pattern");
    pat = parse_pattern(false, nil);

    /* parse_pattern() does confirm and paecheck for us. */

    ia_write(addr, pat);
  }
}

/**********************************************************************/

void set_comment(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET COMMENT <address> <string>\n\
\n\
This command associates a comment string with a specified address.\n\
\n\
");
  } else {
    char* commstr;
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);

    noise("comment");
    commstr = parse_text("comment string");

    paconfirm();

    c_insert(addr, commstr);
  }
}

/**********************************************************************/

static char* texti(char* help, char* prevtext)
{
  para_data pd = { nil, nil };
  static fdb parfdb = { _CMPARA, CM_NEOF, nil, nil, nil, nil, nil };
  char* text;

  parfdb._cmdat = (pdat) &pd;
  pd.buf = prevtext;

  cmxprintf(" %s (End with CTRL/D\n", help);
  cmxprintf("\
  Use CTRL/B to insert a file, CTRL/E to enter editor, CTRL/K to redisplay\n\
  text, CTRL/L to clear screen and redisplay, CTRL/N to abort.):\n");
  parse(&parfdb, &parseval, &used);
  if (parseval._pvpara == nil) {
    cmxprintf("Aborted!\n");
    return(nil);
  }
  text = copystring(parseval._pvpara, nil);
  return(text);
}

void set_description(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET DESCRIPTION <address> <string>\n\
\n\
This command associates a description string with a specified address.\n\
\n\
");
  } else {
    static fdb textfdb = { _CMTXT, 0, nil, nil, nil, nil, nil };
    static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };
    static char* descstr = nil;
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);
    noise("description");
    parse(fdbchn(&cfmfdb, &textfdb, nil), &parseval, &used);
    if (descstr) {
      free(descstr);
      descstr = nil;
    }
    if (used == &textfdb) {
      descstr = copystring(atombuffer, nil);
      paconfirm();
    } else {
      paecheck();
      descstr = texti("Description", nil);
    }
    if (descstr != nil) {
      d_insert(addr, descstr);
      free(descstr);
      descstr = nil;
    }
  }
}

/**********************************************************************/

void set_expansion(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET EXPANSION <address> <string>\n\
\n\
This command associates an expansion string with a specified address.\n\
\n\
");
  } else {
    address* addr;
    char* expstr;
    int explen;

    noise("at address");
    addr = parse_address(nil, false);

    noise("length");
    explen = parse_number("1", "length of expansion");

    noise("expansion");
    expstr = parse_text("expansion string");

    paconfirm();

    e_insert(addr, expstr, explen);
  }
}

/**********************************************************************/

void set_flags(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET FLAGS <address> <flag list>\n\
\n\
This command sets special flags for an address.\n\
\n\
");
  } else {
    static fdb statusfdb = { _CMKEY, 0, nil, (pdat) &(statustab), "status, ",
			     nil, nil };
    address* addr;
    int length;
    byte flags[256];
    byte* oldflags;
    int pos;
    stcode status;

    for (pos = 1; pos <= 255; pos += 1) {
      flags[pos] = st_none;
    }
    length = 0;

    addr = parse_address(nil, false);

    if (addr != nil) {
      oldflags = f_read(addr);
      if (oldflags != nil) {
	length = oldflags[0];
	for (pos = 1; pos <= length; pos += 1) {
	  flags[pos] = oldflags[pos];
	}
      }
    }

    pos = parse_number(nil, "flag pos");

    parse(&statusfdb, &parseval, &used);
    status = parseval._pvkey;

    confirm();

    paecheck();

    flags[pos] = status;
    if (pos > length) {
      length = pos;
    }
    flags[0] = length;

    f_write(addr, &flags[0]);
  }
}

/**********************************************************************/

void set_highlight(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET HIGHLIGHT <address>\n\
\n\
This command sets a highlight point at the given address.\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_range(nil, nil);
    paecheck();
    hl_write(addr);
  }
}

/**********************************************************************/

void set_label(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET LABEL <address> [string]\n\
\n\
This command defines a label.  If there is a label string given, and\n\
it is legal according to the current syntax, it will be used, other-\n\
wise a label will be generated.\n\
\n\
");
  } else {
    static char* labelstr = nil;
    address* addr;

    static fdb labelfdb = { _CMFLD, CM_SDH | FLD_EMPTY, nil, nil,
			     "Name of label", nil, &labelbrk };
    static fdb cfmfdb = { _CMCFM, CM_SDH, nil, nil,
			   "Confirm to generate a label", nil, nil };

    noise("at");
    addr = parse_address(nil, false);

    noise("label");
    parse(fdbchn(&cfmfdb, &labelfdb, nil), &parseval, &used);
    if (used == &cfmfdb) {
      paecheck();
      l_generate(addr);
      if (l_exist(addr)) {
	bufstring("Generated label: ");
	bufstring(l_find(addr));
	bufnewline();
      }
    } else {
      labelstr = copyatom(labelstr);
      paconfirm();

      if (!l_check(labelstr)) {
	bufstring("%Label does not conform to the current syntax.\n");
      } else {
	l_insert(addr, labelstr);
      }
    }
  }
}

/**********************************************************************/

void set_noreturn(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET NORETURN <address>>\n\
\n\
This command flags the address as containing a routine that does\n\
not return.\n\
\n\
");
  } else {
    address* addr;

    noise("at address");
    addr = parse_address(nil, false);

    paconfirm();

    nrf_write(addr, true);
  }
}

/**********************************************************************/

void set_register(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET REGISTER <register> <value> [@ address]\n\
\n\
This command sets register values.\n\
\n\
");
  } else {
    char* regname;
    regindex index;
    int type;
    value* val;
    address* addr;

    regname = parse_objname("name of register");
    index = r_index(regname);
    type = r_type(index);
    if (type == vty_none) {
      type = vty_long;
    }

    noise("value");
    val = parse_value(type, nil, "value of register");

    addr = parse_regrange();

    paecheck();			/* Check for bad labels. */

    if (index == 0) {
      index = r_define(regname, vty_long);
    }
    r_write(index, addr, val);
  }
}

/**********************************************************************/

void set_case(void)
{
  if (help()) {
    bufstring("\
\n\
The SET CASE command sets the prefered case for the output data,\n\
if supported by the processor.\n\
\n\
");
  } else {
    static keywrd cmd[] = {
      { "default",  0,  (keyval) nil },
      { "lower",    0,  (keyval) "lower" },
      { "initial",  0,  (keyval) "initial" },
      { "upper",    0,  (keyval) "upper" },
    };
    static keytab cmdtab = { (sizeof(cmd)/sizeof(keywrd)), cmd };
    static fdb casefdb = { _CMKEY, 0, nil, (pdat) &(cmdtab),
			   "case, ", "default", nil };
    char* val;

    parse(&casefdb, &parseval, &used);
    val = (char *) parseval._pvkey;
    confirm();

    s_write(s_define("casing"), val);
  }
}

/**********************************************************************/

void set_color(void)
{
  if (help()) {
    bufstring("\
\n\
The SET COLOR command sets the highlight color to be used.\n\
\n\
");
  } else {
    char* val;

    val = parse_text("color name");
    confirm();
    w_setcolor(val);
  }
}

/**********************************************************************/

void set_charset(void)
{
  if (help()) {
    bufstring("\
\n\
The SET CHARSET command sets the character set for string constants and\n\
things like that.\n\
\n\
");
  } else {
    static keywrd cmd[] = {
      { "default",  0,  (keyval) 0 },
    };
    static keytab cmdtab = { (sizeof(cmd)/sizeof(keywrd)), cmd };
    static fdb keywfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab),
			   "charset, ", "default", nil };
    int val;

    parse(&keywfdb, &parseval, &used);
    val = parseval._pvint;
    confirm();
    notyeterror();
  }
}

/**********************************************************************/

void set_delimiter(void)
{
  if (help()) {
    bufstring("\
\n\
The SET DELIMITER command sets the type of delimiter you want between\n\
opcode and arguments.\n\
\n\
");
  } else {
    static keywrd cmd[] = {
      { "default",  0,  (keyval) nil },
      { "space",    0,  (keyval) " " },
      { "tab",      0,  (keyval) "\t" },
    };
    static keytab cmdtab = { (sizeof(cmd)/sizeof(keywrd)), cmd };
    static fdb delimfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab),
			    "delimiter, ", "default", nil };
    char* val;

    parse(&delimfdb, &parseval, &used);
    val = (char *) parseval._pvkey;

    confirm();

    s_write(s_define("opdelim"), val);
  }
}

/**********************************************************************/

void set_display(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET DISPLAY <string>\n\
\n\
This command sets the environment variable DISPLAY to the given value.\n\
\n\
");
  } else {
    char* display;

    noise("to");
    display = parse_text("display");
    confirm();
    sy_setenv("DISPLAY", display);
  }
}

/**********************************************************************/

void set_note(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET NOTE <string>\n\
\n\
This command makes a note.\n\
\n\
");
  } else {
    char* note;

    noise("to");
    note = parse_text("note to make");
    confirm();
    n_write(note);
  }
}

/**********************************************************************/

void set_objtype(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET OBJTYPE <type>\n\
\n\
This command sets the default object type for unknown data.\n\
\n\
");
  } else {
    static fdb statusfdb = { _CMKEY, 0, nil, (pdat) &(statustab), "status, ",
			     nil, nil };
    stcode status;

    noise("to");

    parse(&statusfdb, &parseval, &used);
    status = parseval._pvkey;

    confirm();

    defobjstatus = status;
  }
}

/**********************************************************************/

void set_pager(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET PAGER <string>\n\
\n\
This command sets the environment variable PAGER to the given value.\n\
\n\
");
  } else {
    char* pager;

    noise("to");
    pager = parse_text("pager");
    confirm();
    sy_setenv("PAGER", pager);
  }
}

/**********************************************************************/

void set_radix(void)
{
  if (help()) {
    bufstring("\
\n\
The SET RADIX command sets the prefered radix for the output data,\n\
if supported by the processor.\n\
\n\
");
  } else {
    static keywrd cmd[] = {
      { "default",  0,  (keyval) nil },
      { "binary",   0,  (keyval) "2" },
      { "octal",    0,  (keyval) "8" },
      { "decimal",  0,  (keyval) "10" },
      { "hex",      0,  (keyval) "16" },
    };
    static keytab cmdtab = { (sizeof(cmd)/sizeof(keywrd)), cmd };
    static fdb radixfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab),
			    "radix, ", "default", nil };
    char* val;

    parse(&radixfdb, &parseval, &used);
    val = (char *) parseval._pvkey;
    confirm();
    
    s_write(s_define("radix"), val);
  }
}

/**********************************************************************/

void set_syntax(void)
{
  if (help()) {
    bufstring("\
\n\
The SET SYNTAX command select which type of assembler syntax to\n\
use.  There are several values, and exactly what they mean\n\
depends on the selected processor.\n\
\n\
This command is equivalent to setting the symbol \"syntax\" to a\n\
suitable value.\n\
\n\
");
  } else {
    static keywrd cmd[] = {
      { "default",  0,  (keyval) nil },
      { "native",   0,  (keyval) "native" },
      { "foreign",  0,  (keyval) "foreign" },
      { "unix",     0,  (keyval) "unix" },
    };
    static keytab cmdtab = { (sizeof(cmd)/sizeof(keywrd)), cmd };
    static fdb asmfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "syntax, ",
			   "default", nil };
    char* val;

    parse(&asmfdb, &parseval, &used);
    val = (char *) parseval._pvkey;

    confirm();

    s_write(s_define("syntax"), val);
  }
}

/**********************************************************************/

void set_processor(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET PROCESSOR <cpu>\n\
    or:  SET CPU <cpu>\n\
\n\
This command sets the processor to disassemble for.\n\
\n\
To get a list of available processors, use the command 'LIST PROCESSORS'\n\
\n\
");
  } else {
    struct entryvector* p;

    noise("to");
    p = parse_processor(nil);

    confirm();

    setproc(p);
  }
}

/**********************************************************************/

void set_status(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET STATUS <address> <pattern>\n\
\n\
This command sets the status of a specified range of addresses to a\n\
given value.\n\
\n\
Try the HELP STATUS command for an explanation on status codes.\n\
\n\
");
  } else {
    address* addr;
    address* this;

    pattern* pstart;
    pattern* pat;
    int count, size;

    noise("at address");
    addr = parse_range(nil, fdb_pattern());
    
    pstart = parse_pattern(true, nil);
    paecheck();

    while (addr != nil) {
      this = a_car(addr);
      addr = a_cdr(addr);

      pat = pstart;
      if (a_ismulti(this)) {
	count = a_diff(a_last(this), this) + 1;
	while (count > 0) {
	  size = pe_size(this, pat);
	  setstatus(this, pat->status, size);
	  a_inc(this, size);
	  pat = pat->next;
	  if (pat == nil) {
	    pat = pstart;
	  }
	  count -= size;
	}
      } else {
	while (pat != nil) {
	  size = pe_size(this, pat);
	  setstatus(this, pat->status, size);
	  a_inc(this, size);
	  pat = pat->next;
	}
      }
    }
    wc_total();			/* Selective? */
  }
}

/**********************************************************************/

void cmd_show(void)
{
  static keywrd cmd1[] = {
    { "arguments", 0,   (keyval) show_arguments },
    { "comment",   0,   (keyval) show_comment },
    { "description", 0, (keyval) show_description },
    { "expansion", 0,   (keyval) show_expansion },
    { "flags",     0,   (keyval) show_flags },
    { "label",     0,   (keyval) show_label },
    { "noreturn",  0,   (keyval) show_noreturn },
    { "status",    0,   (keyval) show_status },
  };
  static keytab cmd1tab = { (sizeof(cmd1)/sizeof(keywrd)), cmd1 };
  static fdb show1fdb = { _CMKEY, 0, nil, (pdat) &cmd1tab, "Attribute, ",
			    nil, nil };
  static keywrd cmd2[] = {
    { "auto",      0,   (keyval) show_auto },
    { "character", 0,   (keyval) show_character },
    { "counters",  0,   (keyval) show_counters },
    { "cpu",       0,   (keyval) show_processor },
    { "dots",      0,   (keyval) show_dots },
    { "memory",    0,   (keyval) show_memory },
    { "note",      0,   (keyval) show_note },
    { "pattern",   0,   (keyval) show_pattern },
    { "processor", 0,   (keyval) show_processor },
    { "register",  0,   (keyval) show_register },
    { "symbol",    0,   (keyval) show_symbol },
  };
  static keytab cmd2tab = { (sizeof(cmd2)/sizeof(keywrd)), cmd2 };
  static fdb show2fdb = { _CMKEY, 0, nil, (pdat) &(cmd2tab), "Keyword, ",
			   nil, nil };

  if (!dispatch(fdbchn(&show1fdb, &show2fdb, nil))) {
    bufstring("\
\n\
help text for the SHOW command.\n\
\n\
");
  }
}

/**********************************************************************/

void show_arguments(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW ARGUMENTS <address>\n\
\n\
This command shows the inline argument list for an address.\n\
\n\
");
  } else {
    address* addr;
    pattern* pat;

    noise("of address");
    addr = parse_address(nil, false);
    paconfirm();

    pat = ia_read(addr);

    if (pat == nil) {
      bufstring("There is no argument list for that address.");
    }
    while (pat != nil) {
      switch (pat->status) {
	case st_none:   bufstring("none");   break;
	case st_cont:   bufstring("cont");   break;
	case st_byte:   bufstring("byte");   break;
	case st_char:   bufstring("char");   break;
	case st_double: bufstring("double"); break;
	case st_float:  bufstring("float");  break;
	case st_inst:   bufstring("inst");   break;
	case st_long:   bufstring("long");   break;
	case st_mask:   bufstring("mask");   break;
	case st_octa:   bufstring("octa");   break;
	case st_ptr:    bufstring("ptr");    break;
	case st_quad:   bufstring("quad");   break;
	case st_text:   bufstring("text");   break;
	case st_word:   bufstring("word");   break;
	default:        bufstring("unknown"); break;
      }
      if (pat->length != 0) {
	bufchar(':');
	bufnumber(pat->length);
      }
      bufchar(' ');
      pat = pat->next;
    }
    bufnewline();
  }
}

/**********************************************************************/

void show_auto(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW AUTO\n\
\n\
This command shows you the starting points for the DO AUTO command.\n\
\n\
");
  } else {
    address* addr;

    confirm();

    if (pcheck()) {
      addr = autolist();
      if (addr == nil) {
	bufstring("%There are no auto points.\n");
      } else {
	bufstring("auto: ");
	bufaddress(addr);
	bufnewline();
      }
    }
  }
}

/**********************************************************************/

void show_character(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW CHARACTER <number>\n\
\n\
This command translates a number to a character.  Only the lower eight\n\
bits of the number will be used.\n\
\n\
");
  } else {
    char* p;
    byte n;

    n = (parse_number(nil, nil) & 0xff);
    confirm();

    bufoctal(n, 3);
    bufstring(" (oct), ");
    bufdecimal(n, 3);
    bufstring(" (dec), 0x");
    bufhex(n, 2);
    bufstring(" (hex) = ");

    if (n >= 0200) {
      bufstring(" parity and ");
      n -= 0200;
    }
    if (n < 32) {
      bufchar('^');
      bufchar((char) n + 64);
      p = charname(n);
      if (*p != '^') {
	bufstring(" (");
	bufstring(p);
	bufstring(")");
      } else {
      }
    } else if (n == 0177) {
      bufstring(" ^? (rubout)");
    } else { 
      bufchar('"');
      bufchar((char) n);
      bufchar('"');
    }
    bufnewline();
  }
}

/**********************************************************************/

void show_counters(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW COUNTERS\n\
\n\
This command shows various counters.\n\
\n\
");
  } else {
    static address* pos = nil;
    int i;
    int segment, maxseg;
    stcode status, prevstat;
    int cc, dc, ec, lc;

    int bcount[16] = { 0 };
    int icount[16] = { 0 };

    confirm();

    cc = c_count();
    dc = d_count();
    ec = e_count();
    lc = l_count();
    maxseg = getscount();
    if ((cc + dc + ec + lc + maxseg) == 0) {
      bufstring("There is nothing in the database.\n");
      return;
    }
    for (segment = 1; segment <= maxseg; segment += 1) {
      pos = a_copy(getsfirst(segment), pos);
      i = getslength(segment);
      prevstat = st_none;
      while (i > 0) {
	status = getstatus(pos);
	if (status == st_cont) {
	  bcount[prevstat] += 1;
	} else {
	  icount[status] += 1;
	  bcount[status] += 1;
	  prevstat = status;
	}
	i -= pv_bpa;
	a_inc(pos, pv_bpa);
      }
    }
    for (status = (stcode) 0; status < (stcode) 16; status += 1) {
      if (bcount[status] > 0) {
	switch (status) {
	  case st_none:   bufstring("unknown:      "); break;
	  case st_inst:   bufstring("instructions: "); break;
	  case st_byte:   bufstring("bytes:        "); break;
	  case st_word:   bufstring("words:        "); break;
	  case st_long:   bufstring("longwords:    "); break;
	  case st_quad:   bufstring("quadwords:    "); break;
	  case st_octa:   bufstring("octawords:    "); break;
	  case st_char:   bufstring("characters:   "); break;
	  case st_text:   bufstring("texts:        "); break;
	  case st_ptr:    bufstring("pointers:     "); break;
	  case st_float:  bufstring("floats:       "); break;
	  case st_double: bufstring("doubles:      "); break;
	  case st_mask:   bufstring("masks:        "); break;
	}
	bufnumber(icount[status]);
	bufstring("  (");
	bufnumber(bcount[status]);
	bufstring(" bytes)\n");
      }
    }
    if ((cc + dc + ec + lc) > 0) {
      bufnewline();
    }
    if (cc > 0) {
      bufstring("comments:     ");
      bufnumber(cc);
      bufnewline();
    }
    if (dc > 0) {
      bufstring("descriptions: ");
      bufnumber(dc);
      bufnewline();
    }
    if (ec > 0) {
      bufstring("expansions:   ");
      bufnumber(ec);
      bufnewline();
    }
    if (lc > 0) {
      bufstring("labels:       ");
      bufnumber(lc);
      bufnewline();
    }
  }
}

/**********************************************************************/

void show_comment(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW COMMENT <address>\n\
\n\
This command simply shows the comment on a certain address.\n\
\n\
\n\
");
  } else {
    address* addr;

    noise("of address");
    addr = parse_address(nil, false);
    paconfirm();
    if (c_exist(addr)) {
      bufstring(c_find(addr));
      bufnewline();
    } else {
      bufstring("%There is no comment at that address.\n");
    }
  }
}

/**********************************************************************/

void show_description(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW DESCRIPTION <address>\n\
\n\
This command shows the description on a certain address.\n\
\n\
");
  } else {
    address* addr;

    noise("of address");
    addr = parse_address(nil, false);
    paconfirm();
    if (d_exist(addr)) {
      bufstring(d_find(addr));
      bufnewline();
    } else {
      bufstring("%There is no description at that address.\n");
    }
  }
}

/**********************************************************************/

void show_dots(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW DOTS\n\
\n\
This command shows various 'dots' variables.\n\
\n\
");
  } else {
    confirm();

    ls_dots();
  }
}

/**********************************************************************/

void show_expansion(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW EXPANSION <address>\n\
\n\
This command shows the expansion on a certain address.\n\
\n\
");
  } else {
    address* addr;

    noise("of address");
    addr = parse_address(nil, false);
    paconfirm();
    if (e_exist(addr)) {
      bufnumber(e_length(addr));
      bufstring("  ");
      bufstring(e_find(addr));
      bufnewline();
    } else {
      bufstring("%There is no expansion at that address.\n");
    }
  }
}

/**********************************************************************/

void show_flags(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW FLAGS <address>\n\
\n\
This command shows the flags (if any) set for a certain address.\n\
\n\
");
  } else {
    address* addr;
    byte* flags;
    int pos;
    int length;
    stcode status;

    noise("of address");
    addr = parse_address(nil, false);
    paconfirm();

    flags = f_read(addr);
    if (flags == nil) {
      bufstring("There are no flags set at address ");
      bufaddress(addr);
      bufstring(" now.\n");
    } else {
      length = flags[0];
      for (pos = 1; pos <= length; pos += 1) {
	status = flags[pos];
	bufstring("pos = ");
	bufnumber(pos);
	bufstring(", status = ");
	bufchar(st2char(status));
	bufnewline();
      }
    }
  }
}

/**********************************************************************/

void show_label(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW LABEL\n\
\n\
This command simply shows the value of a specified label.\n\
\n\
");
  } else {
    static fdb labelfdb = { _CMFLD, FLD_EMPTY+CM_SDH, nil, nil,
			    "Name of label", nil, &labelbrk };
    static fdb qstrfdb = { _CMQST, CM_SDH, nil, nil, nil, nil, nil };
    address* a;
    char* l;

    parse(fdbchn(&labelfdb, &qstrfdb, nil), &parseval, &used);
    l = parseval._pvstr;
    confirm();

    bufstring("Label ");
    bufstring(l);
    a = l_lookup(l);
    if (a != nil) {
      bufstring(" has value ");
      bufaddress(a);
    } else {
      bufstring(" does not exist.");
    }
    bufnewline();
  }
}

/**********************************************************************/

void show_memory(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW MEMORY\n\
\n\
This command gives a list of all mapped memory.\n\
\n\
");
  } else {
    int i, count;
    longword size;
    longword total;

    confirm();

    count = getscount();
    if (count == 0) {
      bufstring("%There is no memory mapped.\n");
    } else {
      total = 0L;
      for(i = 1; i <= count; i += 1) {
	size = getslength(i);
	bufstring("Segment ");
	bufnumber(i);
	bufstring(", at ");
	bufaddress(getsfirst(i));
	bufstring("-");
	bufaddress(getslast(i));
	bufstring(" len ");
	if (pv_bpa > 1) {
	  size = (size + pv_bpa - 1) / pv_bpa;
	}
	bufsize(size);
	total += size;
	if (bufquit()) break;
      }
      if (count > 1) {
	bufstring("Total size: ");
	bufsize(total);
      }
    }
  }
}

/**********************************************************************/

void show_noreturn(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW NORETURN <address>\n\
\n\
This command shows the noreturn flag for a certain address.\n\
\n\
");
  } else {
    address* addr;

    noise("for address");
    addr = parse_address(nil, false);
    paconfirm();
    bufstring("The noreturn flag for address ");
    bufaddress(addr);
    if (nrf_read(addr)) {
      bufstring(" is set.");
    } else {
      bufstring(" is clear.");
    }
    bufnewline();
  }
}

/**********************************************************************/

void show_note(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW NOTE <number>\n\
\n\
This command shows the given note.\n\
\n\
");
  } else {
    confirm();
    notyeterror();
  }
}

/**********************************************************************/

void show_pattern(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW PATTERN <name>\n\
\n\
This command shows the definition of the specified pattern.\n\
\n\
");
  } else {
    char* patname;
    patindex index;

    noise("name");
    patname = parse_objname("name of pattern");
    confirm();

    index = p_index(patname);
    if (index == 0) {
      bufstring("%There is no pattern named '");
      bufstring(patname);
      bufstring("'.\n");
    } else {
      bufpattern(index);
    }
  }
}

/**********************************************************************/

void show_processor(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW PROCESSOR\n\
    or:  SHOW CPU\n\
\n\
This command simply shows which processor is selected.\n\
\n\
");
  } else {
    confirm();
    if (pcheck()) {
      bufstring("Processor (cpu) type: ");
      bufstring(processor->name);
      bufnewline();
    }
  }
}

/**********************************************************************/

void show_register(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW REGISTER <register> [@ <address>]\n\
\n\
This command needs a lot of work.\n\
\n\
If you want a list of all known registers, use \"LIST REGISTERS\".\n\
\n\
");
  } else {
    static fdb regfdb = { _CMFLD, FLD_EMPTY+CM_SDH, nil, nil, 
			  "name of register", nil, nil };
    static char* regname = nil;
    regindex index;
    address* addr;

    parse(&regfdb, &parseval, &used);

    regname = copyatom(regname);
    addr = parse_regrange();
    index = r_index(regname);
    bufstring("Register ");
    bufstring(regname);
    if (index == 0) {
      bufstring(" is undefined at this moment.");
    } else if (!r_isdef(index, addr)) {
      if (addr == nil) {
	bufstring(" has no default value.");
      } else {
	bufstring(" is undefined at address ");
	bufaddress(addr);
      }
    } else {
      bufstring(" has value ");
      bufvalue(r_read(index, addr));
      if (addr != nil) {
	bufstring(" at address ");
	bufaddress(addr);
      }
    }
    bufnewline();
  }
}

/**********************************************************************/

void show_status(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW STATUS <address> <count>\n\
\n\
This command displays the status of a range of addresses.\n\
\n\
");
  } else {
    static address* addr = nil;
    int count, i, hpos;

    noise("of address");
    addr = a_copy(parse_address(a_a2str(getsfirst(1)), false), addr);
    noise("count");
    count = 0;
    if (addr != nil) {
      count = getsrest(addr);
    }
    if (count > (64 * 20)) {
      count = 64*20;
    }
    if (count != 0) {
      count = parse_number(l2s(count), nil);
    } else {
      count = parse_number("64", nil);
    }
    paconfirm();

    while (count > 0) {
      bufaddress(addr);
      bufchar(':');
      tabto(7);
      i = 64;
      if (i > count) {
	i = count;
      }
      count -= i;
      hpos = 0;
      while (i-- > 0) {
	if ((hpos & 0x0f) == 0) {
	  bufchar(' ');
	}
	hpos += 1;
	bufchar(st2char(getstatus(addr)));
	a_inc(addr, pv_bpa);
      }
      bufnewline();
    }
  }
}

/**********************************************************************/

void show_symbol(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW SYMBOL <name>\n\
\n\
This command shows the definition of the specified symbol.\n\
\n\
If you want a list of all known symbols, use \"LIST SYMBOLS\".\n\
\n\
");
  } else {
    char* symname;
    symindex index;

    noise("name");
    symname = parse_objname("name of symbol");
    confirm();

    index = s_index(symname);
    if (index == 0) {
      bufstring("%There is no symbol named '");
      bufstring(symname);
      bufstring("'.\n");
    } else {
      bufsymbol(index);
    }
  }
}

/**********************************************************************/

void cmd_step(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  STEP\n\
\n\
This command is not yet implemented.\n\
\n\
");
  } else {
    confirm();
    notyeterror();
  }
}

/**********************************************************************/

void cmd_take(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  TAKE <filename>\n\
\n\
This command takes a bunch of commands from a file.\n\
\n\
");
  } else {
    cmtake(commandloop);
  }
}

/**********************************************************************/

void cmd_unset(void)
{
  static keywrd cmds[] = {
    { "noreturn", 0, (keyval) unset_noreturn },
    { "register", 0, (keyval) unset_register },
    { "symbol",   0, (keyval) unset_symbol },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };

  static fdb unsetfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Command, ",
			  nil, nil };

  if (!dispatch(&unsetfdb)) {
    bufstring("\
\n\
The UNSET command is used to unset various things.\n\
\n\
");
  }
}

/**********************************************************************/

void unset_noreturn(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  UNSET NORETURN <address>>\n\
\n\
This command removes the noreturn flag from the given address.\n\
\n\
");
  } else {
    address* addr;

    noise("from address");
    addr = parse_address(nil, false);

    paconfirm();

    nrf_write(addr, false);
  }
}

/**********************************************************************/

void unset_register(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  UNSET REGISTER <name> [@ address]\n\
\n\
This command removes the value of a register, either the main value\n\
or the value(s) for the specified range.\n\
\n\
");
  } else {
    char* regname;
    regindex index;
    address* addr;

    regname = parse_objname("name of register");
    index = r_index(regname);
    addr = parse_regrange();	/* This does a confirm. */

    r_write(index, addr, nil);
  }
}

/**********************************************************************/

void unset_symbol(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  UNSET SYMBOL <name>\n\
\n\
This command removes the value of a symbol, but leaves the symbol\n\
itself defined.\n\
\n\
");
  } else {
    char* symname;
    symindex index;

    noise("name");
    symname = parse_objname("name of symbol");

    confirm();

    index = s_index(symname);
    if (index == 0) {
      bufstring("The symbol does not exist.\n");
    } else {
      s_write(index, nil);
    }
  }
}

/**********************************************************************/

void cmd_window(void)
{
  static keywrd cmds[] = {
    { "address",  0, (keyval) win_address },
    { "close",    0, (keyval) win_close },
    { "current",  0, (keyval) win_current },
    { "lower",    0, (keyval) win_lower },
    { "move",     0, (keyval) win_move },
    { "next",     0, (keyval) win_next },
    { "raise",    0, (keyval) win_raise },
    { "segment",  0, (keyval) win_segment },
  };
  static keywrd openkey[] = {
    { "open",     0, (keyval) win_open },
  };

  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
  static keytab opntab = { (sizeof(openkey)/sizeof(keywrd)), openkey };

  static fdb numfdb = { _CMNUM, NUM_US+CM_SDH, nil, (pdat) 10,
			"Target window index", nil, nil };
  static fdb winfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Command, ",
			nil, nil };
  static fdb opnfdb = { _CMKEY, CM_SDH, nil, (pdat) &(opntab),
			 "the keyword \"open\" to open a new window",
			nil, nil };
  static fdb cfmfdb = { _CMCFM, 0, nil, nil, nil, nil, nil };

  argwinindex = 0;		/* Clear target index. */

  if (helpflag) {
    parse(fdbchn(&winfdb, &opnfdb, &cfmfdb, nil), &parseval, &used);
  } else {
    parse(fdbchn(&winfdb, &numfdb, &opnfdb, nil), &parseval, &used);
    if (used == &numfdb) {
      argwinindex = parseval._pvint;
      parse(fdbchn(&winfdb, nil), &parseval, &used);
    }
  }

  if (used == &cfmfdb) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] <function>\n\
    or:  WINDOW OPEN <type>\n\
\n\
This command manipulates or opens windows for various information\n\
purpouses.\n\
\n\
");
  } else {
    execute(parseval._pvfunc);
  }
}

/**********************************************************************/

void win_address(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] ADDRESS <address>\n\
\n\
This command sets the starting address of the window.\n\
\n\
");
  } else {
    address* addr;

    noise("to address");
    addr = parse_address(nil, false);
    paconfirm();

    w_setaddr(argwinindex, addr);
  }
}

/**********************************************************************/

void win_close(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] CLOSE\n\
\n\
This command closes the window.\n\
\n\
");
  } else {
    confirm();
    w_close(argwinindex);
  }
}

/**********************************************************************/

void win_current(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW <number> CURRENT\n\
\n\
This command sets the window that the other window commands operate on.\n\
\n\
");
  } else {
    confirm();
    w_setcurrent(argwinindex);
  }
}

/**********************************************************************/

void win_lower(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] LOWER\n\
\n\
This command puts the window at the bottom of the stacking order.\n\
\n\
");
  } else {
    confirm();
    w_lower(argwinindex);
  }
}

/**********************************************************************/

void win_move(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] MOVE <direction> <amount>\n\
\n\
This command moves the window <amount> pixels in the specified direction.\n\
The direction can be up, down, left or right.\n\
\n\
");
  } else {
    static keywrd cmd[] = {
      { "down",  0, (keyval) 'd' },
      { "left",  0, (keyval) 'l' },
      { "right", 0, (keyval) 'r' },
      { "up",    0, (keyval) 'u' },
    };
    static keytab cmdtab = { (sizeof(cmd)/sizeof(keywrd)), cmd };
    static fdb movefdb = { _CMKEY, 0, nil, (pdat) &(cmdtab),
			   "direction, ", nil, nil };
    char direction;
    int amount;

    parse(&movefdb, &parseval, &used);
    direction = (char) parseval._pvint;
    amount = parse_number(nil, "number of pixels");
    confirm();

    switch (direction) {
      case 'd': w_move(argwinindex, 0,  amount); break;
      case 'u': w_move(argwinindex, 0, -amount); break;
      case 'r': w_move(argwinindex,  amount, 0); break;
      case 'l': w_move(argwinindex, -amount, 0); break;
    }
  }
}

/**********************************************************************/

void win_next(void)
{
  static keywrd cmds[] = {
    { "comment",     0, (keyval) wnxt_comment },
    { "description", 0, (keyval) wnxt_description },
    { "reference",   0, (keyval) wnxt_reference },
    { "screen",      0, (keyval) wnxt_screen },
    { "segment",     0, (keyval) wnxt_segment },
    { "status",      0, (keyval) wnxt_status },
    { "unknown",     0, (keyval) wnxt_unknown },
  };
  static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };

  static fdb nxtfdb = { _CMKEY, 0, nil, (pdat) &(cmdtab), "Command, ",
			"screen", nil };
  nxtfdb._cmlst = nil;
  if (!dispatch(&nxtfdb)) {
    bufstring("\
\n\
The [WINDOW] NEXT command is used to scroll the window in various ways.\n\
\n\
");
  }
}

/**********************************************************************/

void wnxt_comment(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT COMMENT\n\
\n\
This command moves the window forward to the next address with a\n\
comment.\n\
\n\
");
  } else {
    address* pos;

    confirm();

    pos = w_getaddr(argwinindex);
    if (pos == nil) {
      bufstring("%This window does not have an address.\n");
    } else {
      a_inc(pos, pv_bpa);
      while (mapped(pos) && !c_exist(pos)) {
	a_inc(pos, pv_bpa);
      }
      if (mapped(pos)) {
	w_setaddr(argwinindex, pos);
      } else {
	bufstring("%There is no comment from here on.\n");
      }
    }
  }
}

/**********************************************************************/

void wnxt_description(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT DESCRIPTION\n\
\n\
This command moves the window forward to the next address with a\n\
description.\n\
\n\
");
  } else {
    address* pos;

    confirm();

    pos = w_getaddr(argwinindex);
    if (pos == nil) {
      bufstring("%This window does not have an address.\n");
    } else {
      a_inc(pos, pv_bpa);
      while (mapped(pos) && !d_exist(pos)) {
	a_inc(pos, pv_bpa);
      }
      if (mapped(pos)) {
	w_setaddr(argwinindex, pos);
      } else {
	bufstring("%There is no description from here on.\n");
      }
    }
  }
}

/**********************************************************************/

void wnxt_reference(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT REFERENCE <addr>\n\
\n\
This command moves the window forward to the next object that\n\
references the given address.\n\
\n\
");
  } else {
    address* start;
    address* target;
    int count;

    target = parse_range(nil, nil);
    paecheck();

    start = w_getaddr(argwinindex);
    if (start == nil) {
      bufstring("%This window does not have an address.\n");
    } else {
      setdot(start);
      count = scanref(target);
      if (count > 0) {
	w_setaddr(argwinindex, getdot(1));
      } else {
	bufstring("%There is no reference from here on.\n");
      }
    }
  }
}

/**********************************************************************/

void wnxt_screen(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT SCREEN\n\
\n\
This command moves the window forward one screenful.\n\
\n\
");
  } else {
    confirm();
    w_setnext(argwinindex);
  }
}

/**********************************************************************/

void wnxt_segment(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT SEGMENT\n\
\n\
This command moves the window forward to the beginning of the next\n\
memory segment, if any.\n\
\n\
");
  } else {
    address* addr;
    int segment;

    confirm();

    addr = w_getaddr(argwinindex);
    if (addr == nil) {
      addr = a_zero();
    }
    segment = getsaddr(addr);
    addr = getsfirst(segment + 1);
    if (addr != nil) {
      w_setaddr(argwinindex, addr);
    } else {
      bufstring("%There are no more segments.\n");
    }
  }
}
  
/**********************************************************************/

void wnxt_status(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT STATUS <status code>\n\
\n\
This command moves the window forward to the next address with the\n\
given status.\n\
\n\
");
  } else {
    static fdb statusfdb = { _CMKEY, 0, nil, (pdat) &(statustab), "status, ",
			     nil, nil };
    stcode status;
    address* pos;

    parse(&statusfdb, &parseval, &used);
    status = parseval._pvkey;

    confirm();

    pos = w_getaddr(argwinindex);
    if (pos == nil) {
      bufstring("%This window does not have an address.\n");
    } else {
      a_inc(pos, pv_bpa);
      while (mapped(pos) && (getstatus(pos) != status)) {
	a_inc(pos, pv_bpa);
      }
      if (mapped(pos)) {
	w_setaddr(argwinindex, pos);
      } else {
	bufstring("%There is no data with that status from here on.\n");
      }
    }
  }
}

/**********************************************************************/

void wnxt_unknown(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT UNKNOWN\n\
\n\
This command moves the window forward to the next address with un-\n\
known status.\n\
\n\
");
  } else {
    address* pos;

    confirm();

    pos = w_getaddr(argwinindex);
    if (pos == nil) {
      bufstring("%This window does not have an address.\n");
    } else {
      a_inc(pos, pv_bpa);
      while (mapped(pos) && (getstatus(pos) != st_none)) {
	a_inc(pos, pv_bpa);
      }
      if (mapped(pos)) {
	w_setaddr(argwinindex, pos);
      } else {
	bufstring("%There is no unknown data from here on.\n");
      }
    }
  }
}

/**********************************************************************/

void win_open(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW OPEN <type>\n\
\n\
This command opens a new window of the specified type.  The default\n\
window type is listing.\n\
\n\
");
  } else {
    static keywrd cmds[] = {
      { "counters",   0, (keyval) wty_counters },
      { "dots",       0, (keyval) wty_dots },
      { "dump",       0, (keyval) wty_dump },
      { "highlight",  0, (keyval) wty_highlight },
      { "listing",    0, (keyval) wty_listing },
      { "logger",     0, (keyval) wty_logger },
      { "notes",      0, (keyval) wty_notes },
      { "patterns",   0, (keyval) wty_patterns },
      { "register",   0, (keyval) wty_register },
      { "status",     0, (keyval) wty_status },
      { "symbols",    0, (keyval) wty_symbols },
      { "windows",    0, (keyval) wty_windows },
    };
    static keytab cmdtab = { (sizeof(cmds)/sizeof(keywrd)), cmds };
    static fdb typefdb = { _CMKEY, 0, nil, (pdat) &(cmdtab),
			   "Window type, ", "listing", nil };
    int wintype;

    parse(&typefdb, &parseval, &used);
    wintype = parseval._pvint;

    confirm();
    w_open(wintype);
  }
}

/**********************************************************************/

void win_raise(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] RAISE\n\
\n\
This command puts the window at the top of the stacking order.\n\
\n\
");
  } else {
    confirm();
    w_raise(argwinindex);
  }
}

/**********************************************************************/

void win_segment(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] SEGMENT <number>\n\
\n\
This command sets the window address to the first address of the given\n\
segment.\n\
\n\
");
  } else {
    address* addr;

    addr = getsfirst(parse_number(nil, "number of segment"));
    confirm();

    if (addr != nil) {
      w_setaddr(argwinindex, addr);
    } else {
      bufstring("%That segment does not exist.\n");
    }
  }
}

/**********************************************************************/

/* end of file */
