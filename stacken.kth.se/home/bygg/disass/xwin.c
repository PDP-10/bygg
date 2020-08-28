
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

  bool followed;		/* Are we being followed? */
  struct windowblock* target;	/* Window we are folloing. */

  address* first;		/* First address in window. */
  address* last;		/* Last address in window. */
  bool lastmapped;		/* Is win->last mapped in? */
} windowblock;

/*
** Global variables we know about.
*/

static Display* display = NULL;
static int screen;
static GC gc;
static XFontStruct* font;
static int fontheight;
static int fontwidth;

static unsigned long standoutbackg;
static unsigned long standoutcolor;
static char* colorname = NULL;

static XTextProperty windowname;
static XTextProperty iconname;

static char inputbuffer[20];
static int inputpos;
static int inputcount = 0;

static int tfd;			/* Terminal file descriptor. */
static int xfd;			/* X window file descriptor. */

static winindex lastindex = 0;	/* Unique index. */

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
    case wty_counters:   return "counters";
    case wty_dump:       return "data-dump";
    case wty_highlight:  return "highlight";
    case wty_listing:    return "listing";
    case wty_logger:     return "logger";
    case wty_notes:      return "notes";
    case wty_register:   return "register";
    case wty_status:     return "status";
    case wty_symbols:    return "symbols";
    case wty_windows:    return "windows";
    case wty_dots:       return "dots";
    case wty_patterns:   return "patterns";
    case wty_hexdump:    return "hexdump";
    case wty_rpn:        return "rpn";
    case wty_source:     return "source";
  }
  return "unknown";
}

static windowblock* findwinindex(winindex index)
{
  windowblock* wb;

  wb = windowlist;
  while (wb != NULL) {
    if (wb->index == index) {
      return wb;
    }
    wb = wb->next;
  }
  return NULL;
}

static windowblock* findwinblock(Window win)
{
  windowblock* wb;

  wb = windowlist;
  while (wb != NULL) {
    if (wb->id == win) {
      return wb;
    }
    wb = wb->next;
  }
  return NULL;
}

static windowblock* argwindow(winindex index)
{
  if (index == 0) {
    if (defaultwindow != NULL) {
      return defaultwindow;
    }
    return windowlist;
  }
  return findwinindex(index);
}

static windowblock* makewinblock(int wintype)
{
  windowblock* win;
  char work[100];

  win = malloc(sizeof(windowblock));

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

  win->first = NULL;
  win->last = NULL;
  win->lastmapped = false;

  win->followed = false;
  win->target = NULL;

  sprintf(work, "disass %s (%u)", wty2name(win->wintype), win->index);
  win->name = copystring(work, NULL);

  return win;
}

static void freewinblock(windowblock* win)
{
  windowblock* fw;

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

  win->first = a_copy(NULL, win->first);	/* Zap addresses. */
  win->last = a_copy(NULL, win->last);

  win->name = copystring(NULL, win->name); /* Zap name. */

  /* XXX
  **   if we are being followed we MUST recompute the follow structs.
  **   if we are following others, we SHOULD.
  */

  if (win->followed) {
    for (fw = windowlist; fw != NULL; fw = fw->next) {
      if (fw->target == win) {
	fw->target = NULL;
      }
    }
  }

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
  bufxset(false);
}

/*
** winpeek() performs updates for listing windows.
*/

static void winpeek(windowblock* win, bool listing)
{
  static address* pos = NULL;
  int format;

  format = listing? EX_LIST : EX_ASM;

  if (processor == NULL) {
    return;
  }
  if (win->first == NULL) {
    win->first = a_copy(getsfirst(1), win->first);
  }
  if (win->first != NULL) {
    wwbegin(win);

    pos = a_copy(win->first, pos);
    while(mapped(pos)) {
      if (peekstop) {
	break;
      }
      peek(pos, format, st_none);
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
  static address* pos = NULL;
  byte b;
  int i;
  byte data[4];
  int state;

  if (win->first == NULL) {
    win->first = a_copy(getsfirst(1), win->first);
  }
  if (win->first != NULL) {
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
** winhex() updates a hex dump window.
*/

static void winhex(windowblock* win)
{
  static address* pos = NULL;
  byte b;
  int i;
  byte txt[16];

  if (win->first == NULL) {
    win->first = a_copy(getsfirst(1), win->first);
  }
  if (win->first != NULL) {
    wwbegin(win);

    pos = a_copy(win->first, pos);
    i = 0;

    while (mapped(pos)) {
      if (peekstop)
	break;

      if ((i & 0x0f) == 0) {
	bufhex(a_a2l(pos), 8);
	bufstring(": ");
	bufmark();
      }

      b = getmemory(pos);
      txt[i++] = b;
      a_inc(pos, 1);

      bufhex(b, 2);
      bufchar(' ');
      if ((i & 0x0f) == 8)
	bufchar(' ');
      if ((i & 0x0f) == 0) {
	bufchar(' ');
	drawarray(txt, 16);
	bufnewline();
	i = 0;
      }
    }
    if (i > 0) {
      tabto(50);
      drawarray(txt, i);
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

  if (processor != NULL) {
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
  wwbegin(win);
  ls_notes();
  wwend();
}

/*
** update handler for "pattern" windows:
*/

static void winpatterns(windowblock* win)
{
  wwbegin(win);
  ls_patterns();
  wwend();
}

/*
** update handler for register windows:
*/

static void winregister(windowblock* win)
{
  regindex index;

  wwbegin(win);

  index = r_next(0);
  if (index == 0) {
    bufstring("%There are no defined registers.\n");
  }
  while (index != 0) {
    ls_register(index);
    index = r_next(index);
  }

  wwend();
}

/*
** update handler for rpn calculator windows:
*/

static void winrpn(windowblock* win)
{
  wwbegin(win);

  bufstring("x: "); bufnumber(rpn_rstack(0)); bufnewline();
  bufstring("y: "); bufnumber(rpn_rstack(1)); bufnewline();
  bufstring("z: "); bufnumber(rpn_rstack(2)); bufnewline();
  bufstring("t: "); bufnumber(rpn_rstack(3)); bufnewline();

  wwend();
}

/*
** update handler for "status" windows:
*/

static void winstatus(windowblock* win)
{
  static address* pos = NULL;
  int count, i, hpos;

  if (win->first == NULL) {
    win->first = a_copy(getsfirst(1), win->first);
  }
  if (win->first != NULL) {
    wwbegin(win);

    pos = a_copy(win->first, pos);

    count = getsrest(pos) / pv_bpa;
    if (count == 0) {
      count = 100000;
    }

    win->lastmapped = false;
    while (count > 0) {
      if (peekstop) {
	win->last = a_copy(pos, win->last);
	win->lastmapped = true;
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
	bufchar(st2char(getstatus(pos)));
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
  wwbegin(win);
  ls_symbols();
  wwend();
}

/*
** update handler for window listing windows:
*/

static void winwindows(windowblock* win)
{
  wwbegin(win);

  for (win = windowlist; win != NULL; win = win->next) {
    w_printinfo(win->index);
  }

  wwend();
}

/*
** update handler for dots windows:
*/

static void windots(windowblock* win)
{
  wwbegin(win);

  ls_dots();

  wwend();
}

/*
** update handler for logger windows:
*/

#define logmax 10

static int logcnt = 0;
static int logindex = 0;
static char* logstr[logmax];
static int lognum[logmax];

static void winlogger(windowblock* win)
{
  int i;

  wwbegin(win);

  for (i = 0; i < logmax; i += 1) {
    if (logstr[i] != NULL) {
      bufnumber(lognum[i]);
      bufstring(":");
      tabto(8);
      bufstring(logstr[i]);
      bufnewline();
    }
  }
  wwend();
}

/*
** update handler for highlight list windows:
*/

static void winhighlight(windowblock* win)
{
  wwbegin(win);
  ls_highlights();
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
** return NULL.
*/

static address* xy2addr(int x, int y)
{
  return NULL;
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
      if (report.xconfigure.send_event) { /* Strange... */
	win->x = report.xconfigure.x;
	win->y = report.xconfigure.y;
      }
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
    inputcount = XLookupString((XKeyEvent*) &report,
			       inputbuffer, 20, NULL, NULL);
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
  while (w != NULL) {
    if (w->messed || totalmess) {
      if (w->mapped) {
	switch (w->wintype) {
	case wty_counters:
	  wincount(w);
	  break;
	case wty_dump:
	  windata(w);
	  break;
	case wty_hexdump:
	  winhex(w);
	  break;
	case wty_highlight:
	  winhighlight(w);
	  break;
	case wty_listing:
	  winpeek(w, true);
	  break;
	case wty_logger:
	  winlogger(w);
	  break;
	case wty_notes:
	  winnotes(w);
	  break;
	case wty_register:
	  winregister(w);
	  break;
	case wty_rpn:
	  winrpn(w);
	  break;
	case wty_source:
	  winpeek(w, false);
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
	case wty_dots:
	  windots(w);
	  break;
	case wty_patterns:
	  winpatterns(w);
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
    while (repaint) {
      repaint = false;
      updatewindows();
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

      if (select(maxfd + 1, &readset, NULL, NULL, &tv) > 0) {
	if (FD_ISSET(tfd, &readset)) {
	  inputbuffer[0] = getchar();
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
  return c;
}

/*
** setpri() fixes the fd number for the standard input.
*/

static void setpri(void)
{
  /*  tfd = fileno(cmcsb._cmij); */
}

/*
** getcolor() allocates a standout color.
*/

static void getcolor(char* color, bool fg)
{
  XColor use;
  XColor exact;

  if (color == NULL) {
    color = "red";
  }

  if (fg) {
    colorname = copystring(color, colorname);
  }

  if (display != NULL) {
    XAllocNamedColor(display, XDefaultColormap(display, screen),
		     color,  &use, &exact);
    if (fg) {
      standoutcolor = use.pixel;
    } else {
      standoutbackg = use.pixel;
    }
  }
}

/*
** openup() is the routine that opens the display.
*/

static void openup(int wintype)
{
  windowblock* win;
  int width, height;

  if (display == NULL) {
    display = XOpenDisplay(NULL);
    if (display == NULL) {
      bufstring("%Can't connect to X server.\n");
      return;
    }

    screen = DefaultScreen(display);

    font = XLoadQueryFont(display, "fixed");
    if (font == NULL) {
      bufstring("%Can't load font.\n");
      XCloseDisplay(display);
      display = NULL;
      return;
    }
    fontheight = font->ascent + font->descent;
    fontwidth = font->max_bounds.width;

    getcolor(colorname, true);
    standoutbackg = WhitePixel(display, screen);

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
    case wty_dots:      width = 400;  height = 150;  break;
    case wty_logger:    width = 400;  height = 200;  break;
    case wty_highlight: width = 400;  height = 200;  break;
    case wty_patterns:  width = 400;  height = 200;  break;
    case wty_hexdump:   width = 475;  height = 450;  break;
    case wty_rpn:       width = 200;  height =  60;  break;
    case wty_listing:   width = 450;  height = 550;  break;
    case wty_source:    width = 450;  height = 550;  break;
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

    /*
    ** ... seems like the nice, well thought out XLib requires the
    ** user to deallocate the "value" subfield of TextPropertys if
    ** he wants to reuse them or forget them.
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
** Change local to address.
*/

void wc_local(address* a)
{
  wctype(wty_listing);
  wctype(wty_counters);		/* FOO */
}

/*
** Code might have changed apperance.
*/

void wc_code(void)
{
  wctype(wty_listing);
}

/*
** Note add/delete.
*/

void wc_notes(void)
{
  wctype(wty_notes);
}

/*
** pattern change.
*/

void wc_patterns(void)
{
  wctype(wty_patterns);
}

/*
** register change.
*/

void wc_register(regindex index)
{
  wctype(wty_listing);
  wctype(wty_register);
}

/*
** calculator change.
*/

void wc_rpn(void)
{
  wctype(wty_rpn);
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
  wctype(wty_symbols);
}

/*
** window add/delete.
*/

void wc_windows(void)
{
  wctype(wty_windows);
}

/*
** dot change.
*/

void wc_dots(void)
{
  wctype(wty_dots);
}

/*
** log buffer change.
*/

void wc_log(void)
{
  wctype(wty_logger);
}

/*
** highlight list change.
*/

void wc_highlight(void)
{
  wctype(wty_listing);
  wctype(wty_highlight);
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
    if (windowlist == NULL) {
      return 0;
    }
    return windowlist->index;
  }
  w = findwinindex(index);
  if (w == NULL) {
    return 0;
  }
  w = w->next;
  if (w == NULL) {
    return 0;
  }
  return w->index;
}

/**********************************************************************/

void w_printinfo(winindex index)
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

address* w_getaddr(winindex index)
{
  static address* work = NULL;
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    work = a_copy(win->first, work);
    return work;
  }
  return NULL;
}

/**********************************************************************/

void w_setaddr(winindex index, address* a)
{
  windowblock* win;
  windowblock* fw;

  win = argwindow(index);
  if (win != NULL) {
    win->first = a_copy(a, win->first);
    win->messed = true;
    repaint = true;
    if (win->followed) {
      for (fw = windowlist; fw != NULL; fw = fw->next) {
	if (fw->target == win) {
	  fw->first = a_copy(a, fw->first);
	  fw->messed = true;
	}
      }
    }
  }
}

/**********************************************************************/

void w_setcolor(char* color)
{
  getcolor(color, true);
  wc_highlight();
}

/**********************************************************************/

void w_setinverse(void)
{
  static int mode = 1;

  if (mode++ & 1)
    getcolor("grey", false);
  else
    getcolor("white", false);

  wc_highlight();
}

/**********************************************************************/

void w_setcurrent(winindex index)
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

void w_highlight(int handle, address* addr, bool enterflag)
{
  UNUSED(handle);
  UNUSED(addr);

  if (wwmode) {			/* Only do this when we are at it. */
    dumpbuffer();
    if (enterflag) {
      XSetForeground(display, gc, standoutcolor);
      XSetBackground(display, gc, standoutbackg);
      w_logger("set highlight");
    } else {
      XSetForeground(display, gc, BlackPixel(display, screen));
      XSetBackground(display, gc, WhitePixel(display, screen));
      w_logger("clr highlight");
    }
  }
}

/**********************************************************************/

void w_setnext(winindex index)
{
  windowblock* win;
  windowblock* fw;

  win = argwindow(index);
  if (win != NULL) {
    if (win->lastmapped && (win->last != NULL)) {
      win->first = a_copy(win->last, win->first);
      win->messed = true;
      repaint = true;
      if (win->followed) {
	for (fw = windowlist; fw != NULL; fw = fw->next) {
	  if (fw->target == win) {
	    fw->first = a_copy(win->first, fw->first);
	    fw->messed = true;
	  }
	}
      }
    }
  }
}

/**********************************************************************/

void w_extend(winindex index, winindex target)
{
  /* this should be like w_follow(), but should extend (mainly)
  ** listing windows so that the second one starts where the first
  ** one ended.
  **
  ** Need to think about implementation.
  */
}

/**********************************************************************/

void w_follow(winindex index, winindex target)
{
  windowblock* win;
  windowblock* targ;

  win = argwindow(index);
  targ = findwinindex(target);

  if (win == NULL || targ == NULL) return;

  if (win == targ) {
    win->target = NULL;
    /* XXX should recompute the followed flags. */
  } else {
    targ->followed = true;
    win->target = targ;
    win->first = a_copy(targ->first, win->first);
    win->messed = true;
    repaint = true;
  }
}

/**********************************************************************/

void w_lower(winindex index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    XLowerWindow(display, win->id);
  }
}

/**********************************************************************/

void w_move(winindex index, int xdiff, int ydiff)
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

void w_raise(winindex index)
{
  windowblock* win;

  win = argwindow(index);
  if (win != NULL) {
    XRaiseWindow(display, win->id);
  }
}

/**********************************************************************/

void w_test(void)
{
  /* nothing at the moment. */

#if 0
  bufstring("Default window = ");
  if (defaultwindow != NULL) {
    bufnumber(defaultwindow->index);
  } else {
    bufstring("<none>");
  }
  bufnewline();
#endif
}

/**********************************************************************/

void w_logger(char* s)
{
  logcnt += 1;
  logindex += 1;
  if (logindex >= logmax) {
    logindex = 0;
  }
  logstr[logindex] = copystring(s, logstr[logindex]);
  lognum[logindex] = logcnt;

  wc_log();
}

/**********************************************************************/

/* end of file */
