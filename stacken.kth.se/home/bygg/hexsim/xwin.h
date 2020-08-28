/*
 * Global routines and data from xwin.c:
 */

/*
** Data types:
*/

#define wty_memory    1		/* Memory. */
#define wty_registers 2		/* Registers + PSW/PC etc. */
#define wty_context   3		/* Context (one of them). */
#define wty_terminal  4		/* Terminal line (to the UART). */
#define wty_cpu       5		/* CPU registers. */
#define wty_windows   6		/* Window listing window. */

/*
** routines:
*/

extern void wc_total(void);		/* All changed. */

extern void wc_memory(void);
extern void wc_register(void);
extern void wc_pc(void);
extern void wc_context(int context);
extern void wc_cpu(void);
extern void wc_terminal(void);
extern void wc_windows(void);

extern int  w_open(int type, int subtype);
extern void w_close(int index);
extern void w_lower(int index);
extern void w_raise(int index);
extern void w_move(int index, int xdiff, int ydiff);

extern int  w_next(int index);	/* Get index of next window. */
extern void w_printinfo(int index);
extern void w_setcurrent(int index);

extern void w_putc(char c);
extern void w_beep(void);
extern void w_highlight(int mode);

extern void w_setinput(int index, void (*input)(char));
extern void w_background(void);

extern void w_setaddress(int index, hexaword addr);

extern void wi_reg(char c);
