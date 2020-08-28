I8051:

static void defb(byte b)
{
  pb_length = 1;
  pb_status = st_byte;
  startline(true);
  casestring("defb");
  spacedelim();
  number(b);
  checkblank();
}

static void i8051_dobyte(void)
{
  defb(getbyte());
}

I8086:

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  usselect("db", ".byte");
  spacedelim();
  number(b);
  checkblank();
}

static void i8086_dobyte(void)
{
  defb(getbyte());
}

M6502:

static void defb(byte b)
{
  pb_length = 1;
  pb_status = st_byte;
  startline(true);
  casestring("defb");
  spacedelim();
  number(b);
  checkblank();
}

static void m6502_dobyte(void)
{
  defb(getbyte());
}

M6800:

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

static void m6800_dobyte(void)
{
  defb(getbyte());
}

M6805:

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

static void m6805_dobyte(void)
{
  defb(getbyte());
}

M6809:

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  casestring(".byte");
  spacedelim();
  number(b);
  checkblank();
}

static void m6809_dobyte(void)
{
  defb(getbyte());
}

M68K:

static void defb(byte b)
{
  pb_status = st_byte;
  pb_length = 1;
  prop("dc.b;byte");
  number(b);
  checkblank();
}

static void m68k_dobyte(void)
{
  defb(getbyte());
}

MIPS:

static void mips_dobyte(void)
{
  byte b;

  b = getbyte();
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

PDP11:

static void defb(byte b)
{
  pb_length = 1;
  pdp11_startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

static void pdp11_dobyte(void)
{
  defb(getbyte());
}

PPC:

static void ppc_dobyte(void)
{
  byte b;

  b = getbyte();
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

VAX:

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  if (pb_actual == st_byte) {
    while (getstatus(pc) == st_cont) {
      bufstring(", ");
      number(getbyte());
      pb_length += 1;
    }
  }
  checkblank();
}

static void vax_dobyte(void)
{
  defb(getbyte());
}

Z8:

static void defb(byte b)
{
  pb_length = 1;
  pb_status = st_byte;
  startline(true);
  casestring("defb");
  spacedelim();
  number(b);
  checkblank();
}

static void z8_dobyte(void)
{
  defb(getbyte());
}

Z80:

static void defb(byte b)
{
  pb_length = 1;
  pb_status = st_byte;
  startline(true);
  casestring("defb");
  spacedelim();
  number(b);
  checkblank();
}

static void z80_dobyte(void)
{
  defb(getbyte());
}
