/*
 * Global routines and data from memory.c:
 */

extern void mem_init(void);

extern int mem_examine(int asp, hexaword addr);
extern int mem_deposit(int asp, hexaword addr, uint8_t data);

extern void memc_clear(void);
extern int memc_get(hexaword addr);

extern int mem_read(int size, int asp, hexaword addr, hexaword * res);
extern int mem_write(int size, int asp, hexaword addr, hexaword val);

extern void mem_load_file(char* filename);

/*
 *  Address spaces we know about.
 */

#define ASP_PHYS 0		/* Physical memory. */
#define ASP_VIRT 1		/* Current virtual memory. */
#define ASP_EXEC 2		/* EXEC virtual memory. */
#define ASP_USER 3		/* USER virtual memory. */

/*
 *  Return status for read/write routines:
 */

#define MST_OK      0		/* All OK. */
#define MST_NOMEM   1		/* No memory. */
