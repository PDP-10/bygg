/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *mm_rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/mm.h,v 2.0.1.3 1997/10/21 19:33:32 howie Exp $";
#endif
#endif /* RCSID */

#include "stdio.h"
#include "config.h"			/* site configuration file */
#include "osfiles.h"			/* standard system header files */
#include "compat.h"			/* compatibility macros */
#include "chartype.h"			/* private version of <ctype.h> */
#include "pathnames.h"			/* filename defaults */
#include "address.h"			/* address list definitions */

#define true	1
#define false	0
#define nil	NULL			/* XXX */

#if MDEBUG
#define malloc dmalloc
#define calloc dcalloc
#define realloc drealloc
#define free dfree
#define cmrealloc safe_realloc
#endif

/*
 * Global state flags
 */
#define MM_TOP_LEVEL	0x00		/* in the top-level command parser */
#define MM_READ		0x01		/* in "read mode" */
#define MM_SEND		0x02		/* in "send mode" */
#define MM_REPLY	0x04		/* replying to a sequence */
#define MM_ANSWER	0x08		/* answering a message */
#define MM_BROWSE	0x10		/* browsing a file */

#define KEYLEN	64			/* header keyword size */
#define STRLEN	256			/* string length */
#define BUFLEN	512			/* size of parse buffers, etc */

typedef char keyword[KEYLEN];		/* XXX this is a bad idea... */
typedef char buffer[BUFLEN];
typedef char string[STRLEN];
typedef char **keylist;

#include "seq.h"

/*
 * variables accessible to the user are defined using the "variable" type.
 */

typedef struct {
    char *name;				/* variable name */
    int type;				/* variable type */
    char *addr;				/* address of the variable */
    int size;				/* max length for string variables */
    int changed;			/* modified by user? */
} variable;

/*
 * i don't think this structure isn't used right now 
 */

typedef struct address {
    char *name;				/* name for a group list, or ";" */
    char *phrase;			/* introductory comment */
    char *route;			/* route */
    char *user;				/* local-part */
    char *host;				/* domain */
    char *comment;			/* comment from parens */
    struct address *next;		/* next addr in list */
    struct address *sublist;		/* sublist if group list */
} address;

#define M_SEEN		0001		/* seen */
#define M_DELETED	0002		/* deleted */
#define M_FLAGGED	0004		/* flagged */
#define M_ANSWERED	0010		/* answered */
#define M_FILED		0020		/* filed */
#define M_RECENT	0040		/* recent */
#define M_MODIFIED	0100		/* message flags modified */
#define M_EDITED	0200		/* edited the message text */
#define M_FORWARDED	0400		/* forwarded the message */
#define M_INTERNAL	~(M_MODIFIED|M_RECENT)	/* flags stored in file */

/*
 * The message struct describes a single message in a mail file
 */
typedef struct {
    long offset;			/* position in file */
    unsigned long size;			/* message size */
    unsigned long flags;		/* message flags */
    unsigned long keybits;		/* funky keyword bits */
    char *from;				/* sender */
    time_t date;			/* date received */
    char *text;				/* address in memory */
    char *hdrsum;			/* cached line for "headers" cmd */
    int next;				/* next message in sequence */
    int prev;				/* previous message in sequence */
    char **keywords;			/* list of keywords */
} message;

#define MF_MAILBOX	0x01		/* user's primary mailbox  */
#define MF_SPOOL	0x02		/* user's spool file */
#define MF_RDONLY	0x04		/* read-only mode */
#define MF_DIRTY	0x08		/* mail file is dirty */
#define MF_MODIFIED	0x10		/* some flags have been changed */
#define MF_WRITERR	0x20		/* write error */
#define MF_SAVEONLY	0x40		/* save even if read-only */

/*
 * msgvec describes a mail file
 */

typedef struct msgvec {
    string filename;			/* name of the file these came from */
    time_t when_read;			/* when we opened the file */
    time_t mtime;			/* file's mtime when we opened it */
    FILE *filep;			/* pointer to the file */
    int type;				/* what type of mail file is it? */
    unsigned long flags;		/* message flags */
    int current;			/* current message number */
    int last_read;			/* last message read from the file */
    int count;				/* total number of messages in file */
    unsigned long size;			/* length of file (in bytes) */

    sequence_t sequence;		/* current message sequence */
    sequence_t prev_sequence;		/* previous message sequence */
    sequence_t read_sequence;		/* read sequence */

    /* note that we waste the zeroth entry of this array -- it's never used */
    message *msgs;			/* pointer to the message array */
    keylist keywords;			/* keywords for this file */
} msgvec;

/* exit codes for get_msg (send.c and sendcmds.c) */
#define GET_EDIT  1
#define GET_ESC   2
#define GET_CTRLD 3
#define GET_ABORT 4

/* flags for the update() routine */
#define UPD_ALWAYS	0x01
#define UPD_SAVEMOD	0x02
#define UPD_EXPUNGE	0x04
#define UPD_ALTFILE	0x08
#define UPD_QUIET	0x10

/* a few things for yesno and yesnoask variables */
#define SET_ASK -1
#define SET_NO 0
#define SET_NEVER 0
#define SET_YES 1
#define SET_ALWAYS 1

#include "extern.h"			/* external declarations */
