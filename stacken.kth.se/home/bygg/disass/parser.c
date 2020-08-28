/*
** This module implements the main parser and the main command handling.
*/

#include "comnd.h"
#include "disass.h"

/*
** global variables:
*/

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
void     clear_comments(void);
void     clear_descriptions(void);
void     clear_expansions(void);
void     clear_fields(void);
void     clear_highlights(void);
void     clear_inline_args(void);
void     clear_labels(void);
void     clear_memory(void);
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
void     del_comment(void);
void     del_description(void);
void     del_expansion(void);
void     del_fields(void);
void     del_highlight(void);
void     del_inline_args(void);
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
void     do_purge(void);
void     do_rehash(void);
void     do_test(void);
void cmd_exit(void);
void cmd_follow(void);
void cmd_go(void);
void cmd_help(void);
void     help_address(void);
void     help_comnd(void);
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
void     set_bpl(void);
void     set_branch_target(void);
void     set_case(void);
void     set_charset(void);
void     set_comment(void);
void     set_delimiter(void);
void     set_description(void);
void     set_display(void);
void     set_endian(void);
void     set_expansion(void);
void     set_field(void);
void     set_highlight(void);
void     set_inline_args(void);
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
void     set_standout(void);
void cmd_show(void);
void     show_auto(void);
void     show_character(void);
void     show_comment(void);
void     show_counters(void);
void     show_description(void);
void     show_dots(void);
void     show_expansion(void);
void     show_fields(void);
void     show_inline_args(void);
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
void cmd_unset(void);
void     unset_noreturn(void);
void     unset_register(void);
void     unset_symbol(void);
void cmd_window(void);
void     win_address(void);
void     win_close(void);
void     win_current(void);
void     win_extend(void);
void     win_follow(void);
void     win_lower(void);
void     win_move(void);
void     win_next(void);
void         wnxt_comment(void);
void         wnxt_description(void);
void         wnxt_expansion(void);
void         wnxt_reference(void);
void         wnxt_screen(void);
void         wnxt_segment(void);
void         wnxt_status(void);
void         wnxt_unknown(void);
void     win_open(void);
void     win_raise(void);
void     win_segment(void);
void     win_update(void);
void cmd_write(void);
void cmd_xecute(void);

static cmkeyword cmds[] = {
  { "clea",     KEY_INV+KEY_ABR, "clear" },
  { "clear",    KEY_EMO+KEY_NOC, 0, cmd_clear },
  { "define",   0,       0, cmd_define },
  { "delete",   0,       0, cmd_delete },
  { "disassemble", 0,    0, cmd_disassemble },
  { "do",       0,       0, cmd_do },
  { "exit",	KEY_EMO+KEY_NOC, 0, cmd_exit },
  { "follow",   0,       0, cmd_follow },
  { "go",       0,       0, cmd_go },
  { "help",	0,	 0, cmd_help },
  { "jump",     0,       0, cmd_jump },
  { "list",     0,       0, cmd_list },
  { "locate",   0,       0, cmd_locate },
  { "load",     0,       0, cmd_read },
  { "memory",   0,       0, cmd_memory },
  { "next",     0,       0, cmd_next },
  { "peek",     0,       0, cmd_peek },
  { "quit",	KEY_EMO+KEY_NOC+KEY_INV, 0, cmd_exit },
  { "read",     0,       0, cmd_read },
  { "restore",	0,	 0, cmd_restore },
  { "rpn",      0,       0, cmd_rpn },
  { "s", KEY_INV+KEY_ABR, "set" },
  { "save",     0,       0, cmd_save },
  { "set",      0,       0, cmd_set },
  { "show",     0,       0, cmd_show },
  { "step",     0,       0, cmd_step },
  { "unset",    0,       0, cmd_unset },
  { "w", KEY_INV+KEY_ABR, "window" },
  { "window",   0,       0, cmd_window },
  { "write",    0,       0, cmd_write },
  { "xecute",   0,       0, cmd_xecute },
  { NULL },
};

/* Keyword table for parsing status codes: */

static cmkeyword stats[] = {
  { "none",       0,     (void*) st_none },
  { "asciz",      0,     (void*) st_asciz },
  { "byte",       0,     (void*) st_byte },
  { "character",  0,     (void*) st_char },
  { "double",     0,     (void*) st_double },
  { "float",      0,     (void*) st_float },
  { "instruction", 0,    (void*) st_inst },
  { "jump-slot",  0,     (void*) st_jump },
  { "long",       0,     (void*) st_long },
  { "mask",       0,     (void*) st_mask },
  { "octaword",   0,     (void*) st_octa },
  { "pointer",    0,     (void*) st_ptr },
  { "quadword",   0,     (void*) st_quad },
  { "text",       0,     (void*) st_text },
  { "word",       0,     (void*) st_word },
  { NULL },
};

/*
** Declaration of all the processors we know about:
*/

extern struct entryvector m6502_vector;
extern struct entryvector m65c02_vector;
extern struct entryvector m65c02_rw_vector;
extern struct entryvector m65816_vector;

extern struct entryvector m6800_vector;
extern struct entryvector m6801_vector;
extern struct entryvector m6802_vector;
extern struct entryvector m6803_vector;
extern struct entryvector m6805_vector;
extern struct entryvector m6808_vector;
extern struct entryvector m6809_vector;
extern struct entryvector m6811_vector;

extern struct entryvector m68000_vector;
extern struct entryvector m68010_vector;
extern struct entryvector m68020_vector;
extern struct entryvector m68030_vector;
extern struct entryvector m68040_vector;
extern struct entryvector cpu32_vector;

extern struct entryvector z8_vector;
extern struct entryvector super8_vector;

extern struct entryvector z80_vector;
extern struct entryvector i8080_vector;
extern struct entryvector i8085_vector;
extern struct entryvector h64180_vector;
extern struct entryvector rabbit_vector;

extern struct entryvector i8051_vector;

extern struct entryvector i8086_vector;
extern struct entryvector i8088_vector;
extern struct entryvector i80186_vector;
extern struct entryvector i80286_vector;
extern struct entryvector i80386_vector;
extern struct entryvector i80486_vector;
extern struct entryvector i80486p_vector;

extern struct entryvector x86_64_vector;

extern struct entryvector v25_vector;

extern struct entryvector foo42_vector;

extern struct entryvector hexa_vector;

extern struct entryvector mips_vector;
extern struct entryvector mips64_vector;
extern struct entryvector octeon_vector;

extern struct entryvector pdp10_vector;

extern struct entryvector pdp11_vector;

extern struct entryvector ppc_vector;

extern struct entryvector vax_vector;

static cmkeyword proctab[] = {
  { "64180",   0,        &h64180_vector },
  { "6502",    0,        &m6502_vector },
  { "65c02",   0,        &m65c02_vector },
  { "65c02-rw", 0,       &m65c02_rw_vector },
  { "65816",   0,        &m65816_vector },
  { "6800",    0,        &m6800_vector },
  { "6801",    0,        &m6801_vector },
  { "6802",    0,        &m6802_vector },
  { "6803",    0,        &m6803_vector },
  { "6805",    0,        &m6805_vector },
  { "6808",    0,        &m6808_vector },
  { "6809",    0,        &m6809_vector },
  { "6811",    0,        &m6811_vector },
  { "68000",   0,        &m68000_vector },
  { "68010",   0,        &m68010_vector },
  { "68020",   0,        &m68020_vector },
  { "68030",   0,        &m68030_vector },
  { "68040",   0,        &m68040_vector },
  { "8051",    0,        &i8051_vector },
  { "8080",    0,        &i8080_vector },
  { "8085",    0,        &i8085_vector },
  { "8086",    0,        &i8086_vector },
  { "8088",    0,        &i8088_vector },
  { "80186",   0,        &i80186_vector },
  { "80286",   0,        &i80286_vector },
  { "80386",   0,        &i80386_vector },
  { "80486",   0,        &i80486_vector },
  { "80486-p", 0,        &i80486p_vector },
  { "286",     KEY_INV,  &i80286_vector },
  { "386",     KEY_INV,  &i80386_vector },
  { "486",     KEY_INV,  &i80486_vector },
  { "486-p",   KEY_INV,  &i80486p_vector },
  { "cpu32",   0,        &cpu32_vector },
  { "foo42",   0,        &foo42_vector },
  { "hexa",    0,        &hexa_vector },
  { "mips",    0,        &mips_vector },
  { "mips64",  0,        &mips64_vector },
  { "octeon",  0,        &octeon_vector },
  { "pdp10",   0,        &pdp10_vector },
  { "pdp11",   0,        &pdp11_vector },
  { "ppc",     0,        &ppc_vector },
  { "rabbit",  0,        &rabbit_vector },
  { "super8",  0,        &super8_vector },
  { "v25",     0,        &v25_vector },
  { "vax",     0,        &vax_vector },
  { "x86-64",  0,        &x86_64_vector },
  { "z8",      0,        &z8_vector },
  { "z80",     0,        &z80_vector },
  { NULL },
};

#ifdef NOTDEF

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

#endif

/************************************************************************/

/*
** findproc() is a routine that takes a processor name, and returns
** the entryvector that has a matching name field.  If we can't find
** a matching entryvector, we return NULL.  This is used to restore
** the processor type from saved files.
*/

struct entryvector* findproc(char* name)
{
  struct entryvector* p;
  int i;

  if (name != NULL) {
    for (i = 0; proctab[i].key != NULL; i += 1) {
      p = (struct entryvector*) proctab[i].data;
      if (strcmp(name, p->name) == 0) {
	return p;
      }
    }
  }
  return NULL;
}

/************************************************************************/

static bool done = false;

static char* badlabel = NULL;
static char* badpattern = NULL;

void commandloop(void)
{
  while (!done) {
    buftop();
    cm_prompt("disass> ");
    if (badlabel != NULL) {
      free(badlabel);
      badlabel = NULL;
    }
    if (badpattern != NULL) {
      free(badpattern);
      badpattern = NULL;
    }
    helpflag = false;
    cm_pcmd("Command, ", 0, cmds, KT_MWL);
  }
}

void toploop(void)
{
  m_init();			/* Init memory.c */
  printf("hello, user!  For help type 'HELP'\n");
  commandloop();
}

void bug(char* routine, char* message)
{
  printf("BUG: %s: %s\n", routine, message);
  exit(1);
}

static bool help(void)
{
  if (helpflag) {
    cm_confirm();
  }
  return helpflag;
}

static char* copyatom(char* previous)
{
  if (previous != NULL) {
    free(previous);
  }
  return copystring(atombuffer, NULL);
}

bool dispatch(cmfdb* fdblist)
{
  cmfdb* fdblast;

  fdblast = fdblist;
  while (fdblast->next != NULL) {
    fdblast = fdblast->next;
  }
  if (helpflag) {
    fdblast->next = cm_fdb(_CMCFM, NULL, 0, NULL);
  }
  cm_parse(fdblist);
  if (pval.used == fdblast->next) {
    return false;
  } else {
    cm_dispatch();
    return true;
  }
}

cmfdb* disparg(char* hlp, cmkeyword* tab)
{
  return cm_fdb(_CMKEY, hlp, 0, cm_ktab(tab, KT_MWL));
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
    badpattern = NULL;

    buftop();
    { extern jmp_buf cm_erbuf; longjmp(cm_erbuf, 1); } /* XXXXXX */
  }
  if (badlabel) {
    bufstring("%The label ");
    bufstring(badlabel);
    bufstring(" does not exist.\n");
    free(badlabel);
    badlabel = NULL;

    buftop();
    { extern jmp_buf cm_erbuf; longjmp(cm_erbuf, 1); } /* XXXXXX */
  }
}

/*
** paconfirm() simply does a confirm followed by a call to paecheck().
*/

void paconfirm(void)
{
  cm_confirm();
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

  sprintf(buffer, "%" PRIul, l);
  return buffer;
}

/*
** parse_number() parses a number.  The radix will default to decimal,
** unless the number is prefixed by either "0" or "0x"  in which case
** it will be octal or hexadecimal.
*/

int parse_number(char* helpstring)
{
  cm_pnum(helpstring, NUM_US+NUM_UNIX, 10);
  return (int) pval.num.number;
}

/*
** parse_radix() parses a radix, either as a number (between 2 and 16),
** or as a keyword for the common ones.  The keyword default works, as
** does the number 0, meaning the same thing.  The return value is the
** radix as a number, with 0 meaning default.
*/

int parse_radix(char* helpstring)
{
  static cmkeyword cmd[] = {
    { "default",  0, (void*) 0 },
    { "binary",   0, (void*) 2 },
    { "octal",    0, (void*) 8 },
    { "decimal",  0, (void*) 10 },
    { "hex",      0, (void*) 16 },
    { NULL },
  };

  cm_parse(cm_chain(cm_fdb(_CMKEY, helpstring, 0, cm_ktab(cmd, KT_MWL)),
		    cm_fdb(_CMNUM, NULL, CM_SDH, NULL),
		    NULL));
  if (pval.used->function == _CMKEY)
    return VP2I(pval.kw->data);

  return pval.num.number;
}

/*
** parse_address() parses a (single) address.
*/

address* parse_address(char* dfault, bool primed)
{
  address* addr;

  cmfdb* wtopfdb = cm_fdb(_CMTOK, NULL, CM_SDH, "^");
  cmfdb* addrfdb = cm_fdb(_CMQST, "Address", 0, NULL);
  cmfdb* addr2fdb = cm_fdb(_CMFLD, "Name of label", 0, NULL);

  /* XXX fix addr2fdb to include break set info. How? */

  cm_default(dfault);

  addr = w_getaddr(0);
  if (addr != NULL) {
    cm_parse(cm_chain(wtopfdb, addrfdb, addr2fdb, NULL));
  } else {
    cm_parse(cm_chain(addrfdb, addr2fdb, NULL));
  }
  
  if (pval.used == wtopfdb) {	/* Top of window token? */
    return addr;		/*  Yes. */
  }
  if (pval.used == addrfdb) {	/* Quoted string? */
    addr = l_lookup(atombuffer); /* Yes, try labels first. */
    if (addr == NULL) {
      addr = a_str2a(atombuffer);
    }
  } else {
    addr = a_str2a(atombuffer);	/* No, try parse address first. */
    if (addr == NULL) {
      addr = l_lookup(atombuffer);
    }
  }
  if (addr == NULL) {		/* If we failed, - */
    badlabel = copyatom(badlabel); /* Make a mental note. */
  }
  return addr;			/* Return whatever we found. */
}

/*
** parse_TO() parses the keyword "TO" followed by an address.
** The keyword "TO" should already have been primed into the parser
** by the previous routine.
*/

static cmfdb* fdb_TO(void)
{
  static cmkeyword cmds[] = {
    { "TO", 0, 0 },
  };

  static cmkeytab kt = { 1, cmds };
  static cmfdb f = { _CMKEY, 0, NULL, &kt };
  static cmfdb g = { _CMTOK, 0, NULL, ">" };

  return cm_chain(&g, &f, NULL);

  /*  return cm_fdb(_CMKEY, NULL, CM_SDH, cm_ktab(cmds, 0)); */
}

address* parse_TO(void)
{
  return parse_address(NULL, false);
}

/*
** test: parse addresses according to:
**
**   addr-elem = <single-address>
**   addr-ext  = ":" <number> | "-" addr-elem
**   range     = addr-elem [addr-ext]
**   address   = range [ "," address ]
*/

address* parse_range(char* dfault, cmfdb* nextfield)
{
  static cmfdb hyphfdb =  { _CMTOK, 0, NULL, "-"};
  static cmfdb colonfdb = { _CMTOK, 0, NULL, ":" };
  static cmfdb commafdb = { _CMTOK, 0, NULL, "," };
  static cmfdb cfmfdb =   { _CMCFM };
  
  static address* head = NULL;

  address* tail;

  if (nextfield == NULL) {
    nextfield = &cfmfdb;
  }
  hyphfdb.next = &colonfdb;
  colonfdb.next = &commafdb;
  commafdb.next = nextfield;

  head = a_copy(parse_address(dfault, false), head);
  tail = head;

  for (;;) {
    cm_parse(&hyphfdb);
    if (pval.used == &hyphfdb) {
      a_range(tail, a_copy(parse_address(NULL, false), NULL));
      cm_parse(&commafdb);
    } else if (pval.used == &colonfdb) {
      a_count(tail, parse_number(NULL));
      cm_parse(&commafdb);
    }
    if (pval.used == &commafdb) {
      tail = a_copy(parse_address(NULL, false), NULL);
      a_cons(head, tail);
    } else {			/* next field found. */
      return head;
    }
  }
}

/*
** parse_regrange() parses a subrange (of the format @address) and
** returns a pointer to that address.  If no address is given NULL
** will be returned.  In both cases a confirm will be parsed.
*/

static address* parse_regrange(void)
{
  address* addr;

  cm_parse(cm_chain(cm_fdb(_CMTOK, NULL, 0, "@"),
		    cm_fdb(_CMCFM, NULL, 0, NULL),
		    NULL));

  if (pval.used->function == _CMCFM) {
    return NULL;
  }

  addr = parse_range(NULL, NULL);
  paecheck();
  return addr;
}

/*
** parse_objname() parses a generic name for an object (whatever you
** like) and returns a pointer to a copy of that string.  The string
** is to be used as read-only.
*/

char* parse_objname(char* helpstring)
{
  static char* objname = NULL;

//  cm_pwrd(helpstring, CM_SDH);
  cm_pwrd(helpstring, 0);
  objname = copyatom(objname);
  return objname;
}

/*
** parse_value() parses a value of the specified type.
*/

value* parse_value(int type, char* helpstring)
{
  switch (type) {
  case vty_long:
    return v_l2v(parse_number(helpstring));
  case vty_addr:
    return v_a2v(parse_address(NULL, false));
  }
  return NULL;			/* BUG */
}

/*
** parse_text() parses a line of text, up to but not including the end of
** the line.
*/

char* parse_text(char* helpstring)
{
  static char* txt = NULL;

  cm_parse(cm_chain(cm_fdb(_CMQST, helpstring, 0, NULL),
		    cm_fdb(_CMTXT, NULL, CM_SDH, NULL),
		    NULL));

  txt = copyatom(txt);
  return txt;
}

/*
** parse_pattern() parses a pattern (a series of status codes) and
** returns a pointer to a string of pattern blocks.  If skipflag is
** true our caller has already primed the parser with the first
** status code.  If nextfield is NULL we will parse a simple confirm.
*/

cmfdb* fdb_pattern(void)
{
  static cmkeytab statustab = { (sizeof(stats)/sizeof(cmkeyword)) - 1,
				stats, KT_MWL };
  static cmfdb statusfdb = { _CMKEY, 0, NULL, &statustab, "status, " };
  static cmfdb tokenfdb = { _CMTOK, CM_SDH, NULL, "#",
			    "# to insert a named pattern" };

  return cm_chain(&statusfdb, &tokenfdb, NULL);

  /*
  return cm_chain(cm_fdb(_CMKEY, "status, ", 0, cm_ktab(stats, 0)),
		  cm_fdb(_CMTOK, "# to insert a named pattern", 0, "#"),
		  NULL);
  */
}

pattern* parse_pattern(bool skipfirst, cmfdb* nextfield)
{
  static cmfdb colonfdb = { _CMTOK, 0, NULL, ":" };
  static cmfdb cfmfdb = { _CMCFM };

  static pattern* first = NULL;
  pattern* last;
  pattern* pptr;
  bool done;

  p_free(first);
  first = NULL;
  last = NULL;

  colonfdb.next = &cfmfdb;
  cfmfdb.next = fdb_pattern();

  if (!skipfirst) {
    cm_parse(fdb_pattern());	/* Prime the parser. */
  }

  done = false;

  while (!done) {
    switch (pval.used->function) {
    case _CMCFM:
      done = true;
      break;
    case _CMKEY:		/* Keyword is status code. */
      if (first == NULL) {
	first = p_new();
	last = first;
      } else {
	last->next = p_new();
	last = last->next;
      }
      last->status = VP2I(pval.kw->data);
      last->length = 0;
      cm_parse(&colonfdb);
      if (pval.used == &colonfdb) {
	last->length = parse_number("length in bytes");
	cm_parse(&cfmfdb);
      }
      break;
    case _CMTOK:		/* Token, "pattern" */
      pptr = p_copy(p_read(p_index(parse_objname("name of pattern"))), NULL);
      if (pptr == NULL) {
	badpattern = copyatom(badpattern);
      } else {
	if (first == NULL) {
	  first = pptr;
	  last = first;
	} else {
	  last->next = pptr;
	}
	while (last->next != NULL) {
	  last = last->next;
	}
      }
      cm_parse(&cfmfdb);
      break;
    }
  }
  paecheck();
  return first;
}

/*
** parse_processor() parses the name of a processor (cpu) and returns
** a pointer to the corresponding entryvector.
*/

struct entryvector* parse_processor(char* defaultanswer)
{
  cm_default(defaultanswer);
  cm_pkey("Processor, ", 0, proctab, KT_MWL);
  return (struct entryvector*) pval.kw->data;
}

/*
** redconfirm() parses a confirm, possibly with pipe/file redirection.
*/

cmfdb* fdb_redirect(void)
{

#ifdef NOTDEF
  static cmfdb cfmfdb = { _CMCFM, CM_SDH, NULL, NULL,
			"confirm to use terminal", NULL, NULL };
#endif

  static cmfdb cfmfdb = { _CMCFM };
  static cmfdb redfdb = { _CMTOK, CM_SDH, NULL, ">",
			"\">\" to redirect output to a file", NULL, NULL };
  static cmfdb pipfdb = { _CMTOK, CM_SDH, NULL, "|",
			"\"|\" to redirect output to a pipe", NULL, NULL };

  return cm_chain(&cfmfdb, &redfdb, &pipfdb, NULL);
}

void redconfirm(bool primed)
{
  static cmfdb filfdb = { _CMFIL, 0, NULL, NULL, NULL,
			  "disass.out", NULL };
  static cmfdb cmdfdb = { _CMTXT, CM_SDH, NULL, NULL,
			  "command(s) to pipe output through", NULL, NULL };
  static char* filename = NULL;
  static char* pipecmd = NULL;

  if (!primed) {
    cm_parse(fdb_redirect());
  }

  if (pval.used->function == _CMTOK) {
    switch (((char*)(pval.used->data))[0]) {
    case '>':
      cm_parse(&filfdb);
      filename = copyatom(filename);
      cm_confirm();
      buffile(filename);
      break;
    case '|':
      cm_parse(&cmdfdb);
      pipecmd = copyatom(pipecmd);
      cm_confirm();
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
  static cmkeyword cmds[] = {
    { "comments",     0, 0, clear_comments },
    { "descriptions", 0, 0, clear_descriptions },
    { "expansions",   0, 0, clear_expansions },
    { "fields",       0, 0, clear_fields },
    { "highlights",   0, 0, clear_highlights },
    { "inline-args",  0, 0, clear_inline_args },
    { "labels",       0, 0, clear_labels },
    { "memory",       0, 0, clear_memory },
    { "noreturns",    0, 0, clear_noreturns },
    { "notes",        0, 0, clear_notes },
    { "patterns",     0, 0, clear_patterns },
    { "registers",    0, 0, clear_registers },
    { "status",       0, 0, clear_status },
    { "symbols",      0, 0, clear_symbols },
    { NULL },
  };

  if (!dispatch(disparg("Attribute, ", cmds))) {
    bufstring("\
\n\
The CLEAR command is used to remove all knowledge of certain things.  For\n\
a list of what can be cleared, try 'CLEAR ?'.\n\
\n\
");
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
    cm_confirm();
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
    cm_confirm();
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
    cm_confirm();
    e_clear();
  }
}

/**********************************************************************/

void clear_fields(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR FIELDS\n\
\n\
This command removes all status/radix attributes from the database.\n\
\n\
");
  } else {
    cm_confirm();
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
    cm_confirm();
    hl_clear();
  }
}

/**********************************************************************/

void clear_inline_args(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR INLINE-ARGS\n\
\n\
This command removes all argument lists from the database.\n\
\n\
");
  } else {
    cm_confirm();
    ia_clear();
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
    cm_confirm();
    l_clear();
  }
}

/**********************************************************************/

void clear_memory(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  CLEAR MEMORY\n\
\n\
This command clears (unmaps) all memory.\n\
\n\
");
  } else {
    cm_confirm();
    notyeterror();
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
    cm_confirm();
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
    cm_confirm();
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
    cm_confirm();
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
    cm_confirm();
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
    cm_noise("of all memory");
    cm_confirm();

    m_clear();
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
    cm_confirm();
    s_clear();
  }
}

/**********************************************************************/

void cmd_define(void)
{
  static cmkeyword cmds[] = {
    { "pattern",     0,    0, def_pattern },
    { "register",    0,    0, def_register },
    { "symbol",      0,    0, def_symbol },
    { NULL },
  };

  if (!dispatch(disparg("Attribute, ", cmds))) {
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
    static char* patname = NULL;
    patindex index;
    pattern* pat;

    cm_noise("name");
    (void) parse_objname("name of pattern");
    patname = copyatom(patname);

    cm_noise("pattern");
    pat = parse_pattern(false, NULL);

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
    static cmkeyword cmds[] = {
      { "longword",    0, (void*) vty_long },
      { "address",     0, (void*) vty_addr },
      { NULL },
    };

    char* regname;
    regindex index;
    int type;

    cm_noise("name");
    regname = parse_objname("name of register");

    cm_noise("type");

    cm_default("longword");
    cm_pkey("Type, ", 0, cmds, 0);
    type = VP2I(pval.kw->data);

    cm_confirm();

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

    cm_noise("name");
    symname = parse_objname("name of symbol");

    cm_noise("value");
    symvalue = parse_text("value of symbol");

    cm_confirm();

    s_write(s_define(symname), symvalue);
  }
}

/**********************************************************************/

void cmd_delete(void)
{
  static cmkeyword cmds[] = {
    { "comment",     0,    0, del_comment },
    { "description", 0,    0, del_description },
    { "expansion",   0,    0, del_expansion },
    { "fields",      0,    0, del_fields },
    { "highlight",   0,    0, del_highlight },
    { "inline-args", 0,    0, del_inline_args },
    { "label",       0,    0, del_label },
    { "note",        0,    0, del_note },
    { "pattern",     0,    0, del_pattern },
    { "register",    0,    0, del_register },
    { "symbol",      0,    0, del_symbol },
    { NULL },
  };

  if (!dispatch(disparg("Attribute, ", cmds))) {
    bufstring("\
\n\
The DELETE command is used to remove things like labels, comments, ...\n\
from the database.\n\
\n\
");
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

    cm_noise("at address");
    addr = parse_address(NULL, false);
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

    cm_noise("at address");
    addr = parse_address(NULL, false);
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

    cm_noise("at address");
    addr = parse_address(NULL, false);
    paconfirm();

    e_delete(addr);
  }
}

/**********************************************************************/

void del_fields(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE FIELDS <address>\n\
\n\
This command deletes the status/radix attributes from the specified address.\n\
\n\
");
  } else {
    address* addr;

    cm_noise("at address");
    addr = parse_address(NULL, false);
    paconfirm();

    f_delete(addr);
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

    index = parse_number("highlight number");

    cm_confirm();

    hl_delete(index);
    bufstring("Highlight point number ");
    bufnumber(index);
    bufstring(" is no more.\n");
  }
}

/**********************************************************************/

void del_inline_args(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  DELETE INLINE-ARGS <address>\n\
\n\
This command deletes the in-line argument list for the routine at the\n\
given address.\n\
\n\
");
  } else {
    address* addr;

    cm_noise("at address");
    addr = parse_address(NULL, false);
    paconfirm();

    ia_delete(addr);
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

    cm_noise("at address");
    addr = parse_address(NULL, false);
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

    index = parse_number("note number");

    cm_confirm();

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

    cm_noise("name");
    name = parse_objname("name of pattern");
    cm_confirm();
    
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
    static char* regname = NULL;
    regindex index;

    cm_pwrd("name of register", 0);
    regname = copyatom(regname);

    cm_confirm();

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

    cm_noise("name");
    name = parse_objname("name of symbol");
    cm_confirm();
    
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
    static cmkeyword formats[] = {
      { "listing", 0, (void*) EX_LIST },
      { "source",  0, (void*) EX_ASM },
      { NULL },
    };
    cmfdb* fmtfdb = cm_fdb(_CMKEY, "output format, ", 0, cm_ktab(formats, 0));
			  
    int format;
    static address* pos = NULL;
    int segment, maxseg;

    format = EX_ASM;

    fmtfdb->next = fdb_redirect();

    cm_default("source");
    cm_parse(fmtfdb);
    if (pval.used == fmtfdb) {
      format = VP2I(pval.kw->data);
      redconfirm(false);
    } else {
      redconfirm(true);
    }

    if (pcheck()) {
      if (format == EX_ASM) {
	genbegin();
      }
      maxseg = getscount();
      for (segment = 1; segment <= maxseg; segment += 1) {
	pos = a_copy(getsfirst(segment), pos);
	if (format == EX_ASM) {
	  genorg(pos);
	}
	while (mapped(pos)) {
	  peek(pos, format, st_none);
	  a_inc(pos, pb_length * pv_bpa);
	  if (bufquit()) break;
	}
      }
      if (format == EX_ASM) {
	genend();
      }
    }
    bufclose();
  }
}

/**********************************************************************/

void cmd_do(void)
{
  static cmkeyword cmds[] = {
    { "auto",          0, 0, do_auto },
    { "count",         0, 0, do_count },
    { "dots",          0, 0, do_dots },
    { "nothing",       0, 0, do_nothing },
    { "purge",         0, 0, do_purge },
    { "rehash",        0, 0, do_rehash },
    { "test",          0, 0, do_test },
    { NULL },
  };

  if (!dispatch(disparg("Function, ", cmds))) {
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

    cm_confirm();

    if (pcheck()) {
      addr = autolist();
      if (addr == NULL) {
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
    cm_confirm();
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
    cm_confirm();

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
    cm_confirm();
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
    cm_confirm();
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
    cm_confirm();
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
    cm_confirm();
    a_test();			/* addr.c test function. */
    w_test();			/* xwin.c test function. */
    m_test();			/* memory.c test function. */
    com_test();			/* common.c test function. */
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
    cm_noise("this program");
    cm_confirm();
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
    static address* pos = NULL;
    int count;

    cm_noise("code at");

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
    static address* pos = NULL;
    int count;

    cm_noise("to code at");

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
    static cmkeyword concepts[] = {
      { "address",     0, 0, help_address },
      { "comnd",       0, 0, help_comnd },
      { "description", 0, 0, help_description },
      { "expansion",   0, 0, help_expansion },
      { "pattern",     0, 0, help_pattern },
      { "processor",   0, 0, help_processor },
      { "status",      0, 0, help_status },
      { NULL },
    };

    cm_parse(cm_chain(cm_fdb(_CMKEY, "Command, ", 0, cm_ktab(cmds, KT_MWL)),
		      cm_fdb(_CMKEY, "Concept, ", 0, cm_ktab(concepts,KT_MWL)),
		      cm_fdb(_CMCFM, NULL, 0, NULL),
		      NULL));

    if (pval.used->function == _CMCFM) { /* Command = HELP <return> */
      bufstring("first level help text.\n");
    } else {
      helpflag = true;
      cm_dispatch();
    }
  }
}

/**********************************************************************/

void help_address(void)
{
  cm_confirm();
  bufstring("Help text, address syntax.\n");
}

/**********************************************************************/

void help_comnd(void)
{
  cm_confirm();
  bufstring("Help text elaborating on the comnd system.\n");
}

/**********************************************************************/

void help_description(void)
{
  cm_confirm();
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
  cm_confirm();
  bufstring("\
\n\
Help text elaborating on expansions, and what they are.\n\
\n\
");
}

/**********************************************************************/

void help_pattern(void)
{
  cm_confirm();
  bufstring("\
\n\
Help text elaborating on patterns.\n\
\n\
");
}

/**********************************************************************/

void help_processor(void)
{
  static cmkeyword keys[] = {
    { "general",     0, (void*) hty_general },
    { "registers",   0, (void*) hty_registers },
    { "syntax",      0, (void*) hty_syntax },
    { NULL },
  };
  struct entryvector* p;
  int helptype;

  p = parse_processor(processor != NULL? processor->name: NULL);

  cm_default("general");
  cm_pkey(NULL, 0, keys, 0);
  helptype = VP2I(pval.kw->data);

  cm_confirm();

  bufnewline();
  if (!(*p->help)(helptype)) {
    bufstring("%That type of help seems to be missing.\n");
  }
  bufnewline();
}

/**********************************************************************/

void help_status(void)
{
  stcode status;
  
  cm_parse(cm_chain(cm_fdb(_CMKEY, "status, ", 0, cm_ktab(stats, 0)),
		    cm_fdb(_CMCFM, NULL, 0, NULL),
		    NULL));
  if (pval.used->function == _CMCFM) {
    bufstring("\
\n\
Help text elaborating on the concept of status codes.\n\
\n\
");
  } else {
    status = VP2I(pval.kw->data);
    cm_confirm();
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
    case st_jump:
      bufstring("\
\n\
JUMP-SLOT is code, but with the additional twist that it is supposed to\n\
be the last delay slot after a jump instruction.\n\
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
    case st_asciz:
      bufstring("\
\n\
ASCIZ is null-terminated ascii data, and can be of any length.\n\
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

    addr = parse_address(NULL, false);
    paconfirm();

    setdot(addr);
  }
}

/**********************************************************************/

void cmd_list(void)
{
  static cmkeyword cmds[] = {
    { "cpus",          0, 0, list_processors },
    { "highlights",    0, 0, list_highlights },
    { "notes",         0, 0, list_notes },
    { "patterns",      0, 0, list_patterns },
    { "processors",    0, 0, list_processors },
    { "registers",     0, 0, list_registers },
    { "symbols",       0, 0, list_symbols },
    { "windows",       0, 0, list_windows },
    { NULL },
  };

  if (!dispatch(disparg("Item, ", cmds))) {
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
    for (i = 0; proctab[i].key != NULL; i += 1) {
      if (proctab[i].flags & KEY_INV) {
	continue;
      }
      p = (struct entryvector*) proctab[i].data;
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
      if (r_isdef(index, NULL)) {
	bufstring(", default value=");
	bufvalue(r_read(index, NULL));
      }
      bufnewline();

      subrange = r_subrange(index, NULL);
      while (subrange != NULL) {
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
  static cmkeyword cmds[] = {
    { "next",          0, 0, loc_next },
    { NULL },
  };
  if (!dispatch(disparg("Item, ", cmds))) {
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
  static cmkeyword cmds[] = {
    { "reference",   0, 0, lnxt_reference },
   /*
    { "comment",     0, 0, lnxt_comment },
    { "description", 0, 0, lnxt_description },
    { "segment",     0, 0, lnxt_segment },
    { "status",      0, 0, lnxt_status },
    { "unknown",     0, 0, lnxt_unknown },
   */
    { NULL },
  };
  if (!dispatch(disparg("Item, ", cmds))) {
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

    pos = parse_range(NULL,  NULL);
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
  static cmkeyword cmds[] = {
    { "copy",     0, 0, mem_copy },
    { "exclude",  0, 0, mem_exclude },
    { "include",  0, 0, mem_include },
    { "move",     0, 0, mem_move },
    { "relocate", 0, 0, mem_relocate },
    { "s", KEY_INV+KEY_ABR, "set" },
    { "set",      0, 0, mem_set },
    { "shuffle",  0, 0, mem_shuffle },
    { "xor",      0, 0, mem_xor },
    { NULL },
  };

  if (!dispatch(disparg("Command, ", cmds))) {
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
    static address* srcaddr = NULL;
    static address* dstaddr = NULL;
    
    cm_noise("from");
    srcaddr = a_copy(parse_range(NULL, fdb_TO()), srcaddr);
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

    addr = parse_range(NULL, NULL);
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
    
    addr = parse_range(NULL, NULL);
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
The difference from the MEMORY COPY command is that moving memory will\n\
remove (unmap) the old addresses.\n\
\n\
");
  } else {
    static address* srcaddr = NULL;
    static address* dstaddr = NULL;

    cm_noise("from");
    srcaddr = a_copy(parse_range(NULL, fdb_TO()), srcaddr);
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
    static address* srcaddr = NULL;
    static address* dstaddr = NULL;

    cm_noise("from");
    srcaddr = a_copy(parse_range(NULL, fdb_TO()), srcaddr);
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
Syntax:  MEMORY SET <address> <data> [<data...>]\n\
    or:  SET MEMORY <address> <data> [<data...>]\n\
\n\
The address is a single address.  The data can be either a single numeric\n\
value (prefixed with 0x, 0b or 0 to set the radix) or a quoted string.\n\
\n\
Example:  'set memory 10 \"Hello, world!\" 13 10 0' will set the sixteen\n\
octets from address 10 and onward to the string, carriage return, line-\n\
feed and terminating null.\n\
\n\
");
  } else {
    static address* addr = NULL;
    byte buf[256];
    int pos = 0;
    int val;
    cmfdb* f;
    int i;
    
    addr = a_copy(parse_address(addr != NULL? a_a2str(addr) : NULL, false),
		  addr);
    cm_noise("value");

    cm_default("0");
    for (;;) {
      f = cm_chain(cm_fdb(_CMCFM, NULL, 0, NULL),
		   cm_fdb(_CMNUM, "byte to set",
			  NUM_US+NUM_UNIX, (void*) 10),
		   cm_fdb(_CMQST, "text string", 0, NULL),
		   NULL);
      if (pos == 0) {
	f = f->next;
      }
      cm_parse(f);
      if (pval.used->function == _CMCFM)
	break;
      if (pval.used->function == _CMQST) {
	for (i = 0; atombuffer[i] != 0; i++) {
	  if (pos < 256) {
	    buf[pos++] = atombuffer[i];
	  }
	}
      } else {
	val = (int) pval.num.number;
	if (pos < 256) {
	  buf[pos++] = val;
	}
      }
    }

    paecheck();
    m_set(addr, buf, pos);
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
    cm_confirm();
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
    cm_confirm();
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
    static address* pos = NULL;
    address* defpeek;
    int count;
    stcode format;

    defpeek = getdot(1);

    if (defpeek == NULL) {
      defpeek = getsfirst(1);
    }
    cm_noise("at");
    pos = a_copy(parse_address(a_a2str(defpeek), false), pos);

    cm_noise("format");

    cm_default(peekformat);
    cm_pkey("format, ", 0, stats, 0);
    format = VP2I(pval.kw->data);

    sprintf(peekformat, "%s", atombuffer);

    cm_noise("count");
    cm_default("20");
    count = parse_number("number of objects");

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

bool cmd_ld_bin(char* filename)
{
  enum {
    opt_addr,
    opt_even,
    opt_odd,
    opt_skip,
  };

  static cmkeyword cmds[] = {
    { "address", 0, (void*) opt_addr, NULL, "address to load at" },
    { "even",    0, (void*) opt_even, NULL, "load even bytes" },
    { "odd",     0, (void*) opt_odd, NULL,  "load odd bytes" },
    { "o", KEY_INV+KEY_ABR, "odd" },
    { "offset",  KEY_INV, (void*) opt_addr, NULL, NULL },
    { "skip",    0, (void*) opt_skip, NULL, "initial file bytes to skip" },
    { NULL },
  };

  int fileoffset = 0;
  static address* addr = NULL;
  int evenodd = 0;

  addr = a_copy(a_zero(), addr);
  for (;;) {
    cm_parse(cm_chain(cm_fdb(_CMKEY, "Option, ", 0, cm_ktab(cmds, 0)),
		      cm_fdb(_CMCFM, "confirm to load file", 0, NULL),
		      NULL));
    if (pval.used->function == _CMCFM) {
      paecheck();
      return load_binary(filename, addr, fileoffset, evenodd);
    }
    switch (VP2I(pval.kw->data)) {
    case opt_addr:
      addr = a_copy(parse_address("0", false), addr);
      break;
    case opt_even:
      evenodd = 2;
      break;
    case opt_odd:
      evenodd = 1;
      break;
    case opt_skip:
      cm_default("0");
      fileoffset = parse_number("offset in file");
      break;
    }
  }
}

enum {
  fmt_aout,
  fmt_binary,
  fmt_elf,
  fmt_hex,
  fmt_motorola,
  fmt_shf,
  fmt_tek,
};

void cmd_read(void)
{
  static cmkeyword cmds[] = {
    { "aout",       0, (void*) fmt_aout,     NULL,  "a.out" },
    { "binary",     0, (void*) fmt_binary,   NULL,  "plain binary" },
    { "elf",        0, (void*) fmt_elf,      NULL,  "elf" },
    { "hex",        0, (void*) fmt_hex,      NULL,  "intel hex records" },
    { "motorola",   0, (void*) fmt_motorola, NULL,  "motorola S records" },
    { "shf",        0, (void*) fmt_shf,      NULL,  "S hexdump format" },
    { "tektronix",  0, (void*) fmt_tek,      NULL,  "tektronix hex records" },
    { NULL },
  };

  static char* filename = NULL;
  int format;
  bool ok;
  
  ok = false;			/* Needed to make gcc shut up. */

  if (!helpflag) {
    cm_pfil("Input file", 0);
    filename = copyatom(filename);
  }

  cm_noise("format");

  if (helpflag) {
    cm_parse(cm_chain(cm_fdb(_CMKEY, "File format, ", 0, cm_ktab(cmds, 0)),
		      cm_fdb(_CMCFM, NULL, 0, NULL),
		      NULL));
  } else {
    cm_default("binary");
    cm_pkey("File format, ", 0, cmds, 0);
  }
  if (pval.used->function == _CMCFM) {
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
  format = VP2I(pval.kw->data);

  if (help()) {
    switch (format) {
    case fmt_aout:
      bufstring("\
\n\
Syntax:  READ <filename> AOUT\n\
    or:  LOAD <filename> AOUT\n\
\n\
This command reads a file in a.out format.\n\
\n\
");
      break;
    case fmt_binary:
      bufstring("\
\n\
Syntax:  READ <filename> BINARY [options]\n\
    or:  LOAD <filename> BINARY [options]\n\
\n\
This command reads a plain file, and puts it in memory sequentially.\n\
There are a handful of options that can be specified:\n\
\n\
    ADDRESS <address> -- load the file at the specified offset in memory\n\
                         The default address is zero.\n\
    EVEN              -- load the bytes from the file into consecutive\n\
                         even addresses, setting the corresponding odd\n\
                         locations to zero if unmapped.\n\
    ODD               -- as EVEN, but load the odd addresses.\n\
    SKIP <no. bytes>  -- skip the given number of intial bytes from the\n\
                         file.  Default is 0.  This can be used to skip\n\
                         over file headers etc.\n\
\n\
");
      break;
    case fmt_elf:
      bufstring("\
\n\
Syntax:  READ <filename> ELF\n\
    or:  LOAD <filename> ELF\n\
\n\
This command reads a file in elf format.\n\
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
    case fmt_shf:
      bufstring("\
\n\
Syntax:  READ <filename> SHF\n\
    or:  LOAD <filename> SHF\n\
\n\
This command reads a file in S Hexdump Format.\n\
\n\
");
      break;
    case fmt_tek:
      bufstring("\
\n\
Syntax:  READ <filename> TEKTRONIX\n\
    or:  LOAD <filename> TEKTRONIX\n\
\n\
This command reads a file with tektronix hex-records.\n\
\n\
");
      break;
    }
    return;
  } else {
    switch (format) {
    case fmt_aout:
      cm_confirm();
      ok = load_aout(filename);
      break;
    case fmt_binary:
      ok = cmd_ld_bin(filename); /* Uses subroutine, can have args. */
      break;
    case fmt_elf:
      cm_confirm();
      ok = load_elf(filename);
      break;
    case fmt_hex:
      cm_confirm();
      ok = load_intel(filename);
      break;
    case fmt_motorola:
      cm_confirm();
      ok = load_motorola(filename);
      break;
    case fmt_shf:
      cm_confirm();
      ok = load_shf(filename);
      break;
    case fmt_tek:
      cm_confirm();
      ok = load_tektronix(filename);
      break;
    }
  }
  if (ok) {
    bufstring("OK.\n");
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
    static char* filename = NULL;

    cm_noise("status from");

    cm_default(defsavfile);
    cm_pfil("File to restore", 0);
    filename = copyatom(filename);

    cm_confirm();

    sprintf(defsavfile, "%s", filename);
    m_restore(filename);
    wc_total();
  }
}

/**********************************************************************/

void cmd_save(void)
{
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
    static char* filename = NULL;

    cm_noise("status in");

    cm_default(defsavfile);
    cm_pfil("File to save in", 0);
    filename = copyatom(filename);

    cm_confirm();
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
  static cmkeyword cmd1[] = {
    { "comment",     0,  0, set_comment },
    { "description", 0,  0, set_description },
    { "expansion",   0,  0, set_expansion },
    { "field",       0,  0, set_field },
    { "inline-args", 0,  0, set_inline_args },
    { "label",       0,  0, set_label },
    { "noreturn",    0,  0, set_noreturn },
    { "status",      0,  0, set_status },
    { NULL },
  };

  static cmkeyword cmd2[] = {
    { "cpu",         0,  0, set_processor },
    { "highlight",   0,  0, set_highlight },
    { "memory",      0,  0, mem_set },
    { "processor",   0,  0, set_processor },
    { "register",    0,  0, set_register },
    { "symbol",      0,  0, def_symbol },
    { NULL },
  };

  static cmkeyword cmd3[] = {
    { "bpl",         0,  0, set_bpl },
    { "branch_target",0, 0, set_branch_target },
    { "case",        0,  0, set_case },
    { "charset",     0,  0, set_charset },
    { "delimiter",   0,  0, set_delimiter },
    { "display",     0,  0, set_display },
    { "endian",      0,  0, set_endian },
    { "note",        0,  0, set_note },
    { "objtype",     0,  0, set_objtype },
    { "pager",       0,  0, set_pager },
    { "radix",       0,  0, set_radix },
    { "standout",    0,  0, set_standout },
    { "syntax",      0,  0, set_syntax },
    { NULL },
  };

  if (!dispatch(cm_chain(disparg("Attribute, ", cmd1),
			 disparg("Keyword, ", cmd2),
			 disparg("Variable, ", cmd3),
			 NULL))) {
    bufstring("\
\n\
help text for the SET command.\n\
\n\
");
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

    cm_noise("at address");
    addr = parse_address(NULL, false);

    cm_noise("comment");
    commstr = parse_text("comment string");

    paconfirm();

    c_insert(addr, commstr);
  }
}

/**********************************************************************/

static char* texti(char* help, char* prevtext)
{
  static char buf[1000];
  char* store;
  int count;
  int i;
  char c;

  store = buf;
  count = 0;

  printf("End with a single '.'\n");

  for (;;) {
    cm_prompt(">");
    cm_ptxt(NULL, 0);
    cm_confirm();

    if (atombuffer[0] == '.' && atombuffer[1] == 0)
      break;

    i = 0;
    while ((c = atombuffer[i++]) != 0 && ++count < 999) {
      *store++ = c;
    }
    *store++ = '\n';
  }

  *store = 0;
  return copystring(buf, NULL);
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
    static char* descstr = NULL;
    address* addr;

    cm_noise("at address");
    addr = parse_address(NULL, false);
    cm_noise("description");

    cm_parse(cm_chain(cm_fdb(_CMCFM, NULL, 0, NULL),
		      cm_fdb(_CMTXT, NULL, 0, NULL),
		      NULL));
    if (descstr) {
      free(descstr);
      descstr = NULL;
    }
    if (pval.used->function == _CMTXT) {
      descstr = copystring(atombuffer, NULL);
      paconfirm();
    } else {
      paecheck();
      descstr = texti("Description", NULL);
    }
    if (descstr != NULL) {
      d_insert(addr, descstr);
      free(descstr);
      descstr = NULL;
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
    static address* addr = NULL;
    char* expstr;
    int explen;

    cm_noise("at address");
    addr = a_copy(parse_address(NULL, false), addr);

    cm_noise("length");

    explen = 1;
    if (addr != NULL) {
      bufshutup(true);
      peek(addr, 0, st_none);
      explen =  pb_length;
    }
    cm_default(l2s(explen));
    explen = parse_number("length of expansion");

    cm_noise("expansion");
    expstr = parse_text("expansion string");

    paconfirm();

    e_insert(addr, expstr, explen);
  }
}

/**********************************************************************/

void set_field(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET FIELD <address>/<pos> <arg>\n\
\n\
yadda yadda yadda...\n\
\n\
");
  } else {

    static cmkeyword cmd[] = {
      { "length",   0, (void*) FLD_LENGTH },
      { "radix",    0, (void*) FLD_RADIX },
      { "s", KEY_INV+KEY_ABR, "status" },
      { "sign",     0, (void*) FLD_SIGN },
      { "status",   0, (void*) FLD_STATUS },
      { NULL },
    };

    static cmkeyword signs[] = {
      { "default",  0, (void*) SIGN_DEFAULT },
      { "signed",   0, (void*) SIGN_SIGNED },
      { "unsigned", 0, (void*) SIGN_UNSIGNED },
      { NULL },
    };

    address* addr;
    int pos;
    int type;
    int arg;

    arg = 0;			/* Needed to make gcc shut up. */

    cm_noise("at address");
    addr = parse_address(NULL, false);

    cm_ptok(NULL, 0, "/");
    pos = parse_number("attribute position");

    cm_pkey("field type, ", 0, cmd, KT_MWL);
    type = VP2I(pval.kw->data);

    switch (type) {
    case FLD_LENGTH:
      arg = parse_number("field length");
      break;
    case FLD_RADIX:
      arg = parse_radix("field radix, ");
      break;
    case FLD_SIGN:
      cm_pkey("field signedness, ", 0, signs, KT_MWL);
      arg = VP2I(pval.kw->data);
      break;
    case FLD_STATUS:
      cm_pkey("field status, ", 0, stats, KT_MWL);
      arg = VP2I(pval.kw->data);
      break;
    }

    paconfirm();

    f_write(addr, pos, type, arg);
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

    cm_noise("at address");
    addr = parse_range(NULL, NULL);
    paecheck();
    hl_write(addr);
  }
}

/**********************************************************************/

void set_inline_args(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET INLINE-ARGS <address> <pattern>\n\
\n\
This command sets a list of inline argument types for a given function\n\
or procedure at the given address.\n\
\n\
");
  } else {
    address* addr;
    pattern* pat;

    cm_noise("at address");
    addr = parse_address(NULL, false);

    cm_noise("pattern");
    pat = parse_pattern(false, NULL);

    /* parse_pattern() does confirm and paecheck for us. */

    ia_write(addr, pat);
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
    static char* labelstr = NULL;
    address* addr;

    cm_noise("at");
    addr = parse_address(NULL, false);

    cm_noise("label");

    /* XXX set up break set? */

    cm_parse(cm_chain(cm_fdb(_CMCFM, "Confirm to generate a label", 0, NULL),
		      cm_fdb(_CMQST, NULL, CM_SDH, NULL),
		      cm_fdb(_CMFLD, "Name of label", 0, NULL),
		      NULL));
    if (pval.used->function == _CMCFM) {
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

    cm_noise("at address");
    addr = parse_address(NULL, false);

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

    cm_noise("value");
    val = parse_value(type, "value of register");

    addr = parse_regrange();

    paecheck();			/* Check for bad labels. */

    if (index == 0) {
      index = r_define(regname, vty_long);
    }
    r_write(index, addr, val);
  }
}

/**********************************************************************/

void set_bpl(void)
{
  if (help()) {
    bufstring("\
\n\
The SET BPL command sets the number of bytes/line of expanded data\n\
that is printed before each expanded item.\n\
\n\
");
  } else {
    int bpl;

    cm_default(l2s(pv_bpl));

    cm_pnum("bytes/line", NUM_US, 10);
    bpl = (int) pval.num.number;

    cm_confirm();

    if (bpl > 1 && bpl <= 16) {
      pv_bpl = bpl;
      wc_code();
    }
  }
}

/**********************************************************************/

void set_branch_target(void)
{
  if (help()) {
    bufstring("\
\n\
The SET BRANCH-TARGET command sets the mode used for outputting\n\
unlabelled target addresses for relative branches.\n\
\n\
");
  } else {
    static cmkeyword cmd[] = {
      { "default",   0, NULL },
      { "absolute",  0, "absolute" },
      { "relative",  0, "relative" },
      { NULL },
    };
    char* val;

    cm_default("relative");
    cm_pkey("branch target type, ", 0, cmd, 0);
    val = pval.kw->data;
    cm_confirm();

    s_write(s_define("branch-target"), val);
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
    static cmkeyword cmd[] = {
      { "default",  0, NULL,      0, "default case for this processor" },
      { "lower",    0, "lower",   0, "lower case" },
      { "initial",  0, "initial", 0, "Initial Case" },
      { "upper",    0, "upper",   0, "UPPER CASE" },
      { NULL },
    };
    char* val;

    cm_default("default");
    cm_pkey("case, ", 0, cmd, 0);
    val = pval.kw->data;
    cm_confirm();

    s_write(s_define("casing"), val);
  }
}

/**********************************************************************/

/* FIXME */

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
    static cmkeyword cmd[] = {
      { "default",  0, 0 },
      { NULL },
    };

    cm_default("default");
    cm_pkey("Character set, ", 0, cmd, 0);
    cm_confirm();
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
    static cmkeyword cmd[] = {
      { "default",  0, NULL },
      { "space",    0, " " },
      { "tab",      0, "\t" },
      { NULL },
    };

    char* val;

    cm_default("default");
    cm_pkey("Delimiter, ", 0, cmd, 0);
    val = pval.kw->data;

    cm_confirm();

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

    cm_noise("to");
    display = parse_text("display");
    cm_confirm();
    sy_setenv("DISPLAY", display);
  }
}

/**********************************************************************/

void set_endian(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET ENDIAN [BIG | LITTLE]\n\
\n\
This command sets the endianness flag.\n\
\n\
");
  } else {
    static cmkeyword endian[] = {
      { "big",    0, (void*) true },
      { "little", 0, (void*) false },
      { NULL },
    };
    bool flag;
    
    cm_default(pv_bigendian? "big" : "little");
    cm_pkey("endianness, ", 0, endian, 0);
    flag = (bool) VP2I(pval.kw->data);
    cm_confirm();

    pv_bigendian = flag;
    wc_code();
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

    cm_noise("to");
    note = parse_text("note to make");
    cm_confirm();
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
    stcode status;

    cm_noise("to");
    cm_pkey("status, ", 0, stats, 0);
    status = VP2I(pval.kw->data);
    cm_confirm();

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

    cm_noise("to");
    pager = parse_text("pager");
    cm_confirm();
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
    char buffer[20];
    int radix;

    cm_default("default");
    radix = parse_radix("Global radix, ");

    cm_confirm();

    sprintf(buffer, "%u", radix);
    s_write(s_define("radix"), buffer);
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
    static cmkeyword cmd[] = {
      { "default",  0, NULL },
      { "native",   0, "native" },
      { "foreign",  0, "foreign" },
      { "unix",     0, "unix" },
      { NULL },
    };
    char* val;

    cm_default("default");
    cm_pkey("Syntax, ", 0, cmd, 0);
    val = pval.kw->data;

    cm_confirm();

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

    cm_noise("to");
    p = parse_processor(NULL);

    cm_confirm();

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

    cm_noise("at address");
    addr = parse_range(NULL, fdb_pattern());
    
    pstart = parse_pattern(true, NULL);
    paecheck();

    while (addr != NULL) {
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
	  if (pat == NULL) {
	    pat = pstart;
	  }
	  count -= size;
	}
      } else {
	while (pat != NULL) {
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

void set_standout(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SET STANDOUT COLOR <name of color>\n\
    or:  SET STANDOUT INVERSE\n\
\n\
This sets the way highlighting is done.  Setting INVERSE does it with\n\
white text on grey background, setting a specific color does it with\n\
that color on white.\n\
\n\
");
  } else {
    static cmkeyword cmds[] = {
      { "color",    0, (void*) 0 },
      { "inverse",  0, (void*) 1 },
      { NULL },
    };
    char* val;
    int mode;

    cm_pkey("standout mode, ", 0, cmds, KT_MWL);
    mode = VP2I(pval.kw->data);
    if (mode == 0) {
      val = parse_text("color name");
      cm_confirm();
      w_setcolor(val);
    } else {
      cm_confirm();
      w_setinverse();
    }
  }
}

/**********************************************************************/

void cmd_show(void)
{
  static cmkeyword cmd1[] = {
    { "comment",     0, 0, show_comment },
    { "description", 0, 0, show_description },
    { "expansion",   0, 0, show_expansion },
    { "fields",      0, 0, show_fields },
    { "inline-args", 0, 0, show_inline_args },
    { "label",       0, 0, show_label },
    { "noreturn",    0, 0, show_noreturn },
    { "status",      0, 0, show_status },
    { NULL },
  };

  static cmkeyword cmd2[] = {
    { "auto",      0,   0, show_auto },
    { "character", 0,   0, show_character },
    { "counters",  0,   0, show_counters },
    { "cpu",       0,   0, show_processor },
    { "dots",      0,   0, show_dots },
    { "memory",    0,   0, show_memory },
    { "note",      0,   0, show_note },
    { "pattern",   0,   0, show_pattern },
    { "processor", 0,   0, show_processor },
    { "register",  0,   0, show_register },
    { "symbol",    0,   0, show_symbol },
    { NULL },
  };

  if (!dispatch(cm_chain(disparg("Attribute, ", cmd1),
			 disparg("Keyword, ", cmd2),
			 NULL))) {
    bufstring("\
\n\
help text for the SHOW command.\n\
\n\
");
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

    cm_confirm();

    if (pcheck()) {
      addr = autolist();
      if (addr == NULL) {
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

    n = (parse_number(NULL) & 0xff);

    cm_confirm();

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
    static address* pos = NULL;
    int i;
    int segment, maxseg;
    stcode status, prevstat;
    int cc, dc, ec, lc;

    int bcount[16] = { 0 };
    int icount[16] = { 0 };

    cm_confirm();

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
	  case st_jump:   bufstring("jump-slots:   "); break;
	  case st_byte:   bufstring("bytes:        "); break;
	  case st_word:   bufstring("words:        "); break;
	  case st_long:   bufstring("longwords:    "); break;
	  case st_quad:   bufstring("quadwords:    "); break;
	  case st_octa:   bufstring("octawords:    "); break;
	  case st_char:   bufstring("characters:   "); break;
	  case st_text:   bufstring("texts:        "); break;
	  case st_asciz:  bufstring("asciz texts:  "); break;
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

    cm_noise("of address");
    addr = parse_address(NULL, false);
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

    cm_noise("of address");
    addr = parse_address(NULL, false);
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
    cm_confirm();

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

    cm_noise("of address");
    addr = parse_address(NULL, false);
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

void show_fields(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW FIELDS <address>\n\
\n\
This command shows the status/radix attributes on a certain address.\n\
\n\
\n\
");
  } else {
    int pos;
    address* addr;
    pattern* fields;

    cm_noise("of address");
    addr = parse_address(NULL, false);
    paconfirm();

    fields = f_read(addr);

    if (fields == NULL) {
      bufstring("%There are no fields defined at that address.\n");
    } else {
      for (pos = 1; fields != NULL; fields = fields->next, pos++) {
	/* pos n: status <stcode>, radix <radix> [, [un]signed] */

	bufaddress(addr);
	bufchar('/');
	bufnumber(pos);
	bufstring(": status ");
	bufchar(st2char(fields->status));
	bufstring(", radix ");
	bufnumber(fields->radix);
	bufstring(", length ");
	bufnumber(fields->length);
	switch (fields->sign) {
	case SIGN_SIGNED:
	  bufstring(", signed");
	  break;
	case SIGN_UNSIGNED:
	  bufstring(", unsigned");
	  break;
	}
	bufnewline();
      }
    }
  }
}

/**********************************************************************/

void show_inline_args(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW INLINE-ARGS <address>\n\
\n\
This command shows the inline argument list for an address.\n\
\n\
");
  } else {
    address* addr;
    pattern* pat;

    cm_noise("of address");
    addr = parse_address(NULL, false);
    paconfirm();

    pat = ia_read(addr);

    if (pat == NULL) {
      bufstring("There is no argument list for that address.");
    }
    while (pat != NULL) {
      switch (pat->status) {
	case st_none:   bufstring("none");   break;
	case st_cont:   bufstring("cont");   break;
        case st_asciz:  bufstring("asciz");  break;
	case st_byte:   bufstring("byte");   break;
	case st_char:   bufstring("char");   break;
	case st_double: bufstring("double"); break;
	case st_float:  bufstring("float");  break;
	case st_inst:   bufstring("inst");   break;
        case st_jump:   bufstring("jump");   break;
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

void show_label(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  SHOW LABEL <label>\n\
\n\
This command simply shows the value of a specified label.\n\
\n\
");
  } else {
    address* a;
    char* l;

    /* XXX label break set? how? */

    cm_parse(cm_chain(cm_fdb(_CMFLD, "Name of label", 0, NULL),
		      cm_fdb(_CMQST, NULL, CM_SDH, NULL),
		      NULL));
    l = atombuffer;
    cm_confirm();

    bufstring("Label ");
    bufstring(l);
    a = l_lookup(l);
    if (a != NULL) {
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

    cm_confirm();

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

    cm_noise("for address");
    addr = parse_address(NULL, false);
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
    cm_confirm();
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

    cm_noise("name");
    patname = parse_objname("name of pattern");
    cm_confirm();

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
    cm_confirm();
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
    static char* regname = NULL;
    regindex index;
    address* addr;

    cm_pwrd("name of register", CM_SDH);
    regname = copyatom(regname);

    addr = parse_regrange();
    index = r_index(regname);
    bufstring("Register ");
    bufstring(regname);
    if (index == 0) {
      bufstring(" is undefined at this moment.");
    } else if (!r_isdef(index, addr)) {
      if (addr == NULL) {
	bufstring(" has no default value.");
      } else {
	bufstring(" is undefined at address ");
	bufaddress(addr);
      }
    } else {
      bufstring(" has value ");
      bufvalue(r_read(index, addr));
      if (addr != NULL) {
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
    static address* addr = NULL;
    int count, i, hpos;

    cm_noise("of address");
    addr = a_copy(parse_address(a_a2str(getsfirst(1)), false), addr);
    cm_noise("count");
    count = 0;
    if (addr != NULL) {
      count = getsrest(addr);
    }
    if (count > (64 * 20)) {
      count = 64*20;
    }
    if (count != 0) {
      cm_default(l2s(count));
    } else {
      cm_default("64");
    }
    count = parse_number("number of bytes");
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

    cm_noise("name");
    symname = parse_objname("name of symbol");
    cm_confirm();

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
    cm_confirm();
    notyeterror();
  }
}

/**********************************************************************/

void cmd_unset(void)
{
  static cmkeyword cmds[] = {
    { "noreturn", 0, 0, unset_noreturn },
    { "register", 0, 0, unset_register },
    { "symbol",   0, 0, unset_symbol },
    { NULL },
  };

  if (!dispatch(disparg("Command, ", cmds))) {
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

    cm_noise("from address");
    addr = parse_address(NULL, false);

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

    r_write(index, addr, NULL);
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

    cm_noise("name");
    symname = parse_objname("name of symbol");

    cm_confirm();

    index = s_index(symname);
    if (index == 0) {
      bufstring("The symbol does not exist.\n");
    } else {
      s_write(index, NULL);
    }
  }
}

/**********************************************************************/

void cmd_window(void)
{
  static cmkeyword cmds[] = {
    { "address",  0, 0, win_address },
    { "close",    0, 0, win_close },
    { "current",  0, 0, win_current },
    { "extend",   0, 0, win_extend },
    { "follow",   0, 0, win_follow },
    { "lower",    0, 0, win_lower },
    { "move",     0, 0, win_move },
    { "next",     0, 0, win_next },
    { "raise",    0, 0, win_raise },
    { "segment",  0, 0, win_segment },
    { NULL },
  };

  static cmkeyword openkey[] = {
    { "open",     0, 0, win_open },
    { "update",   0, 0, win_update },
    { NULL },
  };

  /*
  ** The code below is seriously broken...
  */

  cmfdb* numfdb = cm_fdb(_CMNUM, "target window index", NUM_US, (void*) 10);
  cmfdb* winfdb = cm_fdb(_CMKEY, "Command, ", 0, cm_ktab(cmds, 0));
//  cmfdb* opnfdb = cm_fdb(_CMKEY, "the keyword \"open\" to open a new window",
  cmfdb* opnfdb = cm_fdb(_CMKEY, "Keyword, ", 0, cm_ktab(openkey, 0));
  cmfdb* cfmfdb = cm_fdb(_CMCFM, NULL, 0, NULL);

  argwinindex = 0;		/* Clear target index. */

  if (helpflag) {
    cm_parse(cm_chain(winfdb, opnfdb, cfmfdb, NULL));
  } else {
    cm_parse(cm_chain(winfdb, numfdb, opnfdb, NULL));
    if (pval.used->function == _CMNUM) {
      argwinindex = (int) pval.num.number;
      cm_parse(cm_chain(winfdb, NULL));	/* Make sure fdb is unlinked. */
    }
  }

  if (pval.used == cfmfdb) {
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
    cm_dispatch();
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

    cm_noise("to address");
    addr = parse_address(NULL, false);
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
    cm_confirm();
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
    cm_confirm();
    w_setcurrent(argwinindex);
  }
}

/**********************************************************************/

void win_extend(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] EXTEND <target>\n\
\n\
This command makes the argument window automagically have the next\n\
address after the target window.  Useful if you want to have two\n\
windows worth of listing side-by-side, and have the second one\n\
track the first one.\n\
\n\
");
  } else {
    int target;

    cm_pnum("target window number", NUM_US, 10);
    target = (int) pval.num.number;

    cm_confirm();

    w_extend(argwinindex, target);
  }
}

/**********************************************************************/

void win_follow(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW [<number>] FOLLOW <target>\n\
\n\
This command makes the argument window automagically have the address\n\
of the target window.  Useful if you want to have a hex dump window\n\
follow a listing window, for example.\n\
\n\
");
  } else {
    int target;

    cm_pnum("target window number", NUM_US, 10);
    target = (int) pval.num.number;

    cm_confirm();

    w_follow(argwinindex, target);
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
    cm_confirm();
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
    static cmkeyword cmd[] = {
      { "down",  0, 0 },
      { "left",  0, 0 },
      { "right", 0, 0 },
      { "up",    0, 0 },
      { NULL },
    };

    char direction;
    int amount;

    cm_pkey("direction, ", 0, cmd, 0);
    direction = pval.kw->key[0];

    amount = parse_number("number of pixels");
    cm_confirm();

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
  static cmkeyword cmds[] = {
    { "comment",     0, 0, wnxt_comment },
    { "description", 0, 0, wnxt_description },
    { "expansion",   0, 0, wnxt_expansion },
    { "reference",   0, 0, wnxt_reference },
    { "s", KEY_INV+KEY_ABR, "status" },
    { "screen",      0, 0, wnxt_screen },
    { "segment",     0, 0, wnxt_segment },
    { "status",      0, 0, wnxt_status },
    { "unknown",     0, 0, wnxt_unknown },
    { NULL },
  };

  if (!helpflag)
    cm_default("screen");
  if (!dispatch(disparg("Command, ", cmds))) {
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

    cm_confirm();

    pos = w_getaddr(argwinindex);
    if (pos == NULL) {
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

    cm_confirm();

    pos = w_getaddr(argwinindex);
    if (pos == NULL) {
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

void wnxt_expansion(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  [WINDOW [<number>]] NEXT EXPANSION\n\
\n\
This command moves the window forward to the next address with an\n\
expansion.\n\
\n\
");
  } else {
    address* pos;

    cm_confirm();

    pos = w_getaddr(argwinindex);
    if (pos == NULL) {
      bufstring("%This window does not have an address.\n");
    } else {
      a_inc(pos, pv_bpa);
      while (mapped(pos) && !e_exist(pos)) {
	a_inc(pos, pv_bpa);
      }
      if (mapped(pos)) {
	w_setaddr(argwinindex, pos);
      } else {
	bufstring("%There is no expansion from here on.\n");
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

    target = parse_range(NULL, NULL);
    paecheck();

    start = w_getaddr(argwinindex);
    if (start == NULL) {
      bufstring("%This window does not have an address.\n");
    } else {
      a_inc(start, pv_bpa);
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
    cm_confirm();
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

    cm_confirm();

    addr = w_getaddr(argwinindex);
    if (addr == NULL) {
      addr = a_zero();
    }
    segment = getsaddr(addr);
    addr = getsfirst(segment + 1);
    if (addr != NULL) {
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
    stcode status;
    address* pos;

    cm_pkey("status, ", 0, stats, 0);
    status = VP2I(pval.kw->data);

    cm_confirm();

    pos = w_getaddr(argwinindex);
    if (pos == NULL) {
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

    cm_confirm();

    pos = w_getaddr(argwinindex);
    if (pos == NULL) {
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
    static cmkeyword cmds[] = {
      { "counters",   0, (void*) wty_counters },
      { "dots",       0, (void*) wty_dots },
      { "dump",       0, (void*) wty_dump },
      { "h", KEY_INV+KEY_ABR, "hexdump" },
      { "hexdump",    0, (void*) wty_hexdump },
      { "highlight",  0, (void*) wty_highlight },
      { "l", KEY_INV+KEY_ABR, "listing" },
      { "listing",    0, (void*) wty_listing },
      { "logger",     0, (void*) wty_logger },
      { "notes",      0, (void*) wty_notes },
      { "patterns",   0, (void*) wty_patterns },
      { "register",   0, (void*) wty_register },
      { "rpn",        0, (void*) wty_rpn },
      { "s", KEY_INV+KEY_ABR, "status" },
      { "source",     0, (void*) wty_source },
      { "status",     0, (void*) wty_status },
      { "symbols",    0, (void*) wty_symbols },
      { "windows",    0, (void*) wty_windows },
      { NULL },
    };
    int wintype;

    cm_default("listing");
    cm_pkey("Window type, ", 0, cmds, 0);
    wintype = VP2I(pval.kw->data);

    cm_confirm();
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
    cm_confirm();
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

    addr = getsfirst(parse_number("number of segment"));
    cm_confirm();

    if (addr != NULL) {
      w_setaddr(argwinindex, addr);
    } else {
      bufstring("%That segment does not exist.\n");
    }
  }
}

/**********************************************************************/

void win_update(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  WINDOW UPDATE\n\
\n\
This command rewrites all open windowns.\n\
\n\
");
  } else {
    cm_confirm();
    wc_total();
  }
}

/**********************************************************************/

void cmd_write(void)
{
  static cmkeyword formats[] = {
    { "binary",     0, (void*) fmt_binary,   NULL,  "plain binary" },
    { "aout",       0, (void*) fmt_aout,     NULL,  "a.out" },
    { "elf",        0, (void*) fmt_elf,      NULL,  "elf" },
    { "hex",        0, (void*) fmt_hex,      NULL,  "intel hex records" },
    { "motorola",   0, (void*) fmt_motorola, NULL,  "motorola S records" },
    { "shf",        0, (void*) fmt_shf,      NULL,  "S hexdump format" },
    { "tektronix",  0, (void*) fmt_tek,      NULL,  "tektronix hex records" },
    { NULL },
  };

  static char* filename = NULL;
  int format;
  static address* range = NULL;
  
  if (!helpflag) {
    cm_pfil("output file", 0);
    filename = copyatom(filename);
  }

  cm_noise("format");

  if (helpflag) {
    cm_parse(cm_chain(cm_fdb(_CMKEY, "File format, ", 0, cm_ktab(formats, 0)),
		      cm_fdb(_CMCFM, NULL, 0, NULL),
		      NULL));
  } else {
    cm_default("binary");
    cm_pkey("File format, ", 0, formats, 0);
  }

  if (pval.used->function == _CMCFM) {
    bufstring("\
\n\
Syntax:  WRITE <filename> <type>\n\
\n\
This command writes the memory contents to a file.\n\
\n\
");
    return;
  }

  format = VP2I(pval.kw->data);

  cm_confirm();		/* XXX parse eventual range here XXX */
  switch (format) {
  case fmt_aout:
    if (helpflag) {
      bufstring("This help text is missing\n");
    } else
      (void) write_aout(filename, range);
    break;
  case fmt_binary:
    if (helpflag) {
      bufstring("\
\n\
Syntax:  WRITE <filename> BINARY\n\
\n\
This command writes the memory contents to a file sequentially.\n\
\n\
");
    } else
      (void) write_binary(filename, range);
    break;
  case fmt_elf:
    if (helpflag) {
      bufstring("This help text is missing\n");
    } else
      (void) write_elf(filename, range);
    break;
  case fmt_hex:
    if (helpflag) {
      bufstring("\
\n\
Syntax:  WRITE <filename> HEX\n\
\n\
This command writes a file with intel hex-records.\n\
\n\
");
    } else
      (void) write_intel(filename, range);
    break;
  case fmt_motorola:
    if (helpflag) {
      bufstring("This help text is missing\n");
    } else
      (void) write_motorola(filename, range);
    break;
  case fmt_shf:
    if (helpflag) {
      bufstring("This help text is missing\n");
    } else
      (void) write_shf(filename, range);
    break;
  case fmt_tek:
    if (helpflag) {
      bufstring("This help text is missing\n");
    } else
      (void) write_tektronix(filename, range);
    break;
  }
}

/**********************************************************************/

void cmd_xecute(void)
{
  if (help()) {
    bufstring("\
\n\
Syntax:  XCECUTE <address>\n\
\n\
This command executes a single instruction at the given address.\n\
\n\
");
  } else {
    static address* pos = NULL;

    pos = a_copy(parse_address(NULL, false), pos);
    paconfirm();

    if (pcheck()) {
      if (mapped(pos)) {
	bufshutup(true);
	peek(pos, EX_UPD, st_inst);
	if (pb_status == st_inst) {
	  setstatus(istart, st_inst, pb_length);
	}
	wc_total();
	bufshutup(false);
      }
    }
  }
}

/**********************************************************************/

/* end of file */
