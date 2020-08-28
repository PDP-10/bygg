#include <stdio.h>

#define blocksize ((32 + 512) * 5)

typedef unsigned char byte;

int main(int argc, char* argv[])
{
  byte buffer[blocksize];
  FILE* f;
  int len;

  if ((f = fopen(argv[1], "r")) != NULL) {
    while (!feof(f)) {
      if (4 == fread(buffer, 1, 4, f)) {
	len = buffer[0]
	  + (buffer[1] << 8)
	  + (buffer[2] << 16)
	  + (buffer[3] << 24);
	if (len > 0) {
	  if (len <= 2720) {
	    (void) fread(buffer, 1, len, f);
	  }
	  if (len == 2720) {
	    (void) fwrite(buffer, 1, 2720, stdout);
	  }
	}
	(void) fread(buffer, 1, 4, f);
      }
    }
  } else {
    fprintf(stderr, "Can't open %s for reading\n", argv[1]);
  }
}
