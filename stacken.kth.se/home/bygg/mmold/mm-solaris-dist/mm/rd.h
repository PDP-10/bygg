/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *rd_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/rd.h,v 2.3 90/10/04 18:25:40 melissa Exp $";
#endif
#endif /* RCSID */

/* rd.h: stuff for rd.c, and so file.c understands rd.c */

/*
 * structure to handle operations on messages
 */
typedef struct msghandler {
    char *typename;			/* what type is this */
    int (*open)();			/* open a file */
    int (*close)();			/* close the file */
    int (*rd_msg)();			/* read a message */
    int (*wr_msg)();			/* write a message */
    int (*probe)();			/* query the file type */
} msg_handler;

/* 
 * some comments about the different routines...  
 */

#define OP_APP		0001		/* open for appending */
#define OP_INT		0002		/* interactive errors */
#define OP_PND		0004		/* ignore poundfile */
/*
 * open:
 *
 * Given a msgvec structure (nf) and a flags word, this routine should
 * open the file named in nf->filename (for read-only if nf->flags
 * contains MF_RDONLY and read/write otherwise).  It is responsible for 
 * locking the file (if opening it for writing or appending) to prevent
 * simultaneous access by other processes running mm.  Something like
 * the UNIX flock(2) would be good.
 *
 * If the flags word contains OP_APP, meaning we're just appending to
 * a file, don't read in any messages.  If this is a non-contiguous
 * format, which requires a current message count to do an append,
 * determine that count.
 *
 * If OP_APP is not on, always determine the number of messages in the
 * mail file (reading them in if counting requires that and space
 * allows).  Also determine the size of the file.
 *
 * If OP_INT is on, meaning this file was parsed interactively, give
 * error messages directly to user (with cmerr?), else pass back error.
 *
 * If OP_PND is on, caller acknowledges that there is already a
 * #filename# (pound) backup file, so ignore it.  If OP_PND is not 
 * on and #filename# exists, give an error and make the file read-only.
 * (the #filename# only exists when running update, so indicates an error
 * otherwise -- cras
 *
 * The pointer to FILE goes in nf->filep, and the count of messages
 * goes in nf->count.  A set of message structures (possibly all
 * flagged empty) should be allocated into nf->msgs if append is
 * false.  The size of the file should go into nf->size.
 */

/*
 * close: 
 *
 * Given a FILE pointer (fp), this should simply close the
 * corresponding file, closing any sessions (as in POP) as necessary.
 * It should release any locks that were set by the open routine.
 */

#define RD_OK		0
#define RD_EOF		1
#define RD_FAIL		2

/*
 * rd_msg:
 *
 * Given a msgvec structure (nf) and a message number (n), this should
 * read the nth message from the file into nf->msgs[n].  (This
 * structure is assumed to exist already.)  It should fill in the
 * size, flags, from (possibly with null), date, and text fields of
 * the message structure.  rd_msg is expected to use nf->last_read to
 * keep track of which message it read last, and optimize away any
 * unnecessary rewinding of the file.  The rd_msg routine returns RD_OK
 * if the message was read successfully, RD_EOF if the file ends just
 * before this message (that is, there are exactly n-1 messages in the
 * file), and RD_FAIL otherwise.
 *
 * Note that the from field of the message structure, if non-NULL,
 * should be malloc'ed, since it will be free'd.
 */

#define WR_COPY		0001		/* copy to new file */
#define WR_SAVE		0002		/* save to old file */
#define WR_EXP		0004		/* expunge deleted messages */
/*
 * wr_msg:
 *
 * Given ptrs to a msgvec structure (mail) and a message structure
 * (msg), a number (n), and a flags word, this makes sure message is
 * written out to the file associated with mail as the nth message.
 *
 * If WR_COPY is in the flags word, we're copying this message into
 * another file.  n is ignored and message is written to the end of
 * the mail file as the mail->count+1th message (incrementing
 * mail->count).
 *
 * If WR_SAVE is on, we're saving the whole file.  For contiguous
 * formats (like mtxt) this means to write message out at the end of
 * the file (requiring no rewinds or fseeks).  For non-contiguous
 * formats (like mh), the message should only be written out if
 * M_MODIFIED is on for that message.  For either format, we should
 * keep mail->size up to date -- however, this may be left until the
 * last message is written to avoid unnecessary overhead.
 *
 * If WR_EXP is on, it is assumed that WR_SAVE is on as well and we
 * are doing an expunge.  This means that for contiguous files,
 * messages with M_DELETED on in their flags word should not be
 * written out.  For non-contiguous files the message-file should
 * actually be deleted at this time (after being copied to a ~ file).
 *
 * If neither WR_SAVE nor WR_COPY is on, we have modified the text of
 * a message.  For a non-contiguous format file, the message should be
 * written out, but for a contiguous format wr_msg should just flag
 * the fact that the mail file must be written out before the file is
 * closed (by setting MF_DIRTY in mail->flags).
 * 
 * After a message is written out, a fflush (or the equivalent for
 * that format) should be done to make sure it gets written.
 *
 * Whenever any message is written out without WR_COPY on (meaning it
 * is written to its original file), M_MODIFIED should be cleared in
 * its flags word, and if it is a non-contiguous format, the message
 * being overwritten should first be saved to a file of the same name
 * with a ~ appended.
 */

#define PR_NAME		0000	/* badly formed name, didn't look at file */
#define PR_NOEX		0001	/* file does not exist */
#define PR_PERM		0002	/* file exists, but couldn't read it */
#define PR_NOTOK	0003	/* file non-empty, but not this format */
#define PR_EMPTY	0004	/* we could read it, but it's empty */
#define PR_OK		0005	/* file is in this format */
/*
 * probe:
 *
 * Given a filename, this should see if the specified file is of this
 * format, any way that works.  Passes back the most exact of the true
 * return codes.  (The maximum of the various return codes is used to
 * determine the true status of the file.)
 *
 * Note that it is VERY IMPORTANT that this routine give the right
 * answer, especially for PR_OK, since mail will be unreadable if the
 * format is incorrect.
 */

/*
 * rename:
 */

#ifndef MSGOPS
extern msg_handler msg_ops[];
#endif

/*
 * keep these in alphabetical order to line up with all the other tables
 * and make sure it matches the stuff in rd.c
 *
 * whenever a type is added here, it must be added to cmds.H as well!!
 */
#define TYPE_BABYL 0
#define TYPE_MBOX  1
#define TYPE_MH    2
#define TYPE_MTXT  3
#define TYPE_POP2  4
#define TYPE_POP3  5
