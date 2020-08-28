#define STDIO
#define NEW
/*
 * Written by Johnny Eriksson <bygg@sunet.se>
 * Modifications by Phil Budne <budd@cs.bu.edu> 12/95;
 *	use O_XXX defines for block types
 *	created handy "word" type for easy arg passing
 *	display version numbers, dates
 *	dump good stuff from BEGIN/END records
 *	set file dates when extracting
 *	use raw I/O; saves a little time, readblock() returns status
 *	reverted back to STDIO; didn't deal with pipes
 *	changed output to be more columnar (a lot like read20 output)
 *		include blocks, bytes (if extracted), protection, date
 *	added -D flag for debug output
 *	added -a flag for ascii text file extraction (-m was unimplemented)
 *	added a_cusr to verbose display
 *	new cname in buildnames, added -o for old way
 *	error to specify both -t and -x
 *	added SETZM macro
 *	12/29/95 added -F (replaces -d)
 *	12/29/95 added -r
 *	12/24/99!! added -c; read supnik simulator format tapes (bytecounts)
 *
 * TODO/IDEAS;
 *	make arg processing (even) more like tar?
 *	man page!!
 *	input;
 *	     handle large physical blocksize (use setbuf?)??
 *	canonical name formation;
 *	    uc/lc
 *	listings;
 *	    options for which date to show??
 *	extraction patterns;
 *	    accept missing filename as ''
 *	    accept trailing *?
 *	    full globbing??
 *	    -e regexp???
 *	extracted filename;
 *	    make '-d' the default?
 *	    option to drop 'n' leading elements from cname (dev,ufd,sfds)
 *		option to keep/drop devicename?
 *	extracted file;
 *	    option to toss LSN's?
 *	    honor read-only protection?
 *	    honor T$UFD (use for directory protections)??
 *	    honor '-8' flag?
 *	    add flag to extract 36-bit bytes (as rawdata)
 *	    autobytesize option?
 *	    ignore files with known binary extesion types???
 *		ie; EXE,REL,BIN,DMP,UNV,SAV,HGH,LOW,SHR
 *	process T_LABEL records (I have no labeled tapes to test)!
 */


/* Dump contents of Tops-10 BACKUP tapes, which have been read
   into a disk file. The "known good" way to do this is to use the
   unix utility "dd", and a command line something like this:

   dd if=/dev/rmt0 of=data ibs=2720 obs=2720 conv=block

   the key thing is that this program expects a fixed block size of
   2720 bytes.  If the tape actually has some other format, this 
   program probably won't succeed.  You can use the unix utility "tcopy"
   to inspect the contents of the tape.

   Here's the tcopy output from a good tape:

   tcopy /dev/rmt0 
   file 0: block size 2720: 9917 records
   file 0: eof after 9917 records: 26974240 bytes
   eot
   total length: 26974240 bytes

================

All you REALLY need to do is ensure you use a blocksize larger
than tape blocksize on read, ie;

   dd if=/dev/rmt0 of=data bs=32k
-phil
*/

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <utime.h>

#include "backup.h"

#define bool long
#define false 0
#define true 1

#define RAWSIZE (5*(32+512))	/* logical block size */

#define endof(s) (strchr(s, (char) 0))

time_t unixtime();


bool eightbit = false;		/* Status of -8 (eight-bit) flag. */
bool oldnames = false;		/* Status of -o (old names) flag. */
bool ascii = false;		/* Status of -a (ascii) flag. */
bool copytape = false;		/* Status of -c (copytape fmt) flag. */
bool flatnames = false;		/* Status of -F (flat names) flag. */
bool raw36 = false;		/* Status of -r (raw 36 bit output) flag. */
bool interchange = false;	/* Status of -i (interchange) flag. */
long verbose = 0;		/* Status of -v (verbose) flag. */
long debug = 0;			/* Status of -D (debug) flag. */

char** argfiles;		/* File spec's to extract. */
long argcount;			/* Number of them. */

#ifdef STDIO
FILE *source;			/* Source "tape". */
unsigned char rawdata[RAWSIZE];	/* Raw data for a logical tape block. */
#else
int source;			/* Source "tape". */
unsigned char *rawdata;		/* Raw data for a logical tape block. */
unsigned char *rawbuf;		/* Raw data read buffer */
int rawsize;			/* size of rawbuf */
int rawcnt;			/* count of unprocessed char in rawbuf */
#endif

typedef struct {
    long lh, rh;
} word;

#define SETZM(W) (W).lh = (W).rh = 0

word head[32];			/* Header block from tape. */
word data[512];			/* Data block from tape. */

long prevSEQ;			/* SEQ number of previous block. */
long currentfilenumber;

char deferbyte;			/* Defered byte for output. */
long defercount;			/* Count of defered output bytes. */

bool extracting;
FILE* destination;
char* destpath;				/* destination path */

/* Tape information: */

char systemname[100];
char savesetname[100];

/* File information: */

long a_bsiz;				/* For now. */
long a_alls;
long a_mode;
word a_leng;
word a_prot;

word a_vers;
time_t a_writ;				/* creation time */
time_t a_redt;				/* last read time for this gen */
time_t a_modt;				/* last write */
time_t a_cret;				/* creation time for this gen */
char a_cusr[100];			/* creator */

char filedev[100];		/* Device: */
char filedir[100];		/* [ufd] */
char filename[100];		/* file name. */
char fileext[100];		/* extension. */

char filespec[7][100];		/* [0]: device:ufd. */
				/* [1-5]: sfd's, stored directly here. */
				/* [6]: file.ext */

char cname[100];		/* Canonical name. */

int density[] = { 0, 200, 556, 800, 1600, 6250 };

/* GETTAB %CNMNT encoding; */
char *montype[] = { "strange", "TOPS-10", "TENEX", "ITS", "TOPS-20" };

/* unpackheader unpacks the header block from the raw stream. */

void unpackheader() {
  unsigned char* rawptr;
  long i, left, right;
  unsigned char c;

  rawptr = &rawdata[0];

  for (i = 0; i < 32; i++) {
    left = *(rawptr++) << 10;
    left |= *(rawptr++) << 2;
    left |= (c = *(rawptr++)) >> 6;
    right = (c & 077) << 12;
    right |= *(rawptr++) << 4;
    right |= *(rawptr++) & 017;
    head[i].lh = left;
    head[i].rh = right;
    if(debug>1) {printf("\n%i %o,,%o",i,left,right);}
  }
}

/* unpackdata unpacks the data block from the raw stream. */

void unpackdata() {
  unsigned char* rawptr;
  long i, left, right;
  unsigned char c;

  rawptr = &rawdata[32*5];

  for (i = 0; i < 512; i++) {
    left = *(rawptr++) << 10;
    left |= *(rawptr++) << 2;
    left |= (c = *(rawptr++)) >> 6;
    right = (c & 077) << 12;
    right |= *(rawptr++) << 4;
    right |= *(rawptr++) & 017;
    data[i].lh = left;
    data[i].rh = right;
  }
}

/* pars_5chars reads five ASCII chars from a machine word. */

void pars_5chars(index, store)
long index;
char* store;
{
  long l, r;

  l = data[index].lh;
  r = data[index].rh;

  store[0] = (0177 & (l >> 11));
  store[1] = (0177 & (l >> 4));
  store[2] = (0177 & ((l << 3) | ((r >> 15) & 017)));
  store[3] = (0177 & (r >> 8));
  store[4] = (0177 & (r >> 1));
}

/* pars_asciz stores asciz text from data */

void pars_asciz(index, store)
long index;
char* store;
{
  long words;

  words = data[index++].rh;
  while ((words--) > 0) {
    pars_5chars(index++, store);
    store += 5;
  }
  *store = (char) 0;
}

/* pars_o_name parses an o$name block from data. */

void pars_o_name(index)
long index;
{
  long lastw;

  lastw = index + data[index].rh;
  ++index;
  while (index < lastw) {
    switch (data[index].lh) {
    case 0:  index = lastw; break;
    case 1:  pars_asciz(index, filedev);  break;
    case 2:  pars_asciz(index, filename); break;
    case 3:  pars_asciz(index, fileext);  break;
    case 32: pars_asciz(index, filedir);  break;
    case 33: pars_asciz(index, filespec[1]); break;
    case 34: pars_asciz(index, filespec[2]); break;
    case 35: pars_asciz(index, filespec[3]); break;
    case 36: pars_asciz(index, filespec[4]); break;
    case 37: pars_asciz(index, filespec[5]); break;
    default:
	if (debug) {
	    fprintf(stderr, "O$NAME: %d (len %d)\n",
		    data[index].lh, data[index].rh);
	}
	break;
    }
    index += data[index].rh;
  }
}

/*
 * LoaD a Byte of "siz" bits with "pos" bits to the left of it
 * XXX OY!! LDB instruction takes bit to the RIGHT!!! -p
 */

/* use a table? */
#define MASK(SIZ) ((1<<(SIZ))-1)

long
ldb(w, pos, siz)
    word w;
    int pos, siz;
{
    int s1, s2;

    if (pos > 35 || pos < 0 || siz < 1 ||
	pos+siz > 36 || siz > 8*sizeof(long) ) {
	return 0;			/* error */
    }

    if (pos >= 18) {			/* in the right half */
	return (w.rh >> (36-pos-siz)) & MASK(siz);
    }
    if (pos+siz <= 17) {		/* in the left half */
	return (w.lh >> (18-pos-siz)) & MASK(siz);
    }
    /* here if byte spans halves */
    s1 = 18 - pos;			/* siz of portion in lh */
    s2 = siz - s1;

    return ((w.lh & MASK(s1)) << s2) | ((w.rh >> (18-s2)) & MASK(s2));
}

/*
 * interpret owner/group/world fields in A$PROT
 * return printing char for display
 */

char
prot(x)
    char x;
{
    char attr, wr, rd;

    if (x & 0x80)			/* special? */
	return '!';

    /* first try a list of (empirical) canned values */
    switch (x) {
    case 0x6e:
	return '0';			/* change protection */
    case 0x7e:
	return '1';			/* rename */
    case 0x2e:
	return '2';			/* write */
    case 0x2a:
	return '3';			/* update */
    case 0x26:
	return '4';			/* append */
    case 0x22:
	return '5';			/* read */
    case 0x11:
	return '6';			/* execute */
    case 0x10:
	return '7';			/* no access */
    case 0:
	return '*';			/* for interchange?? */
    }

    /* now try interpreting fields; MUST STAY IN ORDER */

    if (debug)
	fprintf(stderr, "prot: %#x\n", x);

    rd = x & 3;
    if (rd == 0)		/* read: no access (7)*/
	return '7';
    if (rd == 1)		/* read: execute only (6) */
	return '6';

    wr = (x>>2) & 3;
    if (wr == 0)		/* write: none (7-5) */
	return '5';
    if (wr == 1)		/* write: append (4) */
	return '4';
    if (wr == 2)		/* write: write (3) */
	return '3';

    attr = (x>>4) & 7;
    if (attr == 7)		/* attr: change prot (0) */
	return '0';
    if (attr == 6)		/* attr: delete file (1) */
	return '1';
    if (wr == 1 ||		/* write: supercede */
	attr == 2) {		/* attr: visible (5-2) */
	return '2';
    }

    /* stumpted!! */
    printf("attr: %d wrt: %d red: %d\n", attr, rd, wr);
    return '?';
}

void pars_o_attr(index)
long index;
{
  int hlen;			/* header len */

  /* parse off O$FILE attribute block */
  ++index;

  a_writ = 0;
  a_alls = 0;
  a_mode = 0;
  SETZM(a_leng);
  a_bsiz = 0;
  SETZM(a_vers);
  SETZM(a_prot);
  a_cret = 0;
  a_redt = 0;
  a_modt = 0;
  a_cusr[0] = '\0';

  hlen = data[index + A_FHLN].rh;

  if (hlen < A_WRIT)
      return;
  a_writ = unixtime(data[index + A_WRIT]);

  if (hlen < A_ALLS)
      return;
  a_alls = data[index + A_ALLS].rh;	/* for now... */

  if (hlen < A_MODE)
      return;
  a_mode = data[index + A_MODE].rh;	/* for now... */

  if (hlen < A_LENG)
      return;
  a_leng = data[index + A_LENG];

  if (hlen < A_BSIZ)
      return;
  a_bsiz = data[index + A_BSIZ].rh;	/* for now... */

  if (hlen < A_VERS)
      return;
  a_vers = data[index + A_VERS];

  if (hlen < A_PROT)
      return;
  a_prot = data[index + A_PROT];

  if (hlen < A_CRET)
      return;
  a_cret = unixtime(data[index + A_CRET]); /* generation creation */

  if (hlen < A_REDT)
      return;
  a_redt = unixtime(data[index + A_REDT]); /* generation last read */

  if (hlen < A_MODT)
      return;
  a_modt = unixtime(data[index + A_MODT]); /* last write */

  if (hlen < A_CUSR)
      return;
  if (data[index + A_CUSR].rh && data[index + A_CUSR].lh == 0440700) {
      int i;
      char *cp;

      i = index+data[index + A_CUSR].rh;
      cp = a_cusr;
      for (;;) {			/* XXX while (i < hlen)??? */
	  pars_5chars(i++, cp);
	  if (*cp == 0 || cp[1] == 0 | cp[2] == 0 || cp[3] == 0 || cp[4] == 0)
	      break;
	  cp += 5;
      }
      cp = strchr(a_cusr, '_');
      if (cp)
	  *cp = ',';
  } /* have CUSR BP */
}

void pars_o_dirt(index)
long index;
{
  /* parse off directory attribute block */
}

void pars_o_sysn(index)
long index;
{
  pars_asciz(index, systemname);
}

void pars_o_ssnm(index)
long index;
{
  pars_asciz(index, savesetname);
}

void zerotapeinfo() {
  systemname[0] = (char) 0;
  savesetname[0] = (char) 0;
}

void zerofileinfo() {

  filedev[0]  = (char) 0;
  filedir[0]  = (char) 0;
  filename[0] = (char) 0;
  fileext[0]  = (char) 0;

  filespec[0][0] = (char) 0;
  filespec[1][0] = (char) 0;
  filespec[2][0] = (char) 0;
  filespec[3][0] = (char) 0;
  filespec[4][0] = (char) 0;
  filespec[5][0] = (char) 0;
  filespec[6][0] = (char) 0;

  cname[0] = (char) 0;
}

/* unpackinfo picks non-data information from data block. */

void unpackinfo() {
  long index;

  unpackdata();

  index = 0;
  while (index < head[G_LND].rh) {
    switch (data[index].lh) {
    case O_NAME: pars_o_name(index); break;
    case O_ATTR: pars_o_attr(index); break;
    case O_DIRECT: pars_o_dirt(index); break;
    case O_SYSNAME: pars_o_sysn(index); break;
    case O_SAVESET: pars_o_ssnm(index); break;
    default:
	if (debug) {
	    fprintf(stderr, "item type %d\n", data[index].lh);
	}
	break;
    }
    index += data[index].rh;
  }
}

void
printvers(w)				/* -phil */
    word w;
{
    int min, who;

    printf("%o", ldb(w, 3, 9));
    min = ldb( w, 12, 6);
    if (min) {				/* minor version */
	if (min > 26) {
	    putchar('A' + (min/26) - 1);
	    min %= 26;
	}
	putchar('A' + min - 1);
    }
    printf("(%o)", w.rh);		/* edit number */
    who = ldb(w, 0, 3);			/* who modified */
    if (who)
	printf("-%o", who);
}

void
printsixbit(w)				/* -phil */
    word w;
{
    char buf[7], *bp;

    /* split into pars_6chars/printsixbit? */

    bp = buf;
    *bp++ = ((w.lh >> 12) & 077) + ' ';
    *bp++ = ((w.lh >>  6) & 077) + ' ';
    *bp++ =  (w.lh        & 077) + ' ';
    *bp++ = ((w.rh >> 12) & 077) + ' ';
    *bp++ = ((w.rh >>  6) & 077) + ' ';
    *bp++ =  (w.rh        & 077) + ' ';
    *bp = '\0';

    printf("%s", buf);
}

static char *months[] = {
    "Jan", "Feb", "Mar",
    "Apr", "May", "Jun",
    "Jul", "Aug", "Sep",
    "Oct", "Nov", "Dec"
};

char *
_fmtdate(date, tm)		/* -phil */
    char *date;
    struct tm *tm;
{
    /* NOTE: strftime %d uses %02d! */
    sprintf( date, "%2d-%s-%d",
	    tm->tm_mday, months[tm->tm_mon], tm->tm_year);
    return date;
}

char *
fmtdate(date,t)			/* -phil */
    char *date;
    time_t t;
{
    struct tm *tm;

    if (t) {
	tm = gmtime(&t);	/* NOTE: NOT local! */
	if (tm) {
	    return _fmtdate(date,tm);
	}
    }
    return "XX-XXX-XX";		/* ?! */
}

char *
_fmtdatetime(date, tm)		/* -phil */
    char *date;
    struct tm *tm;
{
    /* NOTE: strftime %d uses %02d! */
    sprintf( date, "%2d-%s-%d %2d:%02d:%02d",
	    tm->tm_mday, months[tm->tm_mon], tm->tm_year,
	    tm->tm_hour, tm->tm_min, tm->tm_sec);
    return date;
}

char *
fmtdatetime(buf, t)			/* -phil */
    char *buf;
    time_t t;
{
    char date[64];
    struct tm *tm;

    tm = gmtime(&t);			/* NOTE: NOT local! */
    if (tm) {
	return _fmtdatetime(date,tm);
    }
    return "XX-XXX-XX XX:XX:XX";
}

void
printdatetime(t)			/* -phil */
    time_t t;
{
    char date[64];

    printf("%s", fmtdatetime(date,t));
}

void printtapeinfo(type) {
  if (verbose) {
    switch (type) {
    case T_BEGIN:
	printf("\n");
	printf("Start");
	break;
    case T_CONT:
	printf("\n");
	printf("Continuation");
	break;
    case T_END:
	printf("\n");
	printf("End");
	break;
    }
    printf(" of Saveset");
    if (*savesetname != (char) 0) {
	printf(" %s", savesetname);
    }
    printf(" on ");
    printsixbit(head[S_DEVICE]);
    if (head[S_REELID].lh) {
	printf(" ");
	printsixbit(head[S_REELID]);
    }
    printf("\n");

    if (*systemname != (char) 0) {
	printf("System %s ", systemname);
    }

    printf("%s monitor ", montype[head[S_MONTYP].rh >> 12]); /* per %CNMNT */
    printvers(head[S_SVER]);	/* per %CNDVN GETTAB */
    printf(" APR#%d\n", head[S_APR].rh);

    printf("%d BPI %d-track ",
	   density[head[S_MTCHAR].rh & 7],
	   ((head[S_MTCHAR].rh & 020) ? 7 : 9));

    printdatetime(unixtime(head[S_DATE]));
    printf(" BACKUP ");
    printvers(head[S_BVER]);
    printf(" tape format %d\n", head[S_FORMAT].rh);

    printf("Tape number %2d\n",  head[G_RTNM].rh);

    printf("\n");
  }
}

void downcase(s)
char* s;
{
  while (*s != (char) 0) {
    if (isupper(*s)) *s = tolower(*s);
    s++;
  }
}

void buildfilenames() {
  long i;

  if (oldnames) {
      if (*filedev != (char) 0)
	  sprintf(filespec[0], "%s:%s", filedev, filedir);
      else
	  sprintf(filespec[0], "%s", filedir);

      sprintf(filespec[6], "%s.%s", filename, fileext);

      for(i = 0; i < 7; i++)
	  downcase(filespec[i]);

      sprintf(cname, "%s", filespec[0]);
      for(i = 1; i < 6; i++) {
	  if (*filespec[i] != (char) 0)
	      sprintf(endof(cname), ".%s", filespec[i]);
      }
      if (*cname != (char) 0)
	  sprintf(endof(cname), "..%s", filespec[6]);
      else
	  sprintf(cname, "%s", filespec[6]);
  } /* oldnames */
  else {
      char *cp;

      /* NEVER include device in created directories, mash '_' back to comma */
      sprintf(filespec[0], "%s", filedir);
      cp = strchr(filespec[0], '_');
      if (cp)
	  *cp = ',';

      sprintf(filespec[6], "%s.%s", filename, fileext);

      for(i = 0; i < 7; i++)
	  downcase(filespec[i]);

      cname[0] = '\0';
      if (filedev[0]) {
	  strcat(cname, filedev);
	  downcase(cname);
	  strcat(cname, ":");
      }
      strcat(cname, "[");
      strcat(cname, filespec[0]);	/* massaged version of filedir */
      for(i = 1; i < 6; i++) {
	  if (*filespec[i] != (char) 0) {
	      strcat(cname, ",");
	      strcat(cname, filespec[i]);
	  }
	  /* XXX else break? */
      }
      if (*cname)
	  strcat(cname, "]");
      strcat(cname, filespec[6]);
  } /* not oldnames */
}

void printfileinfo() {
  buildfilenames();

#ifdef NEW
  if (verbose != 1)
      printf("%3d", currentfilenumber);
#else
  printf("%3d  %s", currentfilenumber, cname);
#endif
  if (verbose) {
#ifdef NEW
     unsigned long words, chars;
     char date[32];

#ifdef LONG64
     words = chars = (a_leng.lh<<18) + a_leng.rh;
#else
     words = chars = ((a_leng.lh & 037777)<<18) + a_leng.rh;
#endif

     switch (a_bsiz) {
     case 7:
	 words /= 5;
	 break;
     case 8:			/* possible in 7.03 w/ .IOAS8? */
	 words /= 4;
	 break;
     }
     printf("%5d", (words+127)/128); /* blocks */
     printf("%9d(%d)", chars, a_bsiz);
     if (a_bsiz < 10)
	 putchar(' ');

     printf(" <%c%c%c>",
	    prot(ldb(a_prot, 12, 8)),
	    prot(ldb(a_prot, 20, 8)),
	    prot(ldb(a_prot, 28, 8)));
     printf(" %-8.8s", a_cusr);
     if (verbose > 1)
	 printf(" %s", fmtdatetime(date, a_writ)); /* original creation date */
     else
	 printf(" %s", fmtdate(date, a_writ)); /* original creation date */
#else
     printf(" (%d) alloc:%d, mode:%o, len:%d", a_bsiz, a_alls, a_mode,
	    a_leng.rh);
#endif
  }
#ifdef NEW
  putchar(' ');
  printf("%s", cname);
  if (verbose && (a_vers.rh || a_vers.lh)) {
      printf(" ");
      printvers(a_vers);
  }
#endif

  printf("\n");
}

/* readblock reads one logical block from the input stream. */
/* returns true if read ok */
/* The header is unpacked into head; the data is not. */

long blockn=0;

int
readblock() {
  long i;

#ifdef STDIO
  int l1, l2;

  if (feof(source))
      return 0;
  if (copytape) {
      l1 = getc(source);
      if (l1 == EOF)
	  return 0;
      l2 = getc(source);
      if (l2 == EOF)
	  return 0;
      l1 += l2 << 8;
  }
  else
      l1 = RAWSIZE;

  i = fread( rawdata, 1, l1, source);
#else
  if (rawcnt <= 0) {		/* need a refill? */
      rawcnt = read(source, rawbuf, rawsize); /* take a deep breath */
      if (rawcnt == 0)
	  return false;		/* EOF */
      rawdata = rawbuf;		/* reset pointer */
  }
  else
      rawdata += RAWSIZE;	/* step to next logical block */

  i = rawcnt;
  rawcnt -= RAWSIZE;
#endif
  while (i < RAWSIZE)
      rawdata[i++] = (char) 0;
  blockn++;
  unpackheader();
  return true;			/* ok */
}

/* Disk file output routines: */
static bool sawcr;

void WriteBlock() {
  char buffer[5*512];
  long bufpos, index;

  if (raw36) {
      long off, len;

      off = head[G_LND].rh;		/* data words to skip */
      len = head[G_SIZE].rh;		/* words of data */
      if (len) {
	  (void) fwrite(rawdata+(off+32)*5, 5, len, destination);
      }
      return;
  }

  unpackdata();

  for (index = head[G_LND].rh, bufpos = 0;
       index < (head[G_LND].rh + head[G_SIZE].rh); index++) {
    pars_5chars(index, &buffer[bufpos]);
    bufpos += 5;
  }

  if (head[G_FLAGS].lh & GF_EOF) {
    for (index = 1; index < (eightbit ? 4 : 5); index++) {
      if (buffer[bufpos - 1] == (char) 0) bufpos--;
    }
  }

  if (ascii) {
      /* if !sawcr, scan for CRs, fall into fwrite if none found?? */
      char *bp;

      bp = buffer;
      while (bufpos-- > 0) {
	  if (sawcr) {
	      sawcr = false;
	      if (*bp != '\n') {
		  putc('\r', destination);
	      }
	  }
	  if (*bp == '\r') {
	      sawcr = true;
	      bp++;
	      continue;
	  }
	  putc(*bp, destination);
	  bp++;
      }
      return;
  }
  (void) fwrite(buffer, sizeof(char), bufpos, destination);
}

/* OpenOutput opens the output file, according to -d and -i flags. */

bool OpenOutput() {

  struct stat statbuf;
  static char oname[100];
  long i;

  defercount = 0;

  if (interchange) {
    destpath = filespec[6];
  } else if (flatnames) {
    destpath = cname;
  } else {
    for(i = 0, oname[0] = (char) 0; i < 6; i++) {
      if (*filespec[i] == (char) 0) break;
      sprintf(endof(oname), "%s", filespec[i]);
      if (stat(oname, &statbuf) != 0) {
	if (mkdir(oname, 0777) != 0) {
	  fprintf(stderr, "backup: cannot create %s/\n", oname);
	  return(false);
	}
      }
      sprintf(endof(oname), "/");
    }
    sprintf(endof(oname), "%s", filespec[6]);
    destpath = oname;
    sawcr = false;
  }

  destination = fopen(destpath, "w");
  return(destination != NULL);
}

void CloseOutput() {
  /* Close output file after us. */
}

/* Argmatch checks if the current file matches the given argument: */

bool argmatch(arg)
char* arg;
{
  long target;
  char* f;
  char* p;
  char* s;

  if (*arg == '#') {
    (void) sscanf(arg, "#%d", &target);
    return(target == currentfilenumber);
  }

  if (*arg == '*') return(1);

  for (f = cname; *f != (char) 0; f++) {
    for (p = f, s = arg; (*s != (char) 0) && (*p == *s); p++, s++);
    if (*s == (char) 0) return (true);
    /* XXX check for '*\0' ?? XXX DO REAL GLOBBING!! */
  }
  return (false);
}

/* doextract performs the job of "backup -x ..." */

void doextract() {
  long i;

  currentfilenumber = 0;
  extracting = false;
  while (readblock()) {
    if (head[G_SEQ].rh == prevSEQ) continue;

    if (head[G_TYPE].rh == T_FILE) {
      if (head[G_FLAGS].lh & GF_SOF) {
	currentfilenumber++;
	zerofileinfo();
	unpackinfo();
	buildfilenames();
	for (i = 0; i < argcount; i++) {
	  if (argmatch(argfiles[i])) {
	    if (*argfiles[i] == '#') {
	      /* Maybe do a pure shift here? */
	      argfiles[i] = argfiles[--argcount];
	    }
	    extracting = true;
	    break;
	  }
	}
	if (extracting) {
	  if (OpenOutput()) {
	    if (verbose) {
	      printf("Extracting %s", cname);
	      fflush(stdout);
	    }
	  } else {
	    fprintf(stderr, "backup: can't open %s for output\n", cname);
	    extracting = false;
	  }
	}
      }
      if (extracting) {
	WriteBlock();
	if (head[G_FLAGS].lh & GF_EOF) {
	  struct utimbuf ut;

	  (void) fclose(destination);

	  ut.actime = a_redt;		/* access */
	  ut.modtime = a_writ;		/* modify */
	  /* XXX check for zeroes, use current? */

	  (void) utime( destpath, &ut );
	  extracting = false;
	  if (verbose) printf("\n");
	  if (argcount == 0)
	    break;
	}
      }
    }
    prevSEQ = head[G_SEQ].rh;
  }
}

/* dodirectory performs the job of "backup -t ..." */

void dodirectory() {

  currentfilenumber = 0;

  while (readblock()) {
    if (head[G_SEQ].rh == prevSEQ) continue;

    switch (head[G_TYPE].rh) {
    case T_BEGIN:
    case T_CONT:
    case T_END:
      zerotapeinfo();
      unpackinfo();
      printtapeinfo(head[G_TYPE].rh);
      break;
    case T_FILE:
      if (head[G_FLAGS].lh & GF_SOF) {
	++currentfilenumber;
	zerofileinfo();
	unpackinfo();
	printfileinfo();
      }
      break;
    case T_UFD:
      if (debug>1) {
	  puts("T$UFD");
      }
      break;
    default:
      if (debug)
	  fprintf(stderr, "record type %d\n", head[G_TYPE].rh);
      break;
    } /* switch */
    prevSEQ = head[G_SEQ].rh;
  }
}

/* command decoder and dispatcher */

void checkarg(arg)
char* arg;
{
  long i;
  char c;

  if (*arg == '#') {
    if (sscanf(arg, "#%d%c", &i, &c) != 1) {
      fprintf(stderr, "backup: bad argument: %s\n", arg);
      exit(1);
    }
  }
}


void main(argc, argv)
long argc;
char* argv[];
{
  long i;
  char* s;
  bool namenext = false;
  bool actgiven = false;
  char action;
  char* inputname = NULL;
  struct stat st;
  char *flags;

  if (--argc > 0) {
    for (s = *(++argv); *s != (char) 0; s++)
      switch(*s) {
      case '-':
	break;
#if 0
      case '8':
	eightbit = true;  break;
#endif
      case 'a':				/* extract text files; munch CR's */
	ascii = !ascii;
	break;
      case 'c':
	copytape = true; break;
      case 'F':
	flatnames = !flatnames; break;
      case 'f':
	namenext = true;  break;
      case 'i':
	interchange = true;  break;
      case 'o':
	oldnames = !oldnames; break;
      case 'r':
	raw36 = !raw36; break;
      case 't':
      case 'x':
	if (actgiven && action != *s) {
	    fprintf(stderr, "backup: incompatible actions given; %c/%c\n",
		    action, *s);
	    exit(1);
	}
	action = *s;  actgiven = true;  break;
      case 'v':
	  verbose++;  break;
      case 'D':
	  debug++;  break;
      default:
	fprintf(stderr, "backup: bad option %c\n", *s);
	exit(1);
      }
  }

  /* XXX error to specify both raw36 && ascii?? */

  if (namenext) {
    if (--argc > 0)
      inputname = *(++argv);
    else {
      extern char *getenv();

      inputname = getenv("TAPE");
      if (inputname == NULL) {
	  fprintf(stderr, "backup: input file name missing\n");
	  exit(1);
      }
    }
  }

  argfiles = ++argv;		/* Keep rest of arguments. */
  argcount = --argc;		/* ... and count 'em. */

/* XXX complain if argcount == 0? */

  for (i = 0; i < argcount; i++)
    checkarg(argfiles[i]);

  if (inputname == NULL) {
    /* Use environment var. TAPE here? */
    fprintf(stderr, "backup: no input file given\n");
    exit(1);
  }

#ifdef STDIO
  if (strcmp(inputname, "-") != 0) {
    if ((source = fopen(inputname, "r")) != NULL) {
      fprintf(stderr, "backup: can't open %s for input\n", inputname);
      exit(1);
    }
  } else {
    source = stdin;
  }
#else
  if (strcmp(inputname, "-") != 0) {
    if ((source = open(inputname, 0)) < 0) {
      fprintf(stderr, "backup: can't open %s for input\n", inputname);
      exit(1);
    }
  } else {
    source = 0;			/* STDIN_FILENO in unistd.h */
  }

  /* try to pick a blocking factor that is a multiple
   * of the underlying device's blocksize and tape logical blocksize
   */
  rawsize = RAWSIZE;
  if (fstat(0, &st) == 0)
      rawsize *= st.st_blksize;
  else
      rawsize *= 512;

  /* remove extra powers of two */
  {
      int i;
      i = RAWSIZE;
      while ((i&1) == 0) {
	  i >>= 1;
	  rawsize >>= 1;
      }
  }

  /* allocate buffer */
  for (;;) {
      rawbuf = (unsigned char *)malloc(rawsize);
      if (rawbuf)
	  break;
      rawsize >>= 1;		/* back off */
      if (rawsize < RAWSIZE || (rawsize % RAWSIZE) != 0) {
	  fprintf(stderr, "could not allocate rawbuf\n");
	  exit(1);
      }
  }
  rawcnt = 0;
#endif

  switch (action) {
  case 't': dodirectory(); break;
  case 'x': doextract(); break;
  default:
    fprintf(stderr, "backup: internal error in program\n");
    exit(1);
  }
  exit(0);
}

/* from read20.c, Jim Guyton, Rand Corporation */

#define DayBaseDelta 0117213            /* Unix day 0 in Tenex format */

/*
 * This screws up on some of the atime's we see, including, yielding, e.g.
 * Fri Dec 23 23:28:16 1994
 * Fri Dec 23 23:28:16 1994
 * Tue Jan 13 07:57:03 1987
 */
time_t
unixtime(w)
    word w;
{
    long t, s;

    t = w.lh;				/* days */
    s = w.rh;				/* fractions of a day */

    if (t == 0) return(0);		/* 0 means never referenced */

    t -= DayBaseDelta;			/* Switch to unix base */
					/* Now has # days since */
					/* Jan 1, 1970 */

#if 0
    if (t < 0) {
	fprintf(stderr, "ERROR - Date earlier than Jan 1,1970!!!\n");
    }
#endif

/* Note that the following calculation must be performed in the order shown
   in order to preserve precision.  It should also be done in double prec.
   floating point to prevent overflows.
 */
    s = (s * (double)24. * 60. * 60.) / 01000000; /* Turn into seconds */

    s += t*24*60*60;                        /* Add day base */
    return(s);
}
