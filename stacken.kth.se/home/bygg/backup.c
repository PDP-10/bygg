/* Dump contents of Tops-10 BACKUP tapes. */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "backup.h"

#define bool int
#define false 0
#define true 1

#define rawsize (5*(32+512))

#define endof(s) (strchr(s, (char) 0))

FILE* source;			/* Source "tape". */

bool eightbit = false;		/* Status of -8 (eight-bit) flag. */
bool copytape = false;		/* Status of -c (copytape fmt) flag. */
bool buildtree = false;		/* Status of -d (build trees) flag. */
bool interchange = false;	/* Status of -i (interchange) flag. */
bool convertcrlf = false;	/* Status of -m (^M^J => ^J) flag. */
bool verbose = false;		/* Status of -v (verbose) flag. */

char** argfiles;		/* File spec's to extract. */
int argcount;			/* Number of them. */

unsigned char rawdata[rawsize];	/* Raw data for a tape block. */

int headlh[32], headrh[32];	/* Header block from tape. */
int datalh[512], datarh[512];	/* Data block from tape. */

int prevSEQ;			/* SEQ number of previous block. */
int currentfilenumber;

bool extracting;
FILE* destination;

/* Tape information: */

char systemname[100];
char savesetname[100];

/* File information: */

int a_bsiz;				/* For now. */
int a_alls;
int a_mode;
int a_leng;

char filedev[100];		/* Device: */
char filedir[100];		/* [ufd] */
char filename[100];		/* file name. */
char fileext[100];		/* extension. */

char filespec[7][100];		/* [0]: device:ufd. */
				/* [1-5]: sfd's, stored directly here. */
				/* [6]: file.ext */

char cname[100];		/* Canonical name. */

/* unpackheader unpacks the header block from the raw stream. */

void unpackheader() {
  unsigned char* rawptr;
  int i, left, right;
  unsigned char c;

  rawptr = &rawdata[0];

  for (i = 0; i < 32; i++) {
    left = *(rawptr++) << 10;
    left |= *(rawptr++) << 2;
    left |= (c = *(rawptr++)) >> 6;
    right = (c & 077) << 12;
    right |= *(rawptr++) << 4;
    right |= *(rawptr++) & 017;
    headlh[i] = left;
    headrh[i] = right;
  }
}

/* unpackdata unpacks the data block from the raw stream. */

void unpackdata() {
  unsigned char* rawptr;
  int i, left, right;
  unsigned char c;

  rawptr = &rawdata[32*5];

  for (i = 0; i < 512; i++) {
    left = *(rawptr++) << 10;
    left |= *(rawptr++) << 2;
    left |= (c = *(rawptr++)) >> 6;
    right = (c & 077) << 12;
    right |= *(rawptr++) << 4;
    right |= *(rawptr++) & 017;
    datalh[i] = left;
    datarh[i] = right;
  }
}

/* pars_5chars reads five ASCII chars from a machine word. */

void pars_5chars(index, store)
int index;
char* store;
{
  int l, r;

  l = datalh[index];
  r = datarh[index];

  store[0] = (0177 & (l >> 11));
  store[1] = (0177 & (l >> 4));
  store[2] = (0177 & ((l << 3) | ((r >> 15) & 017)));
  store[3] = (0177 & (r >> 8));
  store[4] = (0177 & (r >> 1));
}

/* pars_asciz stores asciz text from data */

void pars_asciz(index, store)
int index;
char* store;
{
  int words;

  words = datarh[index++];
  while ((words--) > 0) {
    pars_5chars(index++, store);
    store += 5;
  }
  *store = (char) 0;
}

/* pars_o_name parses an o$name block from data. */

void pars_o_name(index)
int index;
{
  int lastw;

  lastw = index + datarh[index];
  ++index;
  while (index < lastw) {
    switch (datalh[index]) {
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
    }
    index += datarh[index];
  }
}

void pars_o_attr(index)
int index;
{
  /* parse off file attribute block */
  ++index;
  a_bsiz = datarh[index + A_BSIZ];	/* for now... */
  a_alls = datarh[index + A_ALLS];	/* for now... */
  a_mode = datarh[index + A_MODE];	/* for now... */
  a_leng = datarh[index + A_LENG];	/* for now... */
}

void pars_o_dirt(index)
int index;
{
  /* parse off directory attribute block */
}

void pars_o_sysn(index)
int index;
{
  pars_asciz(index, systemname);
}

void pars_o_ssnm(index)
int index;
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
  int index;

  unpackdata();

  index = 0;
  while (index < headrh[G_LND]) {
    switch (datalh[index]) {
    case 1: pars_o_name(index); break;
    case 2: pars_o_attr(index); break;
    case 3: pars_o_dirt(index); break;
    case 4: pars_o_sysn(index); break;
    case 5: pars_o_ssnm(index); break;
    }
    index += datarh[index];
  }
}

void printtapeinfo() {
  if (verbose) {
    if (*savesetname != (char) 0) printf("Saveset name: %s\n", savesetname);
    if (*systemname != (char) 0) printf("Written on: %s\n", systemname);
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
  int i;

  if (*filedev != (char) 0)
    sprintf(filespec[0], "%s:%s", filedev, filedir);
  else
    sprintf(filespec[0], "%s", filedir);

  sprintf(filespec[6], "%s.%s", filename, fileext);

  for(i = 0; i < 7; i++)
    downcase(filespec[i]);

  sprintf(cname, "%s", filespec[0]);
  for(i = 1; i < 6; i++) {
    if (*filespec[i] != (char) 0) sprintf(endof(cname), ".%s", filespec[i]);
  }
  if (*cname != (char) 0)
    sprintf(endof(cname), "..%s", filespec[6]);
  else
    sprintf(cname, "%s", filespec[6]);

}

void printfileinfo() {

  buildfilenames();
  printf("%3d  %s", currentfilenumber, cname);
  if (verbose) {
  /* 
   * printf(" (%d) alloc:%d, mode:%o, len:%d", a_bsiz, a_alls, a_mode, a_leng);
   */
  }
  printf("\n");
}


/* readblock reads one logical block from the input stream. */
/* The header is unpacked into head{l,r}; the data is not. */

void readblock() {
  int i;
  i = fread(rawdata, sizeof(char), rawsize, source);
  while (i++ < rawsize) rawdata[i] = (char) 0;
  unpackheader();
}

/* dodirectory performs the job of "backup -t ..." */

void dodirectory() {

  currentfilenumber = 0;

  while (!feof(source)) {
    readblock();
    if (headrh[G_SEQ] == prevSEQ) continue;

    if (headrh[G_TYPE] == T_BEGIN) {
      zerotapeinfo();
      unpackinfo();
      printtapeinfo();
    }
    if (headrh[G_TYPE] == T_FILE) {
      if (headlh[G_FLAGS] & GF_SOF) {
	++currentfilenumber;
	zerofileinfo();
	unpackinfo();
	printfileinfo();
      }
    }
    prevSEQ = headrh[G_SEQ];
  }
}

bool argmatch(arg)
char* arg;
{
  int target;
  char* f;
  char* p;
  char* s;

  if (*arg == '#') {
    (void) sscanf(arg, "#%d", &target);
    return(target == currentfilenumber);
  }
  for (f = cname; *f != (char) 0; f++) {
    for (p = f, s = arg; (*s != (char) 0) && (*p == *s); p++, s++);
    if (*s == (char) 0) return (true);
  }
  return (false);
}

void writebuffer() {
  char buffer[5*512];
  int bufpos, index;

  for (index = headrh[G_LND], bufpos = 0;
       index < (headrh[G_LND] + headrh[G_SIZE]); index++) {
    pars_5chars(index, &buffer[bufpos]);
    bufpos += 5;
  }

  if (headlh[G_FLAGS] & GF_EOF) {
    for (index = 1; index < (eightbit ? 4 : 5); index++) {
      if (buffer[bufpos - 1] == (char) 0) bufpos--;
    }
  }

  (void) fwrite(buffer, sizeof(char), bufpos, destination);

}

/* OpenOutput opens the output file, according to -d and -i flags. */

bool OpenOutput() {

  struct stat statbuf;
  char oname[100];
  int i;

  if (interchange) {
    destination = fopen(filespec[6], "w");
  } else if (!buildtree) {
    destination = fopen(cname, "w");
  } else {
    for(i = 0, oname[0] = (char) 0; i < 6; i++) {
      if (*filespec[i] == (char) 0) break;
      sprintf(endof(oname), "%s", filespec[i]);
      if (stat(oname, &statbuf) != 0) {
	if (mkdir(oname, 0777) != 0) {
	  sprintf(stderr, "backup: cannot create %s/\n", oname);
	  return(false);
	}
      }
      sprintf(endof(oname), "/");
    }
    sprintf(endof(oname), "%s", filespec[6]);
    destination = fopen(oname, "w");
  }

  return(destination != NULL);
}

/* doextract performs the job of "backup -x ..." */

void doextract() {
  int i;

  currentfilenumber = 0;
  extracting = false;
  while (!feof(source)) {
    readblock();
    if (headrh[G_SEQ] == prevSEQ) continue;
    if (headrh[G_TYPE] == T_FILE) {
      if (headlh[G_FLAGS] & GF_SOF) {
	currentfilenumber++;
	zerofileinfo();
	unpackinfo();
	buildfilenames();
	for (i = 0; i < argcount; i++) {
	  if (argmatch(argfiles[i])) {
	    if (*argfiles[i] == '#')
	      argfiles[i] = argfiles[--argcount];
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
	unpackdata();
	writebuffer();
	if (headlh[G_FLAGS] & GF_EOF) {
	  (void) fclose(destination);
	  extracting = false;
	  if (verbose) printf("\n");
	  if (argcount == 0)
	    break;
	}
      }
    }
    prevSEQ = headrh[G_SEQ];
  }
}

/* command decoder and dispatcher */


void checkarg(arg)
     char* arg;
{
  int i;
  char c;

  if (*arg == '#') {
    if (sscanf(arg, "#%d%c", &i, &c) != 1) {
      fprintf(stderr, "backup: bad argument: %s\n", arg);
      exit(1);
    }
  }
}


void main(argc, argv)
int argc;
char* argv[];
{
  int i;
  char* s;
  bool namenext = false;
  bool actgiven = false;
  char action;
  char* inputname = NULL;

  if (--argc > 0) {
    for (s = *(++argv); *s != (char) 0; s++)
      switch(*s) {
      case '-':
	break;
      case '8':
	eightbit = true;  break;
      case 'c':
	copytape = true; break;
      case 'd':
	buildtree = true;  break;
      case 'f':
	namenext = true;  break;
      case 'i':
	interchange = true;  break;
      case 'm':
	convertcrlf = true;  break;
      case 't':
      case 'x':
	action = *s;  actgiven = true;  break;
      case 'v':
	verbose = true;  break;
      default:
	fprintf(stderr, "backup: bad option %c\n", *s);
	exit(1);
      }
  }
  if (namenext) {
    if (--argc > 0)
      inputname = *(++argv);
    else {
      fprintf(stderr, "backup: input file name missing\n");
      exit(1);
    }
  }

  argfiles = ++argv;		/* Keep rest of arguments. */
  argcount = --argc;		/* ... and count 'em. */

  for (i = 0; i < argcount; i++)
    checkarg(argfiles[i]);

  if (inputname == NULL) {
    /* Use environment var. TAPE here? */
    fprintf(stderr, "backup: no input file given\n");
    exit(1);
  }

  if (strcmp(inputname, "-") != 0) {
    if ((source = fopen(inputname, "r")) == NULL) {
      fprintf(stderr, "backup: can't open %s for input\n", inputname);
      exit(1);
    }
  } else {
    source = stdin;
  }

  switch (action) {
  case 't': dodirectory(); break;
  case 'x': doextract(); break;
  default:
    fprintf(stderr, "backup: internal error in program\n");
    exit(1);
  }

}
