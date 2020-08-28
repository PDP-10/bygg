/*
** implement output formatting.
*/

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

void bufhex(int number, int n)
{
  char work[20];
  char fmt[10];

  sprintf(fmt, "%%0%ux", n);
  sprintf(work, fmt, number);
  if ((n == 0) && (work[0] >= 'a')) {
    bufchar('0');
  }
  bufstring(work);
}

void bufnewline(void)
{
  bufchar('\n');
}
