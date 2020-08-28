/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *seq_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/seq.h,v 2.1 90/10/04 18:26:37 melissa Exp $";
#endif
#endif /* RCSID */
/*
 * seq.h - sequence keyword table indices
 */

/*
 * sequence bitmap access macros (usually defined in sys/param.h)
 */

#ifndef NBBY
#define NBBY 8
#endif

#ifndef setbit
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)
#endif

/*
 * message sequence structure.
 */

typedef struct {
    unsigned first;			/* first message in sequence */
					/* 0 => sequence is invalid */
    unsigned last;			/* last message in sequence */
    unsigned invert;			/* 1 => invert */
    unsigned char *bits;		/* zero-based bit vector */
    struct msgvec *file;		/* backpointer to message "file" */
} *sequence_t;

#define sequence_first(s)	((s)->first)
#define sequence_last(s)	((s)->last)
#define sequence_bits(s)	((s)->bits)
#define sequence_file(s)	((s)->file)
#define in_sequence(s,i)	(isset (sequence_bits(s), i))
#define valid_sequence(s)	(sequence_first(s) != 0)
#define trivial_sequence(s)	(sequence_first(s) == sequence_last(s))
#define sequence_inverted(s)	((s)->invert)
#define clear_sequence(s) \
    bzero(sequence_bits(s), (s)->file->count / NBBY + 1), \
    sequence_first(s) = sequence_last(s) = sequence_inverted(s) = 0
#define free_sequence(s) \
    if (s) free(sequence_bits(s)), free(s), (s) = 0; else abort()

/*
 * Message sequence operators
 */

#define SEQ_AFTER	0
#define SEQ_ALL		1
#define SEQ_ANSWERED	2
#define SEQ_BEFORE	3
#define SEQ_CURRENT	4
#define SEQ_DELETED	5
#define SEQ_FLAGGED	6
#define SEQ_FROM	7
#define SEQ_INVERSE	8
#define SEQ_KEYWORD	9
#define SEQ_LAST	10
#define SEQ_LONGER	11
#define SEQ_NEW		12
#define SEQ_ON		13
#define SEQ_PREVIOUS	14
#define SEQ_RECENT	15
#define SEQ_SEEN	16
#define SEQ_SHORTER	17
#define SEQ_SINCE	18
#define SEQ_SUBJECT	19
#define SEQ_TEXT	20
#define SEQ_TO		21
#define SEQ_UNANSWERED	22
#define SEQ_UNDELETED	23
#define SEQ_UNFLAGGED	24
#define SEQ_UNKEYWORD	25
#define SEQ_UNSEEN	26

#define SEQ_RANGE	-1

/*
 * structure for representing message sequences
 */

typedef struct seq_node {
    int type;				/* e.g. SEQ_AFTER */
    union {
	struct {
	    int n;			/* number or range of numbers */
	    int m;
	} range;
	struct {
	    time_t first;		/* date or range of dates */
	    time_t last;
	} dates;
	keyword s;			/* string to search for */
	u_long flags;			/* flags to compare against */
	struct seq_node *right;
    } data;
    struct seq_node *left;
} seq_node;

#define SEQ_STACK_SIZE 32		/* max number of sequence elements */ 
