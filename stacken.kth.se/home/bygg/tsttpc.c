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
      if (2 == fread(buffer, 1, 2, f)) {
	len = buffer[0] + (buffer[1] << 8);
	printf("block: %d bytes\n", len);
	if (len > 0) {
	  (void) fread(buffer, 1, blocksize, f);
	}
      }
    }
  } else {
    fprintf(stderr, "Can't open %s for reading\n", argv[1]);
  }
}
