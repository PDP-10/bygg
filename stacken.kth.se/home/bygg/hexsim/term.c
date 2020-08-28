/*
 *   simple ansi terminal:
 */

#include "hexsim.h"

static int inited = 0;

static char buffer[24][80];
static int cursor_row, cursor_col;

static void scroll(void)
{
  int c, r;

  for (c = 0; c < 80; c += 1) {
    for (r = 0; r < 23; r += 1) {
      buffer[r][c] = buffer[r+1][c];
    }
    buffer[23][c] = ' ';
  }
  cursor_row = 23;
}

static void fc_left(int arg)
{
  cursor_col -= arg;
  if (cursor_col < 0) {
    cursor_col = 0;
  }
}

static void fc_right(int arg)
{
  cursor_col += arg;
  if (cursor_col > 79) {
    cursor_col = 79;
  }
}

static void fc_up(int arg)
{
  cursor_row -= arg;
  if (cursor_row < 0) {
    cursor_row = 0;
  }
}

static void fc_down(int arg)
{
  cursor_row += arg;
  if (cursor_row > 23) {
    cursor_row = 23;
  }
}

static void fc_eol(int arg)
{
  int col;

  for (col = cursor_col; col < 80; col += 1) {
    buffer[cursor_row][col] = ' ';
  }
}

static void fc_eos(int arg)
{
  int row, col;

  fc_eol(0);
  for (row = cursor_row + 1; row < 24; row += 1) {
    for (col = 0; col < 80; col += 1) {
      buffer[row][col] = ' ';
    }
  }
}

static void fc_goto(int col, int row)
{
  if (col < 1) col = 1;
  if (col > 80) col = 80;
  if (row < 1) row = 1;
  if (row > 24) row = 24;
  cursor_col = col - 1;
  cursor_row = row - 1;
}

static void fc_lmarg(void)
{
  cursor_col = 0;
}

static void fc_lf(void)
{
  cursor_row += 1;
  if (cursor_row >= 24) {
    scroll();
  }
}

static void fc_tab(void)
{
  cursor_col += 8;
  cursor_col &= ~7;
  if (cursor_col > 79) {
    cursor_col = 79;
  }
}

static void fc_store(char c)
{
  buffer[cursor_row][cursor_col] = c;
  cursor_col += 1;
  if (cursor_col >= 80) {
    cursor_col = 0;
    cursor_row += 1;
    if (cursor_row >= 24) {
      scroll();
    }
  }
}

#define ST_NORM 0
#define ST_ESC  1
#define ST_CSI  2

static void term_input(char c)
{
  static int state = ST_NORM;
  static int arg1, arg2;

  switch (state) {
  case ST_NORM:
    if (c < ' ') {
      switch (c) {
      case 007: w_beep(); break;		/* ^G */
      case 010:	fc_left(1); break;		/* ^H */
      case 011:	fc_tab(); break;		/* TAB */
      case 012:	fc_lf(); break;			/* ^J */
      case 015: fc_lmarg(); break;		/* ^M */
      case 033: state = ST_ESC; break;		/* ESC */
      }
    } else if (c < 0177) {      /* printing char, store. */
      fc_store(c);
    }
    return;
  case ST_ESC:
    if (c == '[') {
      state = ST_CSI;
      arg1 = arg2 = 0;
    } else {
      state = ST_NORM;
    }
    return;
  case ST_CSI:
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      arg1 = arg1 * 10 + c - '0';
      return;
    case ';':
      arg2 = arg1; arg1 = 0;
      return;
    }
    if (arg1 == 0) arg1 = 1;
    if (arg2 == 0) arg2 = 1;
    switch (c) {
    case 'A': fc_up(arg1); break;
    case 'B': fc_down(arg1); break;
    case 'C': fc_right(arg1); break;
    case 'D': fc_left(arg1); break;
    case 'H': fc_goto(arg1, arg2); break;
    case 'J': fc_eos(arg1); break;
    case 'K': fc_eol(arg1); break;
    }
    state = ST_NORM;
  }
}

/*
 *  user callable functions below:
 */

void term_init(void)
{
  if (!inited) {
    fc_goto(1,1);
    fc_eos(1);
  }
  inited = 1;
}

/*
 *  Send a single char to the terminal.
 */

void term_char(char c)
{
  term_input(c);
  wc_terminal();
}

/*
 *  Send the 24*80 screen towards some X window.
 */

void term_print(void)
{
  int row, col;

  for (row = 0; row < 24; row += 1) {
    for (col = 0; col < 80; col += 1) {
      if ((col == cursor_col) && (row == cursor_row)) {
	w_highlight(1);
	w_putc(buffer[row][col]);
	w_highlight(0);
      } else {
	w_putc(buffer[row][col]);
      }
    }
    w_putc('\n');
  }
}
