/*
 *  Global routines in bg.c
 */

extern unsigned int bg_ips_get(void);
extern void bg_ips_set(unsigned int newips);
extern void bg_run(void);
extern int  bg_state(void);
extern void bg_stop(void);
extern void bg_init(void);

extern int  bg_w_trylock(void);
extern void bg_w_unlock(void);
