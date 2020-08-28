/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *chart_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/chartype.h,v 2.1 90/10/04 18:23:38 melissa Exp $";
#endif
#endif /* RCSID */

#define C_UPPER		0001		/* uppercase */
#define C_LOWER		0002		/* lowercase */
#define C_DIGIT		0004		/* 0-9 */
#define C_SPACE		0010		/* space or tab */
#define	C_PUNCT		0020		/* some punctuation */
#define C_PUNC2		0040		/* RFC822 special punctuation */
#define C_EOL		0100		/* return or newline */
#define C_UNUSED	0200		/* unused */
#define C_PRINT		(c_upper|c_lower|c_digit|c_punct|c_punc2)

extern unsigned char chartype[256];	/* length == 2^(bits/char) */

#define isalpha(x) (chartype[(unsigned char) (x)] & (C_UPPER|C_LOWER))
#define isupper(x) (chartype[(unsigned char) (x)] & (C_UPPER))
#define islower(x) (chartype[(unsigned char) (x)] & (C_LOWER))
#define isdigit(x) (chartype[(unsigned char) (x)] & (C_DIGIT))
#define isalnum(x) (chartype[(unsigned char) (x)] & (C_DIGIT|C_UPPER|C_LOWER))
#define isspace(x) (chartype[(unsigned char) (x)] & (C_SPACE|C_EOL))
#define ispunct(x) (chartype[(unsigned char) (x)] & (C_PUNCT|C_PUNC2))
#define isgraph(x) (chartype[(unsigned char) (x)] & \
		    (C_UPPER|C_LOWER|C_DIGIT|C_PUNCT|C_PUNC2))
#define isprint(x) (((unsigned) ((x) - 040)) < 0137)
#define iscntrl(x) (chartype[(unsigned char) (x)]==0)
#define isascii(x) (((unsigned) (x)) <= 0177)
#define isblank(x) (chartype[(unsigned char) (x)] & (C_SPACE))
#define isspecial(x) (chartype[(unsigned char) (x)] & (C_PUNC2))
#define isatom(x)  (chartype[(unsigned char) (x)] & \
		    (C_DIGIT|C_UPPER|C_LOWER|C_PUNCT))
#define iseol(x)   (chartype[(unsigned char) (x)] & (C_EOL))

#define toupper(c) ((c) <= 'z' && (c) >= 'a' ? (c) - 'a' + 'A' : (c))
#define tolower(c) ((c) <= 'Z' && (c) >= 'A' ? (c) - 'A' + 'a' : (c))

#ifdef _CHARTYPE_ARRAY_
unsigned char chartype[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, C_SPACE, C_EOL, 0, C_EOL, C_EOL, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    C_SPACE, C_PUNCT, C_PUNC2, C_PUNCT, C_PUNCT, C_PUNCT, C_PUNCT, C_PUNCT,
    C_PUNC2, C_PUNC2, C_PUNCT, C_PUNCT, C_PUNC2, C_PUNCT, C_PUNC2, C_PUNCT,
    C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT,
    C_DIGIT, C_DIGIT, C_PUNC2, C_PUNC2, C_PUNC2, C_PUNCT, C_PUNC2, C_PUNCT,
    C_PUNC2, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER,
    C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER,
    C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER, C_UPPER,
    C_UPPER, C_UPPER, C_UPPER, C_PUNC2, C_PUNC2, C_PUNC2, C_PUNCT, C_PUNCT,
    C_PUNCT, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER,
    C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER,
    C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER, C_LOWER,
    C_LOWER, C_LOWER, C_LOWER, C_PUNCT, C_PUNCT, C_PUNCT, C_PUNCT, 0
};
#endif /* _CHARTYPE_ARRAY */

