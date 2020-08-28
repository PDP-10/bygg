/*
** implement output formatting.
*/

#include "hexsim.h"

static int xflag = 0;

void bufxset(int flag)
{
  xflag = flag;
}

void bufchar(char c)
{
  if (xflag) {
    w_putc(c);
  } else {
    putchar(c);
  }
}

void bufstring(char* txt)
{
  char c;

  while ((c = *txt++) != (char) 0) {
    bufchar(c);
  }
}

void bufnumber(int number)
{
  char work[100];

  sprintf(work, "%d", number);
  bufstring(work);
}

void bufhex(unsigned int number, int n)
{
  char work[20];
  char fmt[10];

  sprintf(fmt, "%%0%ux", n);
  sprintf(work, fmt, number);
  bufstring(work);
}

void bufhw(hexaword hw)
{
  char work[20];
  sprintf(work, "%8lx,,%08lx",
	  (unsigned long) (hw >> 32),
	  (unsigned long) hw & 0xffffffff);
  bufstring(work);
}

void bufdec(hexaword hw, int n)
{
  char work[30];
  int len;

  sprintf(work, "%" PRIu64, hw);
  len = strlen(work);

  while (n > len) {
    n -= 1;
    bufchar(' ');
  }
  bufstring(work);
}

void bufnewline(void)
{
  bufchar('\n');
}
