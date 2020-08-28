/* routines etc. in xwin.c: */

/*
** Data types:
*/

typedef int winindex;		/* sigh... */

#define wty_terminal  1		/* Terminal. */
#define wty_counters  3		/* Counter window. */
#define wty_registers 5		/* Registers. */
#define wty_windows   6		/* Window listing window. */

/*
** routines:
*/

extern void w_test(void);		/* Test function. */

extern void wc_total(void);		/* All changed. */
extern void wc_registers(void);		/* Register change. */
extern void wc_windows(void);		/* Window add/delete. */
extern void wc_terminal(void);		/* Terminal change. */

extern winindex w_open(int wintype);	/* Open window. */
extern void w_close(winindex index);	/* Close a window. */
extern void w_lower(winindex index);    /* Lower a window. */
extern void w_raise(winindex index);    /* Raise a window. */
extern void w_move(winindex index, int xdiff, int ydiff); /* Move win. */

extern winindex w_next(winindex index);	/* Get index of next window. */
extern void w_printinfo(winindex index);/* Print info about window. */
extern void w_setcurrent(winindex index); /* Set current (default) window. */

extern void w_putc(char c);		/* Take output to window. */
extern void w_beep(void);		/* beep beep. */
extern void w_highlight(int mode);	/* Set/clear highlight mode. */

extern void w_setinput(winindex index, void (*input)(char));
extern void w_background(void);
