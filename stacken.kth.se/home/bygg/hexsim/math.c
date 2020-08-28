/*
 *  This module implements math functions.
 */

#include "hexsim.h"

/*
 *  Perform a 64*64 bit multipy, arguments are in AR and BR, return
 *  value (128 bits) goes into AR (hi-order) and ARX (low-order).
 *
 *  If the signed flag is non-zero, do sign manipulation.
 *
 *  This routine cannot fail.  If the caller wants a 64-bit result,
 *  he has to handle this (and eventual overflow) himself.
 */

void math_multiply(int signflag, int res128)
{
  hexaword au, al, bu, bl;

  /*
   *  Step 1: if either argument is zero, return zero.
   */

  if (AR == 0 || BR == 0) {
    AR = ZERO;
    ARX = ZERO;
    return;
  }

  /*
   *  Step 2: if signed multiply, find out the final sign and strip
   *  out the sign bits, keeping the magnitudes.  After this, the
   *  sign flag is either 0 (unsigned) or +/-1 (final sign).
   */

  if (signflag == MUL_UNSIGNED) {
    signflag = 0;
  } else {
    signflag = 1;
    if (AR & SIGNBIT) {		/* Make positive, and remember sign. */
      signflag = -signflag;
      AR ^= FWMASK;
      AR += 1;
    }
    if (BR & SIGNBIT) {		/* Make positive, and remember sign. */
      signflag = -signflag;
      BR ^= FWMASK;
      BR += 1;
    }
  }

  /*
   *  Step 3: perform a 64*64 multiplication.
   */

  au = AR >> 32; al = AR & RHMASK;
  bu = BR >> 32; bl = BR & RHMASK;

  ARX = al * bl;
  AR = au * bu;

  au *= bl;
  bu *= al;

  AR += (au >> 32);
  AR += (bu >> 32);

  au <<= 32;
  bu <<= 32;

  if (au > FWMASK - ARX)
    AR += 1;
  ARX += au;
  if (bu > FWMASK - ARX)
    AR += 1;
  ARX += bu;

  /*
   *  Step 4: reinstate the sign, and check for overflow.
   */

  if (res128 == MUL_RES128) {	/* Double length result?  cant overflow. */
    if (signflag == -1) {
      AR ^= FWMASK;
      ARX ^= FWMASK;
      ARX += 1;
      if (ARX == ZERO)
	AR += 1;
    }
  } else {
    if ((AR != ZERO) ||
	(signflag == 1 && ARX & SIGNBIT) ||
	(signflag == -1 && ARX > SIGNBIT)) {
      psw_setbit(PSW_OVF);      /* Overflow. */
    }
    AR = ARX;			/* Return result in AR. */
    if (signflag == 1) {
      AR &= ~SIGNBIT;
    }
    if (signflag == -1) {
      AR ^= FWMASK;
      AR += 1;
    }
  }
}

void math_divide(void)
{

}
