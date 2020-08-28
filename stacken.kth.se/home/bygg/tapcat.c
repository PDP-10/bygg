#include <stdio.h>

typedef unsigned char byte;

#define maxblock 65536		/* Max. block size. */

int filecount = 0;
int blocksize = 256;

static void putsize(int size)
{
  putchar(size & 0xff);
  putchar((size >> 8) & 0xff);
  putchar((size >> 16) & 0xff);
  putchar((size >> 24) & 0x7f);
}

static void tapemark(void)
{
  putsize(0);
}

static void setsize(int newsize)
{
  if (newsize <= 0) {
    fprintf(stderr, "block size %d too small, ignored.\n", newsize);
  } else if (newsize > maxblock) {
    fprintf(stderr, "block size %d too large, ignored.\n", newsize);
  } else {
    blocksize = newsize;
  }
}

static void dofile(char* filename)
{
  static byte buffer[maxblock];
  int count;
  int i;
  FILE* f;

  if ((f = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s for input.\n", filename);
  } else {
    filecount += 1;
    for (;;) {
      count = fread(buffer, sizeof(byte), blocksize, f);
      if (count == 0) break;
      if (count < blocksize) {
	(void) memset(&buffer[count], 0, (blocksize - count));
      }
      putsize(blocksize);
      for (i = 0; i < blocksize; i += 1) {
	putchar(buffer[i]);
      }
      if (blocksize & 1) {	/* block size odd, use a pad byte. */
	putchar(0);
      }
      putsize(blocksize);
    }
    tapemark();
    (void) fclose(f);
  }
}

int main(int argc, char* argv[])
{
  int i;

  for (i = 1; i < argc; i += 1) {
    if (isdigit(argv[i][0])) {
      setsize(atoi(argv[i]));
    } else {
      dofile(argv[i]);
    }
  }
  if (filecount == 0) {
    tapemark();
  }
  tapemark();

  return 0;
}
