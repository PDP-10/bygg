/* routines etc. in memory.c: */

/*
** variables:
*/

/* none for now. */

/*
** routines:
*/

extern void m_init(void);		/* Global init. */

extern byte getmemory(address* addr);
extern stcode getstatus(address* addr);
extern void setstatus(address* addr, stcode status, int count);
extern void suggest(address* addr, stcode status, int count);

extern void setnext(address* addr);
extern byte getnext(void);

/* "segment" handling: */

extern int getscount(void);		/* Get number of segments. */
extern int getsaddr(address* addr);	/* Get segment number of addr. */
extern longword getsrest(address* addr);/* Get # bytes to segment end. */
extern address* getsfirst(int n);       /* Get first addr of segment n. */
extern address* getslast(int n);	/* Get last addr of segment n. */
extern longword getslength(int n);	/* Get length of segment n. */

/* Scan all locations with info: */

extern void foreach(addresshandler*);
extern bool mapped(address*);

/* String handling: */

extern char* copystring(char* src, char* dst);

/* Comment handling routines: */

extern bool c_exist(address* addr);
extern char* c_find(address* addr);
extern void c_insert(address* addr, char* name);
extern void c_delete(address* addr);
extern void c_clear(void);
extern counter c_count(void);

/* Description handling routines: */

extern bool d_exist(address* addr);
extern char* d_find(address* addr);
extern void d_insert(address* addr, char* descr);
extern void d_delete(address* addr);
extern void d_clear(void);
extern counter d_count(void);

/* Expansion handling routines: */

extern bool e_exist(address* addr);
extern char* e_find(address* addr);
extern void e_insert(address* addr, char* descr, int length);
extern void e_delete(address* addr);
extern void e_clear(void);
extern counter e_count(void);

extern int e_length(address* addr);

/* Flag handling routines: */

extern byte* f_read(address* addr);
extern void f_write(address* addr, byte* flags);
extern void f_delete(address* addr);
extern void f_clear(void);

/* Inline argument list handling routines: */

extern pattern* ia_read(address* addr);
extern void ia_write(address* addr, pattern* pat);
extern void ia_delete(address* addr);
extern void ia_clear(void);

/* Noreturn flag handling: */

extern bool nrf_read(address* addr);
extern void nrf_write(address* addr, bool flag);
extern void nrf_clear(void);

/* Label handling routines: */

extern bool l_exist(address* addr);
extern char* l_find(address* addr);
extern void l_insert(address* addr, char* name);
extern void l_delete(address* addr);
extern void l_clear(void);
extern counter l_count(void);

extern void l_cflags(void);
extern void l_def(address* addr);
extern void l_ref(address* addr);

extern address* l_lookup(char* name);
extern void l_rehash(void);

extern longword uniq(void);

/*
** highlight handling:
*/

extern counter  hl_count(void);
extern objindex hl_next(objindex index);
extern void     hl_delete(objindex index);
extern void     hl_clear(void);
extern address* hl_read(objindex index);
extern void     hl_write(address* addr);
extern void     hl_setcolor(int index);

/*
** note handling:
*/

extern counter  n_count(void);
extern objindex n_next(objindex index);
extern void     n_delete(objindex index);
extern void     n_clear(void);
extern char*    n_read(objindex index);
extern void     n_write(char* txt);

/*
** pattern handling:
*/

extern int      p_length(pattern* p);
extern pattern* p_new(void);
extern void     p_free(pattern* p);
extern pattern* p_copy(pattern* src, pattern* dst);
extern patindex p_index(char* name);
extern patindex p_define(char* name);
extern void     p_delete(patindex index);
extern char*    p_name(patindex index);
extern void     p_clear(void);
extern counter  p_count(void);
extern patindex p_next(patindex index);
extern pattern* p_read(patindex index);
extern void     p_write(patindex index, pattern* p);

/*
** register handling:
*/

extern regindex r_index(char* name);
extern regindex r_define(char* name, int type);
extern void     r_delete(regindex index);
extern char*    r_name(regindex index);
extern int      r_type(regindex index);
extern void     r_clear(void);
extern counter  r_count(void);
extern regindex r_next(regindex index);
extern address* r_subrange(regindex index, address* addr);
extern bool     r_isdef(regindex index, address* addr);

extern value*   r_read(regindex index, address* addr);
extern void     r_write(regindex index, address* addr, value* val);

/*
** symbol handling:
*/

extern symindex s_index(char* name);
extern symindex s_define(char* name);
extern void     s_delete(symindex index);
extern char*    s_name(symindex index);
extern void     s_clear(void);
extern counter  s_count(void);
extern symindex s_next(symindex index);
extern char*    s_read(symindex index);
extern void     s_write(symindex index, char* p);

/*
** value handling:
*/

extern value*   v_new(int type);
extern value*   v_copy(value* src, value* dst);
extern void     v_free(value* v);
extern int      v_type(value* v);

extern value*   v_a2v(address* a);
extern address* v_v2a(value* v);

extern value*   v_l2v(longword l);
extern longword v_v2l(value* v);

extern bool     v_eq(value* a, value* b);

/*
** file in/out routines:
*/

extern bool load_binary(char* name, address* addr, int offset);
extern bool load_intel(char* name);
extern bool load_motorola(char* name);

/*
** general routines:
*/

extern void m_purge(void);
extern void m_save(char* name);
extern void m_restore(char* name);
extern void m_test(void);

/* memory manipulation: */

extern void m_clear(void);
extern void m_copy(address* fromaddr, address* toaddr);
extern void m_exclude(address* addr);
extern void m_include(address* addr);
extern void m_move(address* fromaddr, address* toaddr);
extern void m_relocate(address* srcaddr, address* dstaddr);
extern void m_set(address* addr, int value);

/*
extern void m_shuffle(address* fromaddr, ...);
extern void m_xor(address* fromaddr, ...);
*/
