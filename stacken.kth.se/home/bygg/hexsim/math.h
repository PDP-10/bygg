/*
 *  Global routines from math.c:
 */

extern void math_multiply(int signflag, int res128);
#define MUL_SIGNED   0
#define MUL_UNSIGNED 1
#define MUL_RES64    0
#define MUL_RES128   1

extern void math_divide(void);

/*
 *  We should have routines for floating point also.
 */
