/* routines in addr.c: */

extern void a_test(void);

extern address* a_copy(address* a, address* b);
extern address* a_new(void);
extern void a_free(address* a);

extern void a_clear(address* a);
extern address* a_zero(void);
extern address* a_next(address* a);
extern address* a_offset(address* a, long offset);
extern void a_inc(address* a, int amount);
extern address* a_fip(address* a);
extern longword a_a2l(address* a);
extern word a_a2w(address* a);
extern address* a_l2a(longword l);
extern int a_mod(address* a, int n);
extern int a_diff(address* a, address* b);
extern int a_compare(address* a, address* b);

extern bool a_eq(address* a, address* b);
extern bool a_ne(address* a, address* b);
extern bool a_gt(address* a, address* b);
extern bool a_ge(address* a, address* b);
extern bool a_lt(address* a, address* b);
extern bool a_le(address* a, address* b);

extern bool a_adjacent(address* a, address* b);

extern void a_step(address* a, int amount);
extern bool a_more(address* a);
extern bool a_ismulti(address* a);
extern bool a_iscount(address* a);
extern bool a_isrange(address* a);
