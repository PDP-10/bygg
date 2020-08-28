/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /src/sos5.bin/cucca/mm/mm/RCS/browse.c,v 2.1 1990/10/04 18:23:34 melissa Exp $";
#endif

#include "mm.h"
#include "parse.h"
#include "cmds.h"

int browse_body(), browse_header(), browse_next(), browse_prev(),
    browse_quit(), browse_switch(), browse_help(), browse_copy(), 
    browse_reply(), browse_delete(), browse_flag(), browse_keyword();

extern int browse_pause, browse_clear_screen;

#define B_FORWARD 1
#define B_BACKWARD 2
#define B_STAY 3
#define B_QUIT 4
#define CS (cf->sequence)
#define PS (cf->prev_sequence)
#define RS (cf->read_sequence)

#define CSfirst (sequence_first(CS))
#define CSlast (sequence_last(CS))
#define CSinverted (sequence_inverted(CS))
#define PSfirst (sequence_first(PS))
#define PSlast (sequence_last(PS))
#define PSinverted (sequence_inverted(PS))
#define RSfirst (sequence_first(RS))
#define RSlast (sequence_last(RS))
#define RSinverted (sequence_inverted(RS))

int browse_direction = B_FORWARD;

struct browse_cmds {
    char cmd;
    int (*fctn)();
    char *hlpstr;
} browse_cmds[] = {
    { ' ', browse_body, 	"see the next message" },
    { 'n', browse_next,		"go to next message" },
    { 'p', browse_prev,		"go to previous message" },
    { 'q', browse_quit,		"quit browsing" },
    { 's', browse_switch,	"switch direction of browsing" },
    { '?', browse_help,		"print this out" },
    { '\f', browse_header,	"redisplay the header" },
    { '\n', browse_body,	"see the message" },
    { 'c', browse_copy,		"copy to a file" },
    { 'r', browse_reply,	"reply to the message" },
    { 'd', browse_delete,	"delete this message" },
    { 'f', browse_flag,		"flag this message" },
    { 'k', browse_keyword,	"add a keyword to this message" },
    { '\0', nil, nil },
};

cmd_browse(n) 
int n;
{
    int omode;

    if (!check_cf (O_RDWR))
	return;

    omode = mode;
    if (!(mode & MM_READ)) {
	parse_sequence ("unseen", NULL, NULL);
	copy_sequence (cf->read_sequence, cf->sequence); /* XXX */
	check_mark();			/* can we mark things? */
	
	if (sequence_start (cf->read_sequence)) {
	    mode |= MM_BROWSE;
	}
    }
    else {
	confirm();
	mode |= MM_BROWSE;
    }
}

browse_message()
{
    int c;

    browse_header();
    c = browse_getchar();
    return(browse_dispatch(c));
}

browse_header() {
    message *m = &cf->msgs[cf->current];
    char *htext(), *from = htext("from", m->text);
    char *date = hdate(m->date);
    char *subj = htext("subject",m->text);

    if (browse_clear_screen == SET_YES)
	cmcls();			/* clear the screen */
    else
	printf("\n");

    header_summary (cf->current, stdout, 1); /* 1 => browse format */

    printf("--%d chars;  More?--", m->size);
    safe_free(from);
    safe_free(subj);
    return(B_STAY);
}

browse_body() 
{
    if (clear_screen)			/* user normally want this? */
	cmcls();
    else
	printf("\n");

    if (!display_message(stdout, &cf->msgs[cf->current], false,
			 only_type_headers, dont_type_headers) ||
	browse_pause == SET_YES)
    {
	if (browse_clear_screen == SET_YES) {
	    printf("---Hit any key to continue---");
	    browse_getchar();
	}
    }
    change_flag(cf->current, CMD_MARK);
    return(B_FORWARD);
}

browse_next() 
{
    return(B_FORWARD);
}

browse_prev()
{
    return(B_BACKWARD);
}

browse_switch()
{
    sequence_inverted(CS) = !sequence_inverted(CS);
    return(B_FORWARD);
}

browse_quit()
{
    cmxputc('\n');
    return(B_QUIT);
}

browse_help()
{
    int i;
    printf("\n");
    for(i = 0; browse_cmds[i].fctn != nil; i++) {
	if (browse_cmds[i].cmd < ' ' || browse_cmds[i].cmd == '\177')
	    printf("'^%c'", browse_cmds[i].cmd ^ 0100);
	else
	    printf(" '%c'", browse_cmds[i].cmd);
	printf(" -- %s\n", browse_cmds[i].hlpstr);
    }

    if (browse_clear_screen == SET_YES) {
	printf("---Hit any key to continue---");
	browse_getchar();
    }
    return(B_STAY);
}

browse_dispatch(c) {
    int ret = -1;
    int r,i;
    for(i = 0; browse_cmds[i].fctn != nil; i++)
	if (browse_cmds[i].cmd == c) {
	    ret = (*browse_cmds[i].fctn)();
	    break;
	}
    switch(ret) {
    case B_STAY:
	break;
    case B_FORWARD:
	if (sequence_next(CS) == 0)
	    mode &= ~MM_BROWSE;
	break;
    case B_BACKWARD:
	sequence_inverted(CS) = !sequence_inverted(CS);
	r = sequence_next(CS);
	sequence_inverted(CS) = !sequence_inverted(CS);
	if (r == 0)
	    mode &= ~MM_BROWSE;
	break;
    case B_QUIT:
	mode &= ~MM_BROWSE;
	break;
    default:
	fflush(cmcsb._cmoj);
	fprintf(stderr,"\n\007Invalid command\n");
	printf("--%d chars;  More?--", cf->msgs[cf->current].size);	
	return(browse_dispatch(browse_getchar()));
    }
}

browse_copy() 
{
    int doprev = false;

    cmseteof();
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF) {
	    return CMxEOF;
	}
	else
	    doprev = true;
    }
    
    prompt("Copy to: ");
    if (doprev) {
	doprev = false;
	prevcmd();
    }
    cmsetrp();
    cmd_copy(CMD_COPY, false);
    return(B_FORWARD);
}

browse_reply() 
{
    cmxnl();
    do_reply(cf->current, reply_all, reply_include_me);
    return(B_FORWARD);
}

int
browse_getchar()
{
    int c = getchar();
    cmechx(c);
    return(c);
}

browse_delete()
{
    check_mark();
    change_flag(cf->current, CMD_DELETE);
    return(B_FORWARD);
}

browse_flag()
{
    check_mark();
    change_flag(cf->current, CMD_FLAG);
    return(B_STAY);
}

browse_keyword() {
    int doprev = false;

    check_mark();
    cmseteof();
    if (cmseter ()) {			/* errors return here */
	if (cmcsb._cmerr == CMxEOF) {
	    return CMxEOF;
	}
	else
	    doprev = true;
    }
    
    prompt("Keyword: ");
    if (doprev) {
	doprev = false;
	prevcmd();
    }
    cmsetrp();
    do_keyword();
    return(B_STAY);
}
