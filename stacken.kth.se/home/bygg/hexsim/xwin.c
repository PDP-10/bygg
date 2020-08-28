/*
 *  Window handler module.
 */

#include "hexsim.h"


/************************************************************************/

/*
** X related include files:
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

#include <setjmp.h>

/*
 *  This should be taken care of via inclusion of "comnd.h", but there
 *  will be side effects if we do that, and right now that complication
 *  is something I do not need.  Therefore I declare that one routine
 *  by hand for now.
 */

extern void  cm_setinput(int (*reader)(void));

/*
** Local data structures:
*/

/* XXX KLUDGE XXX */

#define bool int
#  define false 0
#  define true 1

/* XXX end kludge */

typedef struct windowblock {
  struct windowblock* next;	/* Links and rechts in the list. */
  struct windowblock* prev;
  int index;		/* Unique index, for ext. refs. */
  int wintype;			/* What type of window we are. */
  char* name;			/* Window name. */

  Window id;			/* id for Xlib. */
  int width;			/* Size in pixels. */
  int height;
  int x, y;			/* Position on screen. */

  bool messed;			/* If our image is messed up. */
  bool mapped;			/* If we are mapped onto the screen. */
  
  bool relocflag;		/* Relocating ourselves? */
  int relocx, relocy;		/* Offsets. */

  void (*eater)(char);		/* Handler-of-input. */

  hexaword address;		/* Our starting address. */
} windowblock;

/*
** local vars:
*/

static Display* display = NULL;
static int screen;
static GC gc;
static XFontStruct* font;
static int fontheight;
static int fontwidth;

static XTextProperty windowname;
static XTextProperty iconname;

static char inputbuffer[20];
static int inputpos;
static int inputcount = 0;

static int tfd;			/* Terminal file descriptor. */
static int xfd;			/* X window file descriptor. */

static int lastindex = 0;	/* Unique index. */

/* foo */

static jmp_buf rubber;		/* Try to implement safe X. */

static bool repaint;

static bool totalmess;

static windowblock* windowlist = NULL;
static windowblock* windowlast = NULL;

static windowblock* defaultwindow = NULL;

/************************************************************************/

static char* wty2name(int wintype)
{
  switch (wintype) {
    case wty_memory:     return("memory");
    case wty_registers:  return("registers");
    case wty_context:    return("context");
    case wty_terminal:   return("terminal");
    case wty_cpu:        return("cpu registers");
    case wty_windows:    return("windows");
  }
  return("unknown");
}

static windowblock* findwinindex(int index)
{
  windowblock* wb;

  wb = windowlist;
  while (wb != NULL) {
    if (wb->index == index) {
      return(wb);
    }
    wb = wb->next;
  }
  return(NULL);
}

static windowblock* findwinblock(Window win)
{
  windowblock* wb;

  wb = windowlist;
  while (wb != NULL) {
    if (wb->id == win) {
      return(wb);
    }
    wb = wb->next;
  }
  return(NULL);
}

static windowblock* argwindow(int index)
{
  if (index == 0) {
    if (defaultwindow != NULL) {
      return(defaultwindow);
    }
    return(windowlist);
  }
  return(findwinindex(index));
}

/*
** copystring() allocates a new string, and copies its argument to it.
*/

static char* copystring(char* src, char* dst)
{
  if (dst != NULL) {
    free(dst);
    dst = NULL;
  }
  if (src != NULL) {
    dst = malloc(strlen(src) + 1);
    strcpy(dst, src);
  }
  return(dst);
}

static windowblock* makewinblock(int wintype)
{
  windowblock* win;
  char work[100];

  win = malloc(sizeof(windowblock));

  memset(win, 0, sizeof(*win));

  win->next = NULL;
  win->prev = windowlast;
  if (windowlast != NULL) {
    windowlast->next = win;
    windowlast = win;
  } else {
    windowlist = win;
    windowlast = win;
  }

  win->wintype = wintype;

  lastindex += 1;
  win->index = lastindex;

  win->mapped = false;
  win->messed = false;

  win->relocflag = false;

  sprintf(work, "%s (%u)", wty2name(win->wintype), win->index);
  win->name = copystring(work, NULL);

  win->eater = NULL;
  
  win->address = 0ull;

  return(win);
}

static void freewinblock(windowblock* win)
{
  if (win == defaultwindow) {	/* Deleting default window? */
    defaultwindow = NULL;	/* Yes, forget default. */
  }

  if (win->prev == NULL) {	/* First in list? */
    windowlist = win->next;	/* Yes. */
  } else {
    win->prev->next = win->next; /* No. */
  }
  if (win->next == NULL) {	/* Last in list? */
    windowlast = win->prev;	/* Yes. */
  } else {
    win->next->prev = win->prev; /* No. */
  }

  free(win->name);

  free(win);
  wc_windows();
}

/**********************************************************************/

#define buffersize 100

static windowblock* workwin;	/* Window we are printing into. */

static bool wwmode = false;	/* If we are writing at the moment. */

static char buffer[buffersize]; /* The actual text buffer. */
static int bufcount;		/* Number of chars in buffer. */
static int wintotal;		/* Total chars on line. */
static int winxpos;		/* Where on line to start printing. */
static int winypos;		/* ... */
static int winmax;		/* Max number of chars on one line. */

static bool peekstop;		/* Stop, the window is full... */

/*
** dumpbuffer() dumps the contents of the text buffer onto the screen.
*/

static void dumpbuffer(void)
{
  if (bufcount > 0) {
    XDrawImageString(display, workwin->id, gc,
		     winxpos, winypos, buffer, bufcount);
    winxpos += fontwidth * bufcount;
    bufcount = 0;
  }
}

/*
** drawnewline() handles newlines, by clearing to end of line, and resetting
** some variables.
*/

static void drawnewline(void)
{
  dumpbuffer();

  winypos += fontheight;
  if (winypos > workwin->height) {
    peekstop = true;
  }
  winxpos = 2;
  wintotal = 0;
}

/*
** drawchar() gives one more char to the buffer.
*/

static void drawchar(char c)
{
  if (c == '\n') {
    while (wintotal < winmax) {
      drawchar(' ');
    }
    drawnewline();
  } else {
    if (bufcount >= buffersize) { /* Still more room? */
      dumpbuffer();		/* No, make some. */
    }
    buffer[bufcount] = c;
    bufcount += 1;
    wintotal += 1;
  }
}

/**********************************************************************/

/*
** wwbegin() sets up text output to the specified window.
*/

static void wwbegin(windowblock* win)
{
  bufxset(1);

  workwin = win;
  wwmode = true;
  bufcount = 0;
  wintotal = 0;
  winmax = win->width / fontwidth;
  winxpos = 2;
  winypos = fontheight + 2;
  peekstop = false;
}

/*
** wwend() resets things after window text output.
*/

static void wwend(void)
{
  while (!peekstop) {
    drawchar('\n');
  }
  wwmode = false;

  bufxset(0);
}

/*
** winmemory() performs updates for memory windows.
*/

static void memstart(hexaword addr)
{
  if (addr == pc_examine())
    bufstring("=>");
  else
    bufstring("  ");

  if (memc_get(addr))
    bufstring("* ");
  else
    bufstring("  ");

  bufhw(addr);
  bufstring(":  ");
}

static void winmemory(windowblock* win)
{
  hexaword addr;
  hexaword data;
  int len;
  char buf[50];

  wwbegin(win);

  addr = win->address;

  for (;;) {
    if (mem_read(8, ASP_PHYS, addr, &data) != 0)
      goto done;

    memstart(addr);
    bufhex(data >> 32, 8);
    len = cpu_disass(data, buf);
    if (len > 0) {
      bufstring("    ;");
      bufstring(buf);
      if (len > 4) {
	addr += 4;
	bufnewline();
	memstart(addr);
	bufhex(data & 0xffffffff, 8);
	bufstring("    ;  --''--");
      }
    }
    bufnewline();
    if (peekstop)
      goto done;
    addr += 4;
  }

 done:
  wwend();
}

/*
** winregisters() performs updates for register windows.
*/

static void winregisters(windowblock* win)
{
  int i;
  uint16_t mask;

  wwbegin(win);

  bufnewline();

  mask = reg_rcmask();

  for (i = 0; i < 16; i += 1) {
    if (mask & (1 << i))
      bufstring("  *");
    else
      bufstring("   ");
	
    if (i <= 9)
      bufchar(' ');
    bufstring("  R");
    bufnumber(i);
    bufstring(": ");
    bufhw(reg_read(i));
    switch (i) {
    case 0:
      bufstring("       PC: ");
      bufhw(pc_examine());
      break;
    case 2:
      bufstring("      PSW:           ");
      bufhex(psw_read(), 8);
      break;      
    case 4:
      bufstring("      ICT: ");
      bufdec(ICT, 18);
      break;
    default:
      break;
    }
    bufnewline();
  }

  wwend();
}

/*
 *  Immediate command handler for register windows (set up when we
 *  create the window).
 */

void wi_reg(char c)
{
  switch (c) {
  case ' ':			/* Space, do one instruction. */
    /* should check to see if the CPU is stopped, should not
     * single-step it if not...
     */
    cpu_execute();
    break;
  default:
    w_beep();
  }
}

/*
 *  Update handler for context windows.
 */

static void wincontext(windowblock* win)
{
  wwbegin(win);

  bufstring("  ... context data ...");

  wwend();
}

/*
 *  update handler for terminal windows:
 */

static void winterminal(windowblock* win)
{
  wwbegin(win);
  term_print();
  wwend();
}

/*
 *  update handler for cpu register windows:
 */

static void wincpuregs(windowblock* win)
{
  wwbegin(win);
  cpu_show_hw_regs();
  wwend();
}

/*
** update handler for window listing windows:
*/

static void winwindows(windowblock* win)
{
  wwbegin(win);

  win = windowlist;
  while (win != NULL) {
    w_printinfo(win->index);
    win = win->next;
  }

  wwend();
}

/*
** update handler for unimplemented window types.
*/

static void winnyi(windowblock* win)
{
  wwbegin(win);

  bufnewline();
  bufstring("This type of window is not yet implemented.  Sorry.");
  bufnewline();

  wwend();
}

/**********************************************************************/

/*
** doevent() handles exactly one X event.  Before we come here we
** have checked that there is one or more events in the queue.
*/

static void doevent(void) {
  XEvent report;
  windowblock* win;

  XNextEvent(display, &report);
  switch (report.type) {
  case Expose:
    if (report.xexpose.count == 0) {
      win = findwinblock(report.xexpose.window);
      if (win != NULL) {
	if (win->mapped) {
	  win->messed = true;
	  repaint = true;
	}
      }
    }
    break;
  case ConfigureNotify:
    win = findwinblock(report.xconfigure.window);
    if (win != NULL) {
      if (win->relocflag) {
	XMoveWindow(display, win->id,
		    win->relocx + 2 * win->x - report.xconfigure.x,
		    win->relocy + 2 * win->y - report.xconfigure.y);
	win->relocflag = false;
      }
      win->width = report.xconfigure.width;
      win->height = report.xconfigure.height;
      win->x = report.xconfigure.x;
      win->y = report.xconfigure.y;
      wc_windows();
    }
    break;
  case MapNotify:
    win = findwinblock(report.xmap.window);
    if (win != NULL) {
      win->mapped = true;
    }
    break;
  case UnmapNotify:
    win = findwinblock(report.xunmap.window);
    if (win != NULL) {
      win->mapped = false;
    }
    break;
  case DestroyNotify:
    win = findwinblock(report.xdestroywindow.window);
    if (win != NULL) {
      freewinblock(win);
    }
    break;
  case KeyPress:
    win = findwinblock(report.xkey.window);
    if (win != NULL) {
      inputcount = XLookupString((XKeyEvent*) &report,
				 inputbuffer, 20, NULL, NULL);
      inputpos = 0;
      if (win->eater != NULL) {
	while (inputpos < inputcount) {
	  (*win->eater)(inputbuffer[inputpos++]);
	}
	inputcount = 0;
      }
    }
    break;
  default:
    break;
  } /* end switch */
}

/*
** updatewindows() updates all windows that need it.
*/

void updatewindows(void)
{
  windowblock* w;

  w = windowlist;
  while (w != NULL) {
    if (w->messed || totalmess) {
      if (w->mapped) {
	switch (w->wintype) {
	case wty_memory:
	  winmemory(w);
	  break;
	case wty_registers:
	  winregisters(w);
	  break;
	case wty_context:
	  wincontext(w);
	  break;
	case wty_terminal:
	  winterminal(w);
	  break;
	case wty_cpu:
	  wincpuregs(w);
	  break;
	case wty_windows:
	  winwindows(w);
	  break;
	default:
	  winnyi(w);
	  break;
	}
      }
      w->messed = false;
    }
    w = w->next;
  }
}

/*
** waitforevent() waits for the next X event or keyboard activity, and
** handles it.
*/

static void waitforevent(void)
{
  int maxfd;
  fd_set readset;
  struct timeval tv;

  if (display == NULL) {
    inputbuffer[0] = getchar();
    inputcount = 1;
    inputpos = 0;
    return;
  }

  if (setjmp(rubber) == 0) {
    if (bg_w_trylock()) {
      while (repaint) {
	repaint = false;
	updatewindows();
	totalmess = false;
      }
      if (XPending(display) > 0) {
	doevent();		/* We have events, do one of them. */
	bg_w_unlock();		/* Release window lock. */
	return;			/* Done for now. */
      }
      bg_w_unlock();
    }

    maxfd = tfd;
    if (xfd > maxfd) {
      maxfd = xfd;
    }

    FD_ZERO(&readset);
    FD_SET(tfd, &readset);
    FD_SET(xfd, &readset);

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (select(maxfd + 1, &readset, NULL, NULL, &tv) > 0) {
      if (FD_ISSET(tfd, &readset)) {
	inputbuffer[0] = getchar();
	inputcount = 1;
	inputpos = 0;
      }
    }
  }
}

/*
 *  Called from the background to update windows.  The caller MUST have
 *  the window update lock here.
 */

void w_background(void)
{
  if (display == NULL) return;

  for (;;) {
    while (repaint) {
      repaint = false;
      updatewindows();
      totalmess = false;
    }
    if (XPending(display) <= 0) return;
    doevent();
  }
}

/*
** readchar() reads characters, and handles events while waiting.
*/

static int readchar(void)
{
  char c;

  while (inputcount == 0) {
    waitforevent();
  }
  c = inputbuffer[inputpos];
  inputcount -= 1;
  inputpos += 1;
  return(c);
}

/*
** openup() is the routine that opens the display.
*/

static int openup(int wintype, int subtype)
{
  windowblock* win;
  int width, height;

  if (display == NULL) {
    display = XOpenDisplay(NULL);
    if (display == NULL) {
      bufstring("%Can't connect to X server.\n");
      return 0;
    }

    screen = DefaultScreen(display);

    font = XLoadQueryFont(display, "fixed");
    if (font == NULL) {
      bufstring("%Can't load font.\n");
      XCloseDisplay(display);
      display = NULL;
      return (0);
    }
    fontheight = font->ascent + font->descent;
    fontwidth = font->max_bounds.width;

    /*
    ** We should check for failure below.
    */

    gc = XCreateGC(display, RootWindow(display, screen), 0, NULL);

    XSetForeground(display, gc, BlackPixel(display, screen));
    XSetBackground(display, gc, WhitePixel(display, screen));
    XSetFont(display, gc, font->fid);

    tfd = fileno(stdin);
    xfd = XConnectionNumber(display);

    cm_setinput(readchar);
  }

  /*
  ** now the display is open.  Fix up a new window.
  */

  switch (wintype) {
    case wty_registers: width = 400;  height = 240;  break;
    case wty_cpu:       width = 400;  height = 240;  break;
    case wty_terminal:  width = 484;  height = 320;  break;
    case wty_windows:   width = 400;  height = 150;  break;
    default:            width = 400;  height = 450;  break;
  }

  win = makewinblock(wintype);
  if (win != NULL) {
    win->id = XCreateSimpleWindow(display, RootWindow(display, screen),
				  0, 0,         /* x, y */
				  width, height, 1,  /* w, h, border */
				  BlackPixel(display, screen),
				  WhitePixel(display, screen));
    /*
    ** Set up our names:
    */

    if (XStringListToTextProperty(&win->name, 1, &windowname) != 0) {
      XSetWMName(display, win->id, &windowname);
    }

    if (XStringListToTextProperty(&win->name, 1, &iconname) != 0) {
      XSetWMIconName(display, win->id, &iconname);
    }

    XSelectInput(display, win->id,
		 ExposureMask | KeyPressMask | ButtonPressMask |
		 StructureNotifyMask);

    XMapWindow(display, win->id);
    return (win->index);
  }
  return (0);
}

/**********************************************************************/

int w_open(int type, int subtype)
{
  return openup(type, subtype);
}

/**********************************************************************/

void w_close(int index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    XDestroyWindow(display, win->id);
  }
}

/**********************************************************************/

/*
** Handlers to be called when something in the universe changes, and
** we might have to redraw some windows.  First a common routine:
*/

static void wctype(int wintype)
{
  windowblock* w;

  for (w = windowlist; w != NULL; w = w->next) {
    if (w->wintype == wintype) {
      w->messed = true;
      repaint = true;
    }
  }
}

/*
** Total change.
*/

void wc_total(void)
{
  totalmess = true;		/* Trivial. */
  repaint = true;
}

/*
** register contents change.
*/

void wc_register(void)
{
  wctype(wty_registers);
}

void wc_pc(void)
{
  wctype(wty_registers);
}

/*
 *  context change.
 */

void wc_context(int context)
{
  wctype(wty_registers);
  wctype(wty_context);
}

/*
 *  terminal got output.
 */

void wc_terminal(void)
{
  wctype(wty_terminal);
}

/*
 *  Memory data change.
 */

void wc_memory(void)
{
  wctype(wty_memory);
}

/*
 *  CPU register change.
 */

void wc_cpu(void)
{
  wctype(wty_cpu);
}

/*
** window add/delete.
*/

void wc_windows(void)
{
  wctype(wty_windows);
}

/**********************************************************************/

/*
** w_next() is used to step over all existing windows.  If the argument
** is zero, we return the first window.  If the argument is non-zero,
** we return the next window after that one.  If there are no more
** windows, we return zero.
*/

int w_next(int index)
{
  windowblock* w;

  if (index == 0) {
    if (windowlist == NULL) {
      return(0);
    }
    return(windowlist->index);
  }
  w = findwinindex(index);
  if (w == NULL) {
    return(0);
  }
  w = w->next;
  if (w == NULL) {
    return(0);
  }
  return(w->index);
}

/**********************************************************************/

void w_printinfo(int index)
{
  windowblock* win;

  win = findwinindex(index);
  if (win != NULL) {
    bufstring("Window # "); bufnumber(index);
    bufstring(" ("); bufstring(wty2name(win->wintype)); bufstring(")");
    bufstring(", h="); bufnumber(win->height);
    bufstring(", w="); bufnumber(win->width);
    bufstring(", x="); bufnumber(win->x);
    bufstring(", y="); bufnumber(win->y);
    bufnewline();
  }
}

/**********************************************************************/

void w_setcurrent(int index)
{
  windowblock* win;

  win = findwinindex(index);
  if (win != NULL) {
    defaultwindow = win;
  }
}

/**********************************************************************/

void w_putc(char c)
{
  /*
  ** here we get a character to output to whatever window we are
  ** working with at the moment.  Handle it.
  */
  drawchar(c);
}

/**********************************************************************/

void w_beep(void)
{
  XBell(display, 0);
}

/**********************************************************************/

void w_highlight(int enterflag)
{
  dumpbuffer();
  if (enterflag != 0) {
    XSetForeground(display, gc, WhitePixel(display, screen));
    XSetBackground(display, gc, BlackPixel(display, screen));
  } else {
    XSetForeground(display, gc, BlackPixel(display, screen));
    XSetBackground(display, gc, WhitePixel(display, screen));
  }
}

/**********************************************************************/

void w_lower(int index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    XLowerWindow(display, win->id);
  }
}

/**********************************************************************/

void w_move(int index, int xdiff, int ydiff)
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    win->relocflag = true;
    win->relocx = xdiff;
    win->relocy = ydiff;
    XMoveWindow(display, win->id, win->x, win->y);
  }
}

/**********************************************************************/

void w_raise(int index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    XRaiseWindow(display, win->id);
  }
}

/**********************************************************************/

void w_setaddress(int index, hexaword address)
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    win->address = address;
    win->messed = true;
    repaint = true;
  }
}

/**********************************************************************/

void w_test(void)
{
  /* nothing at the moment. */
}

/**********************************************************************/

void w_setinput(int index, void (*input)(char))
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    win->eater = input;
  }
}

