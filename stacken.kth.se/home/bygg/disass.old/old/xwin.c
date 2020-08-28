
/*  Make code to call XSetErrorHandler & XSetIOErrorHandler */

/*
** This module implements X window junk.
*/

#include "comnd.h"
#include "disass.h"

/*
** X related include files:
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

/*
** Local data structures:
*/

typedef struct windowblock {
  struct windowblock* next;	/* Links and rechts in the list. */
  struct windowblock* prev;
  winindex index;		/* Unique index, for ext. refs. */
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

  /* handlers etc. */

  address* first;		/* First address in window. */
  address* last;		/* Last address in window. */
  bool lastmapped;		/* Is win->last mapped in? */
} windowblock;

/*
** Global variables we know about.
*/

static Display* display = nil;
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

static winindex lastindex = 0;	/* Unique index. */

/*
** first test of live code into the window:
*/

static bool peekstop;

#define winlinesize 200
static char winline[winlinesize];

static int wlmax;
static int wlpos;
static int winypos;
static windowblock* workwin;

/* foo */

static jmp_buf rubber;		/* Try to implement safe X. */

static bool repaint;

static bool totalmess;

static windowblock* windowlist = nil;
static windowblock* windowlast = nil;

static windowblock* defaultwindow = nil;

/************************************************************************/

static char* wty2name(int wintype)
{
  switch (wintype) {
    case wty_counters: return("counters");
    case wty_listing:  return("listing");
    case wty_notes:    return("notes");
    case wty_status:   return("status");
    case wty_symbols:  return("symbols");
    case wty_windows:  return("windows");
    case wty_dump:     return("data-dump");
  }
  return("unknown");
}

static windowblock* findwinindex(winindex index)
{
  windowblock* wb;

  wb = windowlist;
  while (wb != nil) {
    if (wb->index == index) {
      return(wb);
    }
    wb = wb->next;
  }
  return(nil);
}

static windowblock* findwinblock(Window win)
{
  windowblock* wb;

  wb = windowlist;
  while (wb != nil) {
    if (wb->id == win) {
      return(wb);
    }
    wb = wb->next;
  }
  return(nil);
}

static windowblock* argwindow(winindex index)
{
  if (index == 0) {
    if (defaultwindow != nil) {
      return(defaultwindow);
    }
    return(windowlist);
  }
  return(findwinindex(index));
}

static windowblock* makewinblock(int wintype)
{
  windowblock* win;
  char work[100];

  win = malloc(sizeof(windowblock));

  win->next = nil;
  win->prev = windowlast;
  if (windowlast != nil) {
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

  win->first = nil;
  win->last = nil;
  win->lastmapped = false;

  sprintf(work, "disass %s (%u)", wty2name(win->wintype), win->index);
  win->name = copystring(work, nil);

  return(win);
}

static void freewinblock(windowblock* win)
{
  if (win == defaultwindow) {	/* Deleting default window? */
    defaultwindow = nil;	/* Yes, forget default. */
  }

  if (win->prev == nil) {	/* First in list? */
    windowlist = win->next;	/* Yes. */
  } else {
    win->prev->next = win->next; /* No. */
  }
  if (win->next == nil) {	/* Last in list? */
    windowlast = win->prev;	/* Yes. */
  } else {
    win->next->prev = win->prev; /* No. */
  }

  win->first = a_copy(nil, win->first);	/* Zap addresses. */
  win->last = a_copy(nil, win->last);

  win->name = copystring(nil, win->name); /* Zap name. */

  free(win);
  wc_windows();
}

/*
** drawline() outputs the contents of the line buffer to the window.
*/

static void drawline(void)
{
  while (wlpos < wlmax) {
    winline[wlpos] = ' ';
    wlpos += 1;
  }
  XDrawImageString(display, workwin->id, gc, 2, winypos, winline, wlpos);
  winypos += fontheight;
  if (winypos > workwin->height) {
    peekstop = true;
  }
}

static void drawchar(char c)
{
  if (c == '\n') {
    drawline();
    wlpos = 0;
  } else {
    if (wlpos < wlmax) {
      winline[wlpos] = c;
      wlpos += 1;
    }
  }
}

static void drawarray(byte* data, int len)
{
  int i;
  byte b;

  drawchar('"');
  for(i = 0; i < len; i += 1) {
    b = data[i];
    if ((b < 32) || (b >= 127)) {
      b = 1;
    }
    drawchar(b);
  }
  drawchar('"');
}

/*
** wwbegin() sets up text output to the specified window.
*/

static void wwbegin(windowblock* win)
{
  bufxset(true);
  workwin = win;
  wlpos = 0;
  wlmax = win->width / fontwidth;
  if (wlmax > winlinesize) {
    wlmax = winlinesize;
  }
  winypos = fontheight + 2;
  peekstop = false;
}

/*
** wwend() resets things after window text output.
*/

static void wwend(void)
{
  while (!peekstop) {
    drawline();
    wlpos = 0;
  }
  if (wlpos > 0) {		/* Draw any leftover chars. */
    drawline();			/* ** can this be removed? ** */
  }				/* ** can this be removed? ** */
  bufxset(false);
}

/*
** winpeek() performs updates for listing windows.
*/

static void winpeek(windowblock* win)
{
  static address* pos = nil;

  if (processor == nil) {
    return;
  }
  if (win->first == nil) {
    win->first = a_copy(getsfirst(1), win->first);
  }
  if (win->first != nil) {
    wwbegin(win);

    pos = a_copy(win->first, pos);
    while(mapped(pos)) {
      if (peekstop) {
	break;
      }
      peek(pos, EX_LIST, st_none);
      a_inc(pos, pb_length * pv_bpa);
    }
    if (peekstop) {
      win->last = a_copy(pos, win->last);
      win->lastmapped = true;
    } else {
      win->lastmapped = false;
    }
    wwend();
  }
}

/*
** windata() performs updates for data-dump windows.
*/

static void windata(windowblock* win)
{
  static address* pos = nil;
  byte b;
  int i;
  byte data[4];
  int state;

  if (win->first == nil) {
    win->first = a_copy(getsfirst(1), win->first);
  }
  if (win->first != nil) {
    wwbegin(win);

    state = 0;

    pos = a_copy(win->first, pos);
    while(mapped(pos)) {
      if (peekstop) {
	break;
      }
      if (state == 0) {
	switch (pv_abits) {
	  case 16: bufhex(a_a2w(pos), 4); break;
	  case 32: bufhex(a_a2l(pos), 8); break;
	  default: bufhex(a_a2l(pos), 8); break;
	}
	bufstring(": ");
	bufmark();
      }

      b = getmemory(pos);
      data[state] = b;
      a_inc(pos, 1);
      
      for(i = 0x80; i != 0; i >>= 1) {
	if (b & i) {
	  bufchar('1');
	} else {
	  bufchar('0');
	}
      }

      state += 1;
      if (state < 4) {
	bufstring(" ");
      } else {
	state = 0;
	bufstring(" ");
	drawarray(data, 4);
	bufnewline();
      }
    }
    if (state > 0) {
      tabto(36);
      drawarray(data, state);
    }
    if (peekstop) {
      win->last = a_copy(pos, win->last);
      win->lastmapped = true;
    } else {
      win->lastmapped = false;
    }
    wwend();
  }
}

/*
** wincount() performs updates for counter windows.
*/

static void wincount(windowblock* win)
{
  int i, count;
  longword size;
  longword total;

  wwbegin(win);

  if (processor != nil) {
    bufstring("Processor (cpu): ");
    bufstring(processor->name);
  }
  bufnewline();
  bufstring("comments:     "); bufnumber(c_count()); bufnewline();
  bufstring("descriptions: "); bufnumber(d_count()); bufnewline();
  bufstring("expansions:   "); bufnumber(e_count()); bufnewline();
  bufstring("labels:       "); bufnumber(l_count()); bufnewline();
  bufnewline();

  count = getscount();
  if (count == 0) {
    bufstring("No memory mapped.");
  } else {
    total = 0L;
    for(i = 1; i <= count; i += 1) {
      size = getslength(i);
      bufstring("Segment ");
      bufnumber(i);
      bufstring(", at ");
      bufstring(a_a2str(getsfirst(i)));
      bufstring("-");
      bufstring(a_a2str(getslast(i)));
      bufstring(" len ");
      if (pv_bpa > 1) {
	size = (size + pv_bpa - 1) / pv_bpa;
      }
      bufsize(size);
      total += size;
    }
    if (count > 1) {
      bufstring("Total size: ");
      bufsize(total);
    }
  }

  wwend();
}

/*
** update handler for "notes" windows:
*/

static void winnotes(windowblock* win)
{
  objindex index;
  char* note;

  wwbegin(win);

  index = n_next(0);
  while (index != 0) {
    note = n_read(index);
    if (note != nil) {
      bufnumber(index);
      tabto(4);
      bufstring(note);
      bufnewline();
    }
    index = n_next(index);
  }

  wwend();
}

/*
** update handler for "status" windows:
*/

static void winstatus(windowblock* win)
{
  static address* pos = nil;
  stcode status;
  int count, i, hpos;

  if (win->first == nil) {
    win->first = a_copy(getsfirst(1), win->first);
  }
  if (win->first != nil) {
    wwbegin(win);

    pos = a_copy(win->first, pos);

    count = getsrest(pos) / pv_bpa;
    if (count == 0) {
      count = 100000;
    }

    while (count > 0) {
      if (peekstop) {
	break;
      }
      bufaddress(pos);
      bufchar(':');
      tabto(7);
      i = 64;
      if (i > count) {
	i = count;
      }
      count -= i;
      hpos = 0;
      while (i-- > 0) {
	if ((hpos & 0x0f) == 0) {
	  bufchar(' ');
	}
	hpos += 1;
	status = getstatus(pos);
	switch (status) {
	  case st_none:   bufchar('-'); break;
	  case st_cont:   bufchar('.'); break;
	  case st_byte:   bufchar('B'); break;
	  case st_char:   bufchar('C'); break;
	  case st_double: bufchar('D'); break;
	  case st_float:  bufchar('F'); break;
	  case st_inst:   bufchar('I'); break;
	  case st_long:   bufchar('L'); break;
	  case st_mask:   bufchar('M'); break;
	  case st_octa:   bufchar('O'); break;
	  case st_ptr:    bufchar('P'); break;
	  case st_quad:   bufchar('Q'); break;
	  case st_text:   bufchar('T'); break;
	  case st_word:   bufchar('W'); break;
	  default:        bufchar('?');
	}
	a_inc(pos, pv_bpa);
      }
      bufnewline();
    }
    wwend();
  }
}

/*
** update handler for "symbol" windows:
*/

static void winsymbols(windowblock* win)
{
  symindex index;
  
  wwbegin(win);

  index = s_next(0);
  if (index == 0) {
    bufstring("%There are no defined symbols.\n");
  }
  while (index != 0) {
    bufsymbol(index);
    index = s_next(index);
  }

  wwend();
}

/*
** update handler for window listing windows:
*/

static void winwindows(windowblock* win)
{
  wwbegin(win);

  win = windowlist;
  while (win != nil) {
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
** xy2addr() returns the address corresponding to the given position
** in the (listing) window.  If we can't find out that address, we
** return nil.
*/

static address* xy2addr(int x, int y)
{
  return(nil);
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
      if (win != nil) {
	if (win->mapped) {
	  win->messed = true;
	  repaint = true;
	}
      }
    }
    break;
  case ConfigureNotify:
    win = findwinblock(report.xconfigure.window);
    if (win != nil) {
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
    if (win != nil) {
      win->mapped = true;
    }
    break;
  case UnmapNotify:
    win = findwinblock(report.xunmap.window);
    if (win != nil) {
      win->mapped = false;
    }
    break;
  case DestroyNotify:
    win = findwinblock(report.xdestroywindow.window);
    if (win != nil) {
      freewinblock(win);
    }
    break;
  case KeyPress:
    inputcount = XLookupString((XKeyEvent*) &report,
			       inputbuffer, 20, nil, nil);
    inputpos = 0;
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
  while (w != nil) {
    if (w->messed || totalmess) {
      if (w->mapped) {
	switch (w->wintype) {
	case wty_listing:
	  winpeek(w);
	  break;
	case wty_counters:
	  wincount(w);
	  break;
	case wty_notes:
	  winnotes(w);
	  break;
	case wty_status:
	  winstatus(w);
	  break;
	case wty_symbols:
	  winsymbols(w);
	  break;
	case wty_windows:
	  winwindows(w);
	  break;
	case wty_dump:
	  windata(w);
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

  if (display == nil) {
    inputbuffer[0] = fgetc(cmcsb._cmij);
    inputcount = 1;
    inputpos = 0;
    return;
  }

  if (setjmp(rubber) == 0) {
    if (repaint) {
      updatewindows();
      repaint = false;
      totalmess = false;
    }

    if (XPending(display) > 0) {
      doevent();			/* We have events, do one of them. */
    } else {
      maxfd = tfd;
      if (xfd > maxfd) {
	maxfd = xfd;
      }

      FD_ZERO(&readset);
      FD_SET(tfd, &readset);
      FD_SET(xfd, &readset);

      tv.tv_sec = 1;
      tv.tv_usec = 0;

      if (select(maxfd + 1, &readset, nil, nil, &tv) > 0) {
	if (FD_ISSET(tfd, &readset)) {
	  inputbuffer[0] = fgetc(cmcsb._cmij);
	  inputcount = 1;
	  inputpos = 0;
	}
      }
    }
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
** setpri() fixes the fd number for the standard input.
*/

static void setpri(void)
{
  tfd = fileno(cmcsb._cmij);
}

/*
** openup() is the routine that opens the display.
*/

static void openup(int wintype)
{
  windowblock* win;
  int width, height;

  if (display == nil) {
    display = XOpenDisplay(nil);
    if (display == nil) {
      bufstring("%Can't connect to X server.\n");
      return;
    }

    font = XLoadQueryFont(display, "fixed");
    if (font == nil) {
      bufstring("%Can't load font.\n");
      XCloseDisplay(display);
      display = nil;
      return;
    }

    fontheight = font->ascent + font->descent;
    fontwidth = font->max_bounds.width;

    screen = DefaultScreen(display);

    /*
    ** We should check for failure below.
    */

    gc = XCreateGC(display, RootWindow(display, screen), 0, nil);

    XSetForeground(display, gc, BlackPixel(display, screen));
    XSetBackground(display, gc, WhitePixel(display, screen));

    XSetFont(display, gc, font->fid);

    tfd = fileno(cmcsb._cmij);
    xfd = XConnectionNumber(display);

    cmcsb._cmXrd = &readchar;	/* Set up reader. */
    cmcsb._cmXset = &setpri;	/* Set up catcher. */
    /*
      (void) XSetErrorHandler(eh);
      (void) XSetIOErrorHandler(ioeh);
    */
  }

  /*
  ** now the display is open.  Fix up a new window.
  */

  switch (wintype) {
    case wty_counters:  width = 400;  height = 150;  break;
    case wty_notes:     width = 400;  height = 200;  break;
    case wty_status:    width = 460;  height = 220;  break;
    case wty_symbols:   width = 400;  height = 150;  break;
    case wty_windows:   width = 400;  height = 150;  break;
    default:            width = 400;  height = 450;  break;
  }

  win = makewinblock(wintype);
  if (win != nil) {
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
  }
}

/**********************************************************************/

void w_open(int wintype)
{
  openup(wintype);
}

/**********************************************************************/

void w_close(winindex index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != nil) {
    XDestroyWindow(display, win->id);
  }
}

/**********************************************************************/

/*
** Total change.
*/

void wc_total(void)
{
  totalmess = true;		/* Trivial. */
  repaint = true;
}

/*
** Change local to address.
*/

void wc_local(address* a)
{
  totalmess = true;		/* Simple version. */
  repaint = true;
}

/*
** Note add/delete.
*/

void wc_notes(void)
{
  windowblock* w;

  for (w = windowlist; w != nil; w = w->next) {
    if (w->wintype == wty_notes) {
      w->messed = true;
      repaint = true;
    }
  }
}

/*
** Segment data change.
*/

void wc_segment(void)
{
  totalmess = true;		/* Simple version. */
  repaint = true;
}

/*
** symbol change.
*/

void wc_symbols(void)
{
  windowblock* w;

  for (w = windowlist; w != nil; w = w->next) {
    if (w->wintype == wty_symbols) {
      w->messed = true;
      repaint = true;
    }
  }
}

/*
** window add/delete.
*/

void wc_windows(void)
{
  windowblock* w;

  for (w = windowlist; w != nil; w = w->next) {
    if (w->wintype == wty_windows) {
      w->messed = true;
      repaint = true;
    }
  }
}
  
/**********************************************************************/

/*
** w_next() is used to step over all existing windows.  If the argument
** is zero, we return the first window.  If the argument is non-zero,
** we return the next window after that one.  If there are no more
** windows, we return zero.
*/

winindex w_next(winindex index)
{
  windowblock* w;

  if (index == 0) {
    if (windowlist == nil) {
      return(0);
    }
    return(windowlist->index);
  }
  w = findwinindex(index);
  if (w == nil) {
    return(0);
  }
  w = w->next;
  if (w == nil) {
    return(0);
  }
  return(w->index);
}

/**********************************************************************/

void w_printinfo(winindex index)
{
  windowblock* win;

  win = findwinindex(index);
  if (win != nil) {
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

address* w_getaddr(winindex index)
{
  static address* work = nil;
  windowblock* win;

  win = argwindow(index);
  if (win != nil) {
    work = a_copy(win->first, work);
    return(work);
  }
  return(nil);
}

/**********************************************************************/

void w_setaddr(winindex index, address* a)
{
  windowblock* win;

  win = argwindow(index);
  if (win != nil) {
    win->first = a_copy(a, win->first);
    win->messed = true;
    repaint = true;
  }
}

/**********************************************************************/

void w_setcurrent(winindex index)
{
  windowblock* win;

  win = findwinindex(index);
  if (win != nil) {
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

void w_setnext(winindex index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != nil) {
    if (win->lastmapped && (win->last != nil)) {
      win->first = a_copy(win->last, win->first);
      win->messed = true;
      repaint = true;
    }
  }
}

/**********************************************************************/

void w_lower(winindex index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != nil) {
    XLowerWindow(display, win->id);
  }
}

/**********************************************************************/

void w_move(winindex index, char direction, int amount)
{
  windowblock* win;

  win = argwindow(index);
  if (win != nil) {
    win->relocflag = true;
    switch (direction) {
      case 'd': win->relocx = 0; win->relocy =  amount; break;
      case 'u': win->relocx = 0; win->relocy = -amount; break;
      case 'r': win->relocx =  amount; win->relocy = 0; break;
      case 'l': win->relocx = -amount; win->relocy = 0; break;
    }
    XMoveWindow(display, win->id, win->x, win->y);
  }
}

/**********************************************************************/

void w_raise(winindex index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != nil) {
    XRaiseWindow(display, win->id);
  }
}

/**********************************************************************/

void w_test(void)
{
  bufstring("Default window = ");
  if (defaultwindow != nil) {
    bufnumber(defaultwindow->index);
  } else {
    bufstring("nil");
  }
  bufnewline();

  /* nothing at the moment. */
}

/**********************************************************************/

/* end of file */
