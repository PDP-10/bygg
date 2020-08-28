/* routines etc. in xwin.c: */

/*
** Data types:
*/

#define wty_listing   1		/* Listing window. */
#define wty_status    2		/* Status map window. */
#define wty_counters  3		/* Counter window. */
#define wty_notes     4		/* Notes window. */
#define wty_symbols   5		/* Symbol summary. */
#define wty_windows   6		/* Window listing window. */
#define wty_dump      7		/* Dump data. */

/*
** routines:
*/

extern void w_test(void);		/* Test function. */

extern void wc_total(void);		/* All changed. */
extern void wc_local(address* a);	/* Local change to address. */
extern void wc_notes(void);		/* Note add/delete */
extern void wc_segment(void);		/* Segment data change. */
extern void wc_symbols(void);		/* Symbol change. */
extern void wc_windows(void);		/* Window add/delete. */

extern void w_open(int wintype);	/* Open window. */
extern void w_close(winindex index);	/* Close a window. */
extern void w_lower(winindex index);    /* Lower a window. */
extern void w_raise(winindex index);    /* Raise a window. */
extern void w_move(winindex index, char direction, int amount);	/* Move win. */

extern winindex w_next(winindex index);	/* Get index of next window. */
extern void w_printinfo(winindex index);/* Print info about window. */

extern address* w_getaddr(winindex index); /* Get addr of window. */

extern void w_setaddr(winindex index, address* a); /* Set addr of window. */
extern void w_setnext(winindex index);	/* Move window forward one screen. */
extern void w_setcurrent(winindex index); /* Set current (default) window. */

extern void w_putc(char c);		/* Take output to window. */
