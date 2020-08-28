/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *msg_rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/message.h,v 2.2 90/10/04 18:24:45 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * Header types
 */

#define TO		1
#define CC		2
#define BCC		3
#define FROM		4
#define DATE		5
#define SUBJECT		6
#define REPLY_TO	7
#define IN_REPLY_TO	8
#define RESENT_TO	9
#define RESENT_DATE	10
#define RESENT_FROM	11
#define SENDER		12
#define REFERENCES	13
#define COMMENTS	14
#define MESSAGE_ID	15
#define KEYWORDS	16
#define ENCRYPTED	17
#define RECEIVED	18
#define USER_HEADERS	19
#define FCC		20
/*
 * header flags
 */
#define HEAD_KNOWN 0x0001
#define HEAD_UNKNOWN 0x0002

typedef struct headers {		/* define a message header */
    int type;				/* type of header */
    int flags;				/* info about the header. */
    char *name;				/* name of the header */
    struct headers *next;		/* next header */
/* ******* the following should be made a union at some point, but the names
   ******* are too general to make into #defines right now.
*/
    addresslist *address;		/* address if an address field */
    keylist keys;			/* place to hold a bunch of strings */
    char *string;			/* or just a string otherwise */
} headers;

typedef struct mail_msg {
    headers *headers;			/* list of unordered headers */
    headers *last;
    char *body;				/* text of the message */
    headers *to;			/* list of known headers for */
    headers *cc;			/* reference and easy lookup */
    headers *bcc;
    headers *fcc;
    headers *from;
    headers *date;
    headers *subject;
    headers *reply_to;
    headers *in_reply_to;
    headers *resent_to;
    headers *resent_date;
    headers *resent_from;
    headers *sender;
    headers *references;
    headers *comments;
    headers *message_id;
    headers *keywords;
    headers *encrypted;
    headers *received;
} mail_msg;

