#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <utime.h>
#include <sys/utsname.h>

#include "backwr.h"

typedef unsigned int bool;
#  define false 0
#  define true  1

typedef unsigned char byte;
typedef unsigned long long int36;

/*
** input/output buffers:
*/

byte diskbuffer[512*5];		/* Disk file data. */
int diskcount;			/* Amount of data in above. */

int36 taperecord[32+512];	/* Tape record we build/decode. */

FILE* tapefile;			/* Tape file we read/write. */

FILE* diskfile;			/* Disk file we read/write. */
unsigned long filelength;	/*   Length in bytes. */
time_t filemodified;		/*   Time last modified. */

int dataoffset;			/* Data offset in file. */

bool debugflag = false;

struct utsname unameinfo;

/*
** tops-10 file information:
*/

char topsname[7];
char topsext[4];

/************************************************************************/

static void print36(int36 word)
{
  int l, r;

  l = (word >> 18) & 0777777;
  r = word & 0777777;

  printf("%06o,,%06o", l, r);
}

static int nextseq(void)
{
  static int record = 1;

  return(record++);
}

static int36 sixbit(char name[])
{
  int i;
  char c;
  int36 w;

  w = (int36) 0;
  c = '*';

  for (i = 0; i < 6; i += 1) {
    if (c != (char) 0) {
      c = name[i];
    }
    if (c != (char) 0) {
      if ((c >= 'a') && (c <= 'z')) {
	c = c - 'a' + 'A';
      }
      c -= 32;
    }
    w = (int36) w << 6;
    w += c;
  }
  return (w);
}

static int36 ascii(char name[])
{
  int i;
  char c;
  int36 w;

  w = (int36) 0;
  c = '*';

  for (i = 0; i < 5; i += 1) {
    if (c != (char) 0) {
      c = name[i];
    }
    w = (int36) w << 7;
    w += c;
  }
  w <<= 1;
  return (w);
}

static int36 xwd(int l, int r)
{
  int36 w;

  w = l;
  w <<= 18;
  w += r;
  return (w);
}

static int36* w_text(int36* ptr, int type, int words, char* text)
{
  int bytes;

  bytes = strlen(text) + 1;
  if (words == 0) {
    words = (bytes + 4) / 5;
  }
  *ptr++ = xwd(type, words + 1);
  while (words-- > 0) {
    *ptr++ = ascii(text);
    bytes -= 5;
    if (bytes > 0) {
      text += 5;
    } else {
      text = "";
    }
  }
  return(ptr);
}

static void zerotaperecord(void)
{
  int i;

  for (i = 0; i < 544; i += 1) {
    taperecord[i] = (int36) 0;
  }
}

static int36 checksumdata(int36* data, int count)
{
  int36 checksum;

  checksum = (int36) 0;
  while (count-- > 0) {
    checksum += (int36) *data++;
    checksum <<= 1;
    if (checksum & (int36) 0x1000000000LL) {
      checksum++;
    }
    checksum &= (int36) 0xfffffffffLL;
  }
  return (checksum);
}

static void checksumbuffer(void)
{
  taperecord[G_CHECK] = (int36) 0;
  taperecord[G_CHECK] = checksumdata(taperecord, 544);
}

static void writetape(void)
{
  int i;
  int36 w;

  fputc(0xa0, tapefile);
  fputc(0x0a, tapefile);
  fputc(0x00, tapefile);
  fputc(0x00, tapefile);

  for (i = 0; i < 544; i += 1) {
    w = taperecord[i];
    fputc((w >> 28) & 0xff, tapefile);
    fputc((w >> 20) & 0xff, tapefile);
    fputc((w >> 12) & 0xff, tapefile);
    fputc((w >> 4) & 0xff, tapefile);
    fputc(w & 0x0f, tapefile);
  }

  fputc(0xa0, tapefile);
  fputc(0x0a, tapefile);
  fputc(0x00, tapefile);
  fputc(0x00, tapefile);
}

static void writeeof(void)
{
  int i;

  for (i = 0; i < 8; i += 1) {
    fputc(0x00, tapefile);
  }
}

/*
** time2udt() converts a unix time_t to a 36-bit tops UDT.
*/

static int36 time2udt(time_t t)
{
  int days, seconds;

  /* here we should apply timezone. */

  days = t / 86400;
  seconds = t - days * 86400;

  days += 0117213;
  seconds *= 2048;
  seconds /= 675;

  return (((int36) days << 18) + seconds);
}

/*
** getnow() returns a word with the current date/time in a DEC10
** universial date/time format.
*/

static int36 getnow(void)
{
  return (time2udt(time(NULL)));
}

/*
** setup_general() inits a general record.
*/

void setup_general(int blocktype)
{
  zerotaperecord();
  taperecord[G_TYPE] = blocktype;
  taperecord[G_SEQ] = nextseq();
  taperecord[G_RTNM] = 1;	/* Tape number, only one. */
  taperecord[G_FLAGS] = 0;	/* No flags. */
}

/*
** setup_BE() inits a T$BEGIN or T$END record.
*/

void setup_BE(int blocktype)
{
  setup_general(blocktype);	/* Do the common fields. */

  taperecord[S_DATE] = getnow();/* (014) Date/time. */
  taperecord[S_FORMAT] = 1;	/* (015) Format */
  taperecord[S_BVER] = 0;	/* (016) Backup version. */
  taperecord[S_BVER] = xwd(0300, 0411);
  taperecord[S_MONTYP] = 0;	/* (017) Strange system... */
  taperecord[S_SVER] = 0;	/* (020) OS version. */
  taperecord[S_APR] = 4097;	/* (021) BAH will hate this. */
  taperecord[S_DEVICE] = sixbit("mta0"); /* (022) Device. */
  taperecord[S_MTCHAR] = 4;	/* (023) 1600 bpi. */

  (void) w_text(&taperecord[32], O_SYSNAME, 6, unameinfo.sysname);
  taperecord[G_LND] = 7;	/* FUCK! */
}

void wr_begin(void)
{
  setup_BE(T_BEGIN);
  checksumbuffer();
  writetape();
}

void wr_end(void)
{
  setup_BE(T_END);
  checksumbuffer();
  writetape();
  writeeof();
}

static int36* w_foo(int36* ptr, int type, char* text)
{
  char buffer[100];
  int bytes, words;

  bytes = strlen(text);
  words = (bytes + 6) / 5;
  buffer[0] = type;
  buffer[1] = words;
  strcpy(&buffer[2], text);
  text = buffer;
  while (words-- > 0) {
    *ptr++ = ascii(text);
    text += 5;
  }
  return(ptr);
}

static void mkheadpath(int36 sum)
{
  int36* pos;

  pos = &taperecord[F_PTH];
  pos = w_foo(pos, 2, topsname);
  pos = w_foo(pos, 3, topsext);

  taperecord[F_PCHK] = sum;
}

static void readdisk(void)
{
  int i;

  diskcount = fread(diskbuffer, sizeof(char), (5*512), diskfile);
  for (i = diskcount; i < (5*512); i += 1) {
    diskbuffer[i] = 0;
  }    
}

static bool openinput(char* name)
{
  struct stat statbuf;
  char* namepart;
  char* extpart;
  int pos;
  char c;

  namepart = rindex(name, '/');
  if (namepart == NULL) {
    namepart = name;
  } else {
    namepart++;
  }
  extpart = rindex(namepart, '.');
  if (extpart) {
    extpart++;
  }

  pos = 0;
  while ((c = *namepart++) != 0) {
    if ((c >= 'a') && (c <= 'z')) {
      c = c - 'a' + 'A';
    }
    if (c == '.') break;
    if (pos < 6) {
      topsname[pos] = c;
      pos += 1;
    }
  }
  topsname[pos] = 0;

  pos = 0;
  if (extpart != NULL) {
    while ((c = *extpart++) !=0) {
      if ((c >= 'a') && (c <= 'z')) {
	c = c - 'a' + 'A';
      }
      if (pos < 3) {
	topsext[pos] = c;
	pos += 1;
      }
    }
  }
  topsext[pos] = 0;

  diskfile = fopen(name, "rb");
  if (diskfile == NULL) {
    printf("Can't open %s for reading.\n", name);
    return (false);
  }

  if (fstat(fileno(diskfile), &statbuf) != 0) {
    printf("... fstat failed...\n");
  }

  filelength = statbuf.st_size;
  filemodified = statbuf.st_mtime;

  dataoffset = 0;

  readdisk();
}

static bool binarydata(void)
{
  int i;

  for (i = 4; i < diskcount; i += 5) {
    if (diskbuffer[i] & 0x80) {
      return (true);
    }
  }
  return (false);
}

static void copy2tape(int36* ptr, int bytes)
{
  int i;
  byte b;
  int36 w;

  for (i = 0; i < bytes; i += 5) {
    w = (int36) diskbuffer[i] << 29;
    w |= (int36) diskbuffer[i+1] << 22;
    w |= (int36) diskbuffer[i+2] << 15;
    w |= (int36) diskbuffer[i+3] << 8;
    b = diskbuffer[i+4];
    if (b & 0x80) {
      b = ((b << 1) & 0xfe) + 1;
    } else {
      b = (b << 1) & 0xfe;
    }
    w |= (int36) b;
    *ptr++ = w;
    dataoffset += 1;
  }

  taperecord[G_SIZE] = (bytes + 4) /5;
}

void wr_file(char* name)
{
  int36* pos;
  int36 pthchecksum;

  if (debugflag) {
    printf("Writing file %s:\n", name);
  }

  if (!openinput(name)) {
    return;
  }

  setup_general(T_FILE);	/* Set up for a file block. */
  if (diskcount <= 1280) {	/* Fits in one record? */
    taperecord[G_FLAGS] = xwd(GF_SOF+GF_EOF, 0);
  } else {
    taperecord[G_FLAGS] = xwd(GF_SOF, 0);
  }

  /* build name block: */

  taperecord[32] = xwd(O_NAME, 0200);
  pos = &taperecord[33];
  pos = w_text(pos, 2, 0, topsname);
  pos = w_text(pos, 3, 0, topsext);

  /* compute checksum of file name block: */

  pthchecksum = checksumdata(&taperecord[32], 0200);

  /* build attribute block: */

  taperecord[32+0200] = xwd(O_FILE, 0200);

  pos = &taperecord[32+0201];

  pos[A_FHLN] = 032;

  pos[A_WRIT] = time2udt(filemodified);
  pos[A_ALLS] = 02400;
  if (filelength == 0) {
    pos[A_ALLS] = 3 * 128;
  } else {
    pos[A_ALLS] = (((filelength + 639) / 640) * 128 ) + 256;
  }
  if (binarydata()) {
    pos[A_MODE] = 016;
    pos[A_LENG] = (filelength + 4) / 5;
    pos[A_BSIZ] = 36;
  } else{
    pos[A_MODE] = 0;
    pos[A_LENG] = filelength;
    pos[A_BSIZ] = 7;
  }

  taperecord[G_LND] = 0400;

  /* fill in header: */

  mkheadpath(pthchecksum);

  if (diskcount < 1280) {
    copy2tape(&taperecord[32+0400], diskcount);
    if (debugflag) {
      printf("writing all data (%d bytes) in first tape block.\n", diskcount);
    }
    checksumbuffer();
    writetape();
  } else {
    bool more = true;

    checksumbuffer();
    writetape();
    if (debugflag) {
      printf("writing first tape block with no data.\n");
    }

    while (more) {
      setup_general(T_FILE);	/* Set up for next file block. */
      mkheadpath(pthchecksum);
      taperecord[F_RDW] = dataoffset;
      copy2tape(&taperecord[32], diskcount);
      readdisk();
      if (diskcount == 0) {
	taperecord[G_FLAGS] = xwd(GF_EOF, 0);
	more = false;
      } else {
	taperecord[G_FLAGS] = xwd(0, 0);
      }
      checksumbuffer();
      writetape();
    }
  }
}

int main(int argc, char* argv[])
{
  char* outputname;
  char* s;
  bool namenext = false;
  
  if (--argc > 0) {
    for (s = *(++argv); *s != (char) 0; s++) {
      switch(*s) {
      case '-':
        break;
      case 'd':
	debugflag = true; break;
      case 'f':
	namenext = true;  break;
      default:
	fprintf(stderr, "backwr: bad option %c\n", *s);
	exit(1);
      }
    }
  }

  if (namenext) {
    if (--argc > 0) {
      outputname = *(++argv);
    } else {
      fprintf(stderr, "backwr: input file name missing\n");
      exit(1);
    }
  } else {
    outputname = "-";		/* STDIO */
  }

  (void) uname(&unameinfo);	/* Get sysname etc. */

  if (strcmp(outputname, "-") != 0) {
    tapefile = fopen(outputname, "wb");
    if (tapefile == NULL) {
      fprintf(stderr, "backwr: can't open output file\n");
      exit(1);
    }
  } else {
    tapefile = stdin;
  }

  wr_begin();
  while (--argc > 0) {
    wr_file(*(++argv));
  }
  wr_end();
}
