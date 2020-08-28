/*
** This module implements file loading.
*/

#include "disass.h"

/* Compile-time logic to find the symbols needed for parsing of
 * elf files.  On all systems that know about elf files, the
 * symbols can be found in /usr/include/elf.h, except OpenBSD,
 * which for some unknown reason calls this file elf_abi.h.
 *
 * Maybe we should make our own copy of the symbols we need to
 * read in elf files, and avoid the system headers.  Some systems
 * does not have these headers since they do not have elf files...
 */

#if defined (__OpenBSD__)
#  include <elf_abi.h>
#else
#  include <elf.h>
#endif

//#define DEBUG			/* For now... */

static int endian;

static bool error(char* message)
{
  bufchar('?');
  bufstring(message);
  bufnewline();
  wf_close();

  return false;
}

static word get16(void* data)
{
  byte* ptr;
  word ret;

  ptr = data;

  if (endian == 1) {
    ret = ptr[0] + (ptr[1] << 8);
  } else {
    ret = ptr[1] + (ptr[0] << 8);
  }
  return ret;
}

static longword get32(void* data)
{
  byte* ptr;
  longword ret;

  ptr = data;

  if (endian == 1) {
    ret = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
  } else {
    ret = ptr[3] + (ptr[2] << 8) + (ptr[1] << 16) + (ptr[0] << 24);
  }
  return ret;
}

static quadword get64(void* data)
{
  byte* p;
  quadword ret;

  p = data;

  if (endian == 1) {
    ret = get32(&p[4]);
    ret <<= 32;
    ret |= get32(p);
  } else {
    ret = get32(p);
    ret <<= 32;
    ret |= get32(&p[4]);
  }
  return ret;
}

#define get32_addr(x)   get32(x)
#define get32_half(x)   get16(x)
#define get32_off(x)    get32(x)
#define get32_word(x)   get32(x)

#define get64_addr(x)   get64(x)
#define get64_half(x)   get16(x)
#define get64_off(x)    get64(x)
#define get64_word(x)   get32(x)
#define get64_xword(x)  get64(x)

/*
** sym32() reads a set of symbols from the currently open elf file.
*/

static void sym32(int offset, int size, char* names, int namesize)
{
  Elf32_Sym sym;
  int type, bind;

  if (names == NULL)
    return;			/* No need to do this then. */

  wf_setpos(offset);

  while (size >= sizeof(sym)) {
    if (sizeof(sym) != wf_rblock((byte*) &sym, sizeof(sym))) {
      return;			/* No can read, give up. */
    }
    offset = get32(&sym.st_name);
    type = ELF32_ST_TYPE(sym.st_info);
    bind = ELF32_ST_BIND(sym.st_info);
    if ((type == STT_OBJECT || type == STT_FUNC) &&
	(bind == STB_GLOBAL || bind == STB_LOCAL || bind == STB_WEAK) &&
	offset < namesize) {
      if ((bind == STB_GLOBAL || l_lookup(&names[offset]) == NULL) &&
	  sym.st_value != 0) {
	l_insert(a_l2a(get32_addr(&sym.st_value)), &names[offset]);
      }
    }
    size -= sizeof(sym);
  }
}

/*
** sym64() is the 64-bit version of sym32.
*/

static void sym64(int offset, int size, char* names, int namesize)
{
  Elf64_Sym sym;
  int type, bind;

  if (names == NULL)
    return;			/* No need to do this then. */

  wf_setpos(offset);

  while (size >= sizeof(sym)) {
    if (sizeof(sym) != wf_rblock((byte*) &sym, sizeof(sym))) {
      return;			/* No can read, give up. */
    }
    offset = get64_word(&sym.st_name);
    type = ELF64_ST_TYPE(sym.st_info);
    bind = ELF64_ST_BIND(sym.st_info);
    if ((type == STT_OBJECT || type == STT_FUNC) &&
	(bind == STB_GLOBAL || bind == STB_LOCAL || bind == STB_WEAK) &&
	offset < namesize) {
      if ((bind == STB_GLOBAL || l_lookup(&names[offset]) == NULL) &&
	  sym.st_value != 0) {
	l_insert(a_q2a(get64_addr(&sym.st_value)), &names[offset]);
      }
    }
    size -= sizeof(sym);
  }
}

/*
** getdata() reads a block of data from the file into memory.
*/

static void getdata(address* ldaddr, int ldsize)
{
  byte buffer[256];
  int size;

  while (ldsize > 256) {
    size = wf_rblock(buffer, 256);
    m_set(ldaddr, buffer, size);
    ldsize -= size;
  }

  if (ldsize > 0) {
    size = wf_rblock(buffer, ldsize);
    m_set(ldaddr, buffer, size);
  }
}

/*
** load_elf is the routine that loads elf files.
*/

bool load_elf(char* filename)
{
  char* errstr = NULL;	   /* Set to error msg before "goto abort". */

  int i;

  int size;
  int class;
  int phoff, phnum, phsiz;
  int shoff, shnum, shsiz;

  address* startaddr = NULL;

  byte ident[EI_NIDENT];

  byte* pharray = NULL;
  byte* sharray = NULL;

  typedef struct strblock {
    char* str;
    int size;
  } strblock;
  
  strblock* strarray = NULL;

  Elf32_Ehdr e32h;
  Elf32_Phdr* e32p;
  Elf32_Shdr* e32s;

  Elf64_Ehdr e64h;
  Elf64_Phdr* e64p;
  Elf64_Shdr* e64s;
  
  static address* ldaddr = NULL;
  int ldoffset;
  int ldsize;
  int ldlink;

  if (!iocheck(wf_bopen(filename)))
    return false;

  size = wf_rblock(ident, EI_NIDENT);
  if (size < EI_NIDENT)
    return error("file too short for ELF header");

  if (ident[EI_MAG0] != ELFMAG0 ||
      ident[EI_MAG1] != ELFMAG1 ||
      ident[EI_MAG2] != ELFMAG2 ||
      ident[EI_MAG3] != ELFMAG3)
    return error("file is not an ELF file");

  switch (ident[EI_DATA]) {
  case ELFDATA2LSB:
    endian = 1;			/* little-endian. */
    break;
  case ELFDATA2MSB:
    endian = 2;			/* big-endian. */
    break;
  default:
    return error("unrecognised endianness");
  }

  class = ident[EI_CLASS];

  switch (class) {
  case ELFCLASS32:
    memcpy(&e32h, ident, size);
    size += wf_rblock(&((byte*)&e32h)[size], sizeof(Elf32_Ehdr) - size);
    if (size < sizeof(Elf32_Ehdr))
      return error("file to short for ELF32 header");

    startaddr = a_copy(a_l2a(get32_addr(&e32h.e_entry)), NULL);

    phoff = get32_off(&e32h.e_phoff);
    phnum = get32_half(&e32h.e_phnum);
    phsiz = get32_half(&e32h.e_phentsize);

    shoff = get32_off(&e32h.e_shoff);
    shnum = get32_half(&e32h.e_shnum);
    shsiz = get32_half(&e32h.e_shentsize);

    break;
  case ELFCLASS64:
    memcpy(&e32h, ident, size);
    size += wf_rblock(&((byte*)&e64h)[size], sizeof(Elf64_Ehdr) - size);
    if (size < sizeof(Elf64_Ehdr))
      return error("file to short for ELF64 header");

    startaddr = a_copy(a_q2a(get64_addr(&e64h.e_entry)), NULL);

    phoff = get64_off(&e64h.e_phoff);
    phnum = get64_half(&e64h.e_phnum);
    phsiz = get64_half(&e64h.e_phentsize);

    shoff = get64_off(&e64h.e_shoff);
    shnum = get64_half(&e64h.e_shnum);
    shsiz = get64_half(&e64h.e_shentsize);

    break;
  default:
    return error("unrecognised file class");
  }

#ifdef DEBUG
  bufstring("phoff = "); bufnumber(phoff); bufnewline();
  bufstring("phnum = "); bufnumber(phnum); bufnewline();
  bufstring("phsiz = "); bufnumber(phsiz); bufnewline();
  bufstring("shoff = "); bufnumber(shoff); bufnewline();
  bufstring("shnum = "); bufnumber(shnum); bufnewline();
  bufstring("shsiz = "); bufnumber(shsiz); bufnewline();
#endif

  /*
   * From here on, we start to allocate memory, and therefore we cant
   * really do "return error(...)" anymore, we have to set up errstr
   * and goto abort to make sure we clean up.
   */

  size = phnum * phsiz;
  if (size > 0) {
    pharray = malloc(size);
    if (pharray == NULL) {
      errstr = "cant allocate program header array";
      goto abort;
    }
    wf_setpos(phoff);
    if (size != wf_rblock(pharray, size)) {
      errstr = "cant read program header array";
      goto abort;
    }
  }

  size = shnum * shsiz;
  if (size > 0) {
    sharray = malloc(size);
    if (sharray == NULL) {
      errstr = "cant allocate section header array";
      goto abort;
    }
    wf_setpos(shoff);
    if (size != wf_rblock(sharray, size)) {
      errstr = "cant read section header array";
      goto abort;
    }
  }

  /*
   * Handle the data we have, according to class.
   */

  switch (class) {
  case ELFCLASS32:
    e32p = (Elf32_Phdr*) pharray;

    for (i = 0; i < phnum; i += 1, e32p++) {
#ifdef DEBUG
      bufstring("p_type = ");
      bufnumber(get32_word(&e32p->p_type));
      bufnewline();
#endif

      switch (get32_word(&e32p->p_type)) {
      case PT_LOAD:
	ldaddr = a_copy(a_l2a(get32_addr(&e32p->p_vaddr)), ldaddr);

	wf_setpos(get32_off(&e32p->p_offset));
	getdata(ldaddr, get32_word(&e32p->p_filesz));

	break;
      }
    }
    break;
  case ELFCLASS64:
    e64p = (Elf64_Phdr*) pharray;

    for (i = 0; i < phnum; i += 1, e64p++) {
#ifdef DEBUG
      bufstring("p_type = ");
      bufnumber(get64_word(&e64p->p_type));
      bufnewline();
#endif

      switch (get64_word(&e64p->p_type)) {
      case PT_LOAD:
	ldaddr = a_copy(a_q2a(get64_addr(&e64p->p_vaddr)), ldaddr);

	wf_setpos(get64_off(&e64p->p_offset));
	getdata(ldaddr, get64_xword(&e64p->p_filesz));

	break;
      }
    }
    break;
  }

  /*
   * Allocate a table to hold our copies of the string sections.
   */

  size = shnum * sizeof(struct strblock);
  strarray = malloc(size);
  if (strarray == NULL) {
    errstr = "cant allocate string table array";
    goto abort;
  }
  memset(strarray, 0, size);

  /*
   * walk thru the section headers, read in the string sections.
   */

  switch (class) {
  case ELFCLASS32:
    e32s = (Elf32_Shdr*) sharray;

    for (i = 0; i < shnum; i += 1, e32s++) {
      if (get32_word(&e32s->sh_type) == SHT_STRTAB) {
	size = get32_word(&e32s->sh_size);
       
#ifdef DEBUG
	bufstring("section ");
	bufnumber(i);
	bufstring(" is string table, size ");
	bufnumber(size);
	bufstring(" bytes");
	bufnewline();
#endif

	strarray[i].str = malloc(size + 1);
	strarray[i].size = size;
	if (strarray[i].str == NULL) {
	  errstr = "cant allocate string table entry";
	  goto abort;
	}
	wf_setpos(get32_off(&e32s->sh_offset));
	if (size != wf_rblock((byte*)strarray[i].str, size)) {
	  errstr = "cant read string section";
	  goto abort;
	}
	strarray[i].str[size] = (char) 0;
      }
    }
    break;
  case ELFCLASS64:
    e64s = (Elf64_Shdr*) sharray;

    for (i = 0; i < shnum; i += 1, e64s++) {
      if (get64_word(&e64s->sh_type) == SHT_STRTAB) {
	size = get64_xword(&e64s->sh_size);
     
#ifdef DEBUG
	bufstring("section ");
	bufnumber(i);
	bufstring(" is string table, size ");
	bufnumber(size);
	bufstring(" bytes");
	bufnewline();
#endif

	strarray[i].str = malloc(size + 1);
	strarray[i].size = size;
	if (strarray[i].str == NULL) {
	  errstr = "cant allocate string table entry";
	  goto abort;
	}
	wf_setpos(get64_off(&e64s->sh_offset));
	if (size != wf_rblock((byte*)strarray[i].str, size)) {
	  errstr = "cant read string section";
	  goto abort;
	}
	strarray[i].str[size] = (char) 0;
      }
    }
    break;
  }

  /*
   * walk thru the section headers, looking for symbols.
   */

  switch (class) {
  case ELFCLASS32:
    e32s = (Elf32_Shdr*) sharray;

    for (i = 0; i < shnum; i += 1, e32s++) {
      switch (get32_word(&e32s->sh_type)) {
      case SHT_SYMTAB:
      case SHT_DYNSYM:

#ifdef DEBUG
	bufstring("section ");
	bufnumber(i);
	bufstring(", symbol table (");
	bufstring(get32_word(&e32s->sh_type) == SHT_SYMTAB?
		  "symtab" : "dynsym");
	bufstring("), strings in ");
	bufnumber(get32_word(&e32s->sh_link));
	bufnewline();
#endif

	ldoffset = get32_off(&e32s->sh_offset);
	ldsize = get32_word(&e32s->sh_size);
	ldlink = get32_word(&e32s->sh_link);

	if (ldlink < shnum) {
	  sym32(ldoffset, ldsize, strarray[ldlink].str, strarray[ldlink].size);
	}

	break;
      }
    }

    break;
  case ELFCLASS64:
    e64s = (Elf64_Shdr*) sharray;

    for (i = 0; i < shnum; i += 1, e64s++) {
      switch (get64_word(&e64s->sh_type)) {
      case SHT_SYMTAB:
      case SHT_DYNSYM:

#ifdef DEBUG
	bufstring("section ");
	bufnumber(i);
	bufstring(", symbol table (");
	bufstring(get64_word(&e64s->sh_type) == SHT_SYMTAB?
		  "symtab" : "dynsym");
	bufstring("), strings in ");
	bufnumber(get64_word(&e64s->sh_link));
	bufnewline();
#endif

	ldoffset = get64_off(&e64s->sh_offset);
	ldsize = get64_xword(&e64s->sh_size);
	ldlink = get64_word(&e64s->sh_link);

	if (ldlink < shnum) {
	  sym64(ldoffset, ldsize, strarray[ldlink].str, strarray[ldlink].size);
	}
	
	break;
      }
    }

    break;
  }

  bufstring("start address: ");
  bufaddress(startaddr);
  bufnewline();

 abort:

  if (pharray != NULL)
    free(pharray);
  if (sharray != NULL)
    free(sharray);

  if (strarray != NULL) {
    for (i = 0; i < shnum; i += 1) {
      if (strarray[i].str != NULL) {
#ifdef DEBUG
	bufstring("freeing string table entry ");
	bufnumber(i);
	bufstring(", size ");
	bufnumber(strarray[i].size);
	bufnewline();
#endif

	free(strarray[i].str);
      }
    }
    free(strarray);
  }

  a_free(startaddr);

  if (errstr != NULL)
    return error(errstr);

  return true;
}

/*
** load_aout is the routine that loads a.out files.
*/

bool load_notyet(void)
{
  bufstring("?File format not yet implemented.\n");
  return false;
}

bool load_aout(char* filename)
{
  UNUSED(filename);

  return load_notyet();		/* Not yet implemented. */
}

/*
** load_shf is the routine that loads files in shf format.
*/

bool load_shf(char* filename)
{
  UNUSED(filename);

  return load_notyet();		/* Not yet implemented. */
}

/*
** load_tektronix() is the routine that loads files in tektronix hex
** format.
*/

bool load_tektronix(char* filename)
{
  UNUSED(filename);

  return load_notyet();		/* Not yet implemented. */
}

static address* defaultrange(void)
{
  static address* range = NULL;

  /* generate a range here, looking like the segment list, and return
   * a pointer to it.
   */

  return range;
}

/*
** write_aout() writes the memory contents as an a.out format file
*/

bool write_aout(char* filename, address* range)
{
  return error("not yet implemented");
}

/*
** write_binary() is the routine that writes the full memory contents out
** in binary, to to given file.  Note that we dont worry about things like
** holes in the layout or starting addresses or anything like that.
*/

bool write_binary(char* filename, address* range)
{
  int i, count;
  longword size;
  byte buffer[256];
  int pos;

  if (!iocheck(wf_wbopen(filename)))
    return false;

  count = getscount();
  for (i = 1; i <= count; i += 1) {
    setpc(getsfirst(i));
    size = getslength(i);
    pos = 0;
    while (size-- > 0) {
      buffer[pos++] = getnext();
      if (pos == 256) {
	wf_wblock(buffer, pos);
	pos = 0;
      }
    }
    if (pos > 0)
      wf_wblock(buffer, pos);
  }

  wf_close();

  return true;
}

/*
** write_elf() writes the memory contents as an elf file.
*/

bool write_elf(char* filename, address* range)
{
  return error("not yet implemented");
}

/*
** write_intel() writes the memory contents as a text file, containing
** intel hex records.
*/

static void intelrecord(int type, int addr, int len, byte* data)
{
  int csm;
  byte b;
  int i;

  wf_wchar(':');
  wf_w2hex(len);
  csm = len;

  b = (addr & 0xff00) >> 8;
  wf_w2hex(b);
  csm += b;

  b = addr & 0xff;
  wf_w2hex(b);
  csm += b;
       
  wf_w2hex(type);
  csm += type;

  for (i = 0; i < len; i += 1) {
    b = data[i];
    wf_w2hex(b);
    csm += b;
  }

  wf_w2hex(0x100 - (csm & 0xff));
  wf_newline();
}

bool write_intel(char* filename, address* range)
{
  int seg, count;
  int size;
  byte buffer[32];
  int pos;
  int offset;
  int addr;

  offset = 0;

  if (!iocheck(wf_wopen(filename)))
    return false;

  count = getscount();
  for (seg = 1; seg <= count; seg += 1) {
    setpc(getsfirst(seg));
    size = getslength(seg);
    while (size > 0) {
      addr = a_a2l(pc);

      if (offset != (addr >> 16)) {
	offset = addr >> 16;

	buffer[0] = offset >> 8;
	buffer[1] = offset & 0xff;
	intelrecord(4, 0, 2, buffer);
      }

      pos = 0;
      while (pos < 32 && size-- > 0) {
	buffer[pos++] = getnext();
      }
      intelrecord(0, addr & 0xffff, pos, buffer);
    }
  }

  intelrecord(1, 0, 0, buffer);

  wf_close();
  return true;
}

/*
** write_motorola() writes the memory contents as a text file, containing
** motorola S records.
*/

static void motorolarecord(char type, int addr, int len, byte* data)
{
  int csm;
  byte b;
  int i;
  int alen;

  wf_wchar('S');
  wf_wchar(type);

  switch (type) {
  case '2': alen = 3; break;
  case '3': alen = 4; break;
  default: alen = 2; break;
  }

  wf_w2hex(len + alen + 1);
  csm = len + alen + 1;

  switch (type) {
  case '3':
    csm += (b = (addr >> 24) & 0xff);
    wf_w2hex(b);    
    /* fallthrough */
  case '2':
    csm += (b = (addr >> 16) & 0xff);
    wf_w2hex(b);
    /* fallthrough */
  case '1':
  case '5':
    csm += (b = (addr >> 8) & 0xff);
    wf_w2hex(b);
    csm += (b = addr & 0xff);
    wf_w2hex(b);
    break;
  default:
    /* bug, internal error in program. */
    break;
  }

  for (i = 0; i < len; i += 1) {
    wf_w2hex(data[i]);
    csm += data[i];
  }

  wf_w2hex(~csm & 0xff);
  wf_newline();
}

bool write_motorola(char* filename, address* range)
{
  int seg, count;
  int size;
  byte buffer[32];
  int pos;
  int addr;
  char rtype = '1';

  count = getscount();
  if (count == 0)
    return true;	  /* No need to try to write an empty file. */

  if (!iocheck(wf_wopen(filename)))
    return false;

  addr = a_a2l(getslast(count));
  if (addr & 0x00ff0000)
    rtype = '2';
  if (addr & 0xff000000)
    rtype = '3';

  for (seg = 1; seg <= count; seg += 1) {
    setpc(getsfirst(seg));
    size = getslength(seg);
    while (size > 0) {
      addr = a_a2l(pc);
      pos = 0;
      while (pos < 32 && size-- > 0) {
	buffer[pos++] = getnext();
      }
      motorolarecord(rtype, addr, pos, buffer);
    }
    /* store S5 record here? */
  }
  /* store S5 record here? */

  wf_close();
  return true;
}

/*
** write_shf() writes the memory contents as a file in shf format.
*/

bool write_shf(char* filename, address* range)
{
  return error("not yet implemented");
}

/*
** write_tektronix() writes the memory contents as a text file, containing
** tektronix hex records.
*/

bool write_tektronix(char* filename, address* range)
{

  /*
  ** tektronix hex:
  **
  ** "/" aaaa bc pc dddd... dc -- data record
  **
  ** aaaa = hex addr
  ** bc   = bytecount
  ** pc   = prefix checksum
  ** dddd = data
  ** dc   = data checksum
  **
  ** "/" aaaa 00 cc -- end-of-file record
  **
  ** aaaa = transfer address
  ** 00   = zero byte count
  ** cc   = checksum
  **
  ** all checksums are the sum of the four-bit values of the individual
  ** hex chars, modulo 256.
  **
  ** Tek Hex Example
  **   /00001102444154414D414E2053332053455249414C8F
  **   /01000001
  **
  */

  return error("not yet implemented");
}
