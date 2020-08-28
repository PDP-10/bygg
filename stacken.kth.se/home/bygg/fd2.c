#include <stdio.h>

static char data[] = {
  "$*ABC4%StdStdStdStdStdStMAC@IPIABC6KRESERI@CC4JPDQEPDQ5JPDQEPDQsl@TRAXAS"
  "DS3+KSP!!PDQ2*OK%SBSs+OK&-piOK&-sfTD&-sfZ@$(sf)K2()@0-$937%+pC"
};

int main(int argc, char* argv[])
{
  int c, p = 0, s = sizeof(data) - 1;

  while (p < s) {
    c = (data[p++] >> 2) & 15;
    if ((c & 12) == 12) putchar('\n');
    if (c & 8) c = ((c & 3) << 4) + ((p < s) ? (data[p++] >> 2) & 15 : 0);
    while (c-->0) putchar(' ');
    if (p < s) putchar('*');
  }
  printf("\n");
}
