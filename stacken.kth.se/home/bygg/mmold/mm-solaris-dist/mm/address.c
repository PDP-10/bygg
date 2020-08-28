/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/address.c,v 2.2 91/06/07 14:37:50 fuat Exp $";
#endif

#include "mm.h"
#include "ccmd.h"

char *malloc(), *safe_strcat(), *realloc(), *safe_free(), *safe_strcpy();
static int parselen;
addresslist *lookup_alias();

/* 
 * break masks for various fdbs
 */

/*
 * break mask for comment text.   Cannot have "(" or ")" in it.
 */
static brktab commentbrk = {
  {					/* print chars except (, ), ? */
    0xff, 0xff, 0xff, 0xff, 0x80, 0xc0, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
  },
  {					/* all print characters */
    0xff, 0xbf, 0xff, 0xff, 0x00, 0xc0, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
  }
};

/*
 * break mask for remote user.  A slightly modified field break mask.
 * (also used in seq.c)
 */
brktab rembrk = {			/* break table for remote user name */
    {					/* letters only in first position */
      0xff, 0xff, 0xff, 0xff, 0xf3, 0xfb, 0x00, 0x3b,
      0x80, 0x00, 0x00, 0x16, 0x80, 0x00, 0x00, 0x1f
    },
    {					/* letters, digits and hyphens here */
      0xff, 0xff, 0xff, 0xff, 0xb2, 0xe8, 0x00, 0x3b,
      0x80, 0x00, 0x00, 0x16, 0x80, 0x00, 0x00, 0x1f
    }					/* (also +%_.!/) */
};

/*
 * break mask for a username.
 */
static brktab aliasbrk = {		/* all valid chars for users */
  {					/* alphanums, "~#/_-\[]," */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xd9, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0b,
  },
  {
    0xff, 0xff, 0xff, 0xff, 0xbb, 0xd9, 0x00, 0x1f,
    0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0b,
  }
};

/*
 * break mask for a username.
 */
static brktab usrbrk = {		/* all valid chars for users */
  {					/* alphanums, "~#/_-\[]," */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xd1, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0b,
  },
  {
    0xff, 0xff, 0xff, 0xff, 0xfb, 0xd1, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0b,
  }
};

/*
 * break mask for a hostname.
 */
static brktab hostbrk = {
{					/* letters only in first position */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3f,
    0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x1f
  },
  {					/* alphanums, "-." */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0x00, 0x3f,
    0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x1f
  }
};

/*
 * break mask for at sign token parse
 */
static brktab atbrk = {
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    },
    {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    }
};

/*
 * fdb's for all of the address parses.
 */

/*
 * start of comment
 */
static fdb comment = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) "(",
    nil, nil, nil, nil };

/* 
 * text of a comment
 */
static fdb commenttxt = { _CMFLD, FLD_EMPTY|CM_SDH, nil, nil, "a comment",
    nil, &commentbrk, nil};

/*
 * end of a comment
 */
static fdb commentend = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) ")",
    "\")\" to end the comment", nil, nil, nil };

/*
 * start of a mailbox
 */
static fdb mbox = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) "<", "<mailbox>", 
    nil, nil, "invalid mail recipient" };

/*
 * end of a mailbox
 */
static fdb mboxend = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) ">" , ">", nil,
    nil, "expected a \">\"" };

/*
 * start of an internet address
 */
static fdb ipaddr = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) "[", nil, nil, nil,
    "invalid hostname or host address" };

/*
 * a number in an IP address
 */
static fdb ipaddrnum = { _CMNUM, CM_SDH, nil, (pdat) 10, "host IP address",
    nil, nil, "expected an IP address octet" };

/*
 * a "." in an IP address
 */
static fdb ipaddrdot = { _CMTOK, TOK_WAK|CM_SDH, nil, ".", ".", nil, nil,
    "expected a \".\"" };

/*
 * end of an ip address
 */
static fdb ipaddrend = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) "]", nil, nil, 
    nil, nil };

/* 
 * a "::" to make a string into an ugly mail11 hostname (for melissa)
 */
static fdb mail11 = { _CMTOK, CM_SDH, nil, (pdat) "::", nil, nil, nil, nil };

/* 
 * a ":" to make a string into a group name
 */
static fdb group = { _CMTOK, CM_SDH, nil, (pdat) ":", 
    "\":\" to make this a group name", nil, nil, nil };

/*
 * ";" to end of a group list
 */
static fdb groupend = { _CMTOK, CM_SDH, nil, (pdat) ";", nil, nil, nil, nil };

/*
 * a comma for between addresses
 */
static fdb comma = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) ",",
    "\",\" for another address", nil, nil,
    "comma required between addresses" };

/*
 * a comma for route info
 */
static fdb comma2 = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) ",",
    "\",\" for an additional route", nil, nil, nil };

/*
 * a colon
 */
static fdb colon = { _CMTOK, TOK_WAK|CM_SDH, nil, (pdat) ":",
    "\":\" to end route information", nil, nil, "expected a \":\"" };


/*
 * remote user name.   really a field.
 */
static fdb remusr = { _CMFLD, FLD_EMPTY|CM_SDH, nil, nil, "network address",
    nil, &rembrk, "invalid mail recipient" };

/*
 * a field.  used for parts of phrases.
 */
static fdb field = { _CMFLD, FLD_EMPTY|CM_SDH, nil, nil, nil, nil,
    &rembrk, nil };

/*
 * a quoted string
 */
static fdb qstr = { _CMQST, CM_SDH, nil, nil, "quoted string", nil, 
    nil, nil };

/*
 * a username
 */
static fdb username = { _CMUSR, CM_SDH, nil, nil, "user name" , nil,
    &usrbrk, nil };

/*
 * "." to signify sending to self
 */
static fdb dot = { _CMTOK, CM_SDH, nil, (pdat) ".", 
    "\".\" to send to yourself", nil, nil, nil };

/*
 * an asterix to send to a file
 */
static fdb asterix = { _CMTOK, CM_SDH|TOK_WAK, nil, (pdat) "*",
    "\"*\" to send to a file", nil, nil, nil };

/*
 * an @ sign for an indirect address through a file.
 */
static fdb indirect = { _CMTOK, CM_SDH|TOK_WAK, nil, (pdat) "@", 
    "\"@\" to obtain addresses from a file", nil, nil, nil };

/*
 * a "@@" for mailing lists (put in aliases to postpone reading the file until
 * the message is being sent.  Only works in the define command.
 */

static fdb mlist = { _CMTOK, CM_SDH|TOK_WAK, nil, (pdat) "@@", 
    "\"@@\" to obtain addresses from a file when alias is invoked",
    nil, nil, nil };

/*
 * confirmation
 */
static fdb confirm = { _CMCFM, 0, nil, nil, nil, nil, nil, nil };

/*
 * a host name
 */
static fdb hostname = { _CMFLD, CM_SDH, nil, nil, "host name", nil,
    &hostbrk, nil};

/*
 * at sign for user@host
 */
static fdb at = { _CMTOK, CM_SDH|TOK_WAK, nil, (pdat) "@", 
    "\"@\" for network host name", nil, &atbrk, nil };

/*
 * an output file for "*filename"
 */
static fdb outfile = { _CMFIL, FIL_NODIR|FIL_PO|CM_SDH, nil, nil, 
    "filename to output message to", nil, nil, "invalid output filename" };

/*
 * an input filename for "@filename"
 */
static fdb indfile = { _CMFIL, CM_SDH|FIL_NODIR, nil, nil, 
    "filename of indirect file",nil, nil, "invalid indirect filename" };

/* 
 * an input file @@filename
 */
static fdb listfile = { _CMFIL, CM_SDH|FIL_PO|FIL_NODIR, nil, nil, 
    "filename of deferred indirect file", nil, nil,
    "invalid indirect filename" };


/* 
 * keyword table for user defined aliases
 */
static fdb aliasfdb = {
    _CMKEY, CM_SDH|KEY_EMO, nil, nil, "mail alias", nil,  &aliasbrk, nil
};



static int active_label = FALSE;

#ifdef TEST
char atmbuf[100];
static int cmdbuf[10000];		/* ccmd work buffers */
static char wrkbuf[100];

main() 
{
    char *cmini();
    static addresslist a= { nil, nil };

    cmcsb._cmntb = "#";			/* comment to eol starts with '#' */
    cm_set_ind(FALSE);			/* no indirections allowed */
    cmseti(stdin, stdout, stderr);
    cmbufs (cmdbuf, sizeof cmdbuf, atmbuf, sizeof atmbuf,
	    wrkbuf, sizeof wrkbuf);	/* set up buffers for ccmd */
    while(1) {
	cmseter();
	prompt("addr>");
	cmsetrp();
	parse_addresses(&a);
	disp_addresses(stdout,"address",&a,TRUE,TRUE,FALSE,TRUE);
    }
}
#else 
#define atmbuf cmcsb._cmabp
#endif /* TEST */

/* 
 * parse a list of addresses.
 * returns an addresslist in a.
 * since it will be called inside of reparses, it free's
 * the data pointed to by a.
 */

parse_addresses(a) 
addresslist *a;
{
    static char *bp=nil;
    cm_set_ind(FALSE);
    free_addresslist(a);
    active_label = FALSE;
    while(TRUE) {
	if (bp) {
	    free(bp);
	    bp = nil;
	}
	if (!parse_address_1(a,&bp,FALSE,FALSE))
	    break;
    }
    if (active_label) {			/* if still an active_label */
	bp = safe_strcat(bp,";",FALSE);
	add_addresslist(a,bp,ADR_GROUPEND);
	bp = safe_free(bp);
    }
    active_label = FALSE;
}

parse_address(a) 
addresslist *a;
{
    static char *bp=nil;
    cm_set_ind(FALSE);
    free_addresslist(a);
    active_label = FALSE;
    while(TRUE) {
	if (bp) {
	    free(bp);
	    bp = nil;
	}
	if (!parse_address_1(a,&bp,TRUE,FALSE))
	    break;
    }
    if (active_label) {			/* if still an active_label */
	bp = safe_strcat(bp,";",FALSE);
	add_addresslist(a,bp,ADR_GROUPEND);
	bp = safe_free(bp);
    }
    active_label = FALSE;
}

parse_define(a) 
addresslist *a;
{
    static char *bp=nil;
    cm_set_ind(FALSE);
    free_addresslist(a);
    active_label = FALSE;
    while(TRUE) {
	if (bp) {
	    free(bp);
	    bp = nil;
	}
	if (!parse_address_1(a,&bp,FALSE,TRUE))
	    break;
    }
    if (active_label) {			/* if still an active_label */
	bp = safe_strcat(bp,";",FALSE);
	add_addresslist(a,bp,ADR_GROUPEND);
	bp = safe_free(bp);
    }
    active_label = FALSE;
}
/*
 * parse a single address, terminated by a comma, or a confirm.
 * also allow group names, or endings.
 * returns 1 if parsing should continue.  0 if it should end.
 * returns the parsed address in a.
 */
parse_address_1(a,this,one,alias) 
addresslist *a;				/* list of addresses */
char **this;				/* address currently being parsed */
int one;				/* only accept one address? */
{
    pval pv;
    fdb *used;

    aliasfdb._cmdat = (pdat) mk_alias_keys();
    if (!alias)
	parse(fdbchn(&confirm,		/* confirm to send list */
		     &asterix,		/* *filename */
		     &indirect,		/* @filename */
		     &aliasfdb,		/* mail alias */
#ifdef undef
		     &username,		/* username */
#endif
		     &dot,		/* "." */
		     &qstr,		/* quotedstring */
		     &comment,		/* (comment) */
		     &mbox,		/* <mbox> */
		     &groupend,		/* ";" */
		     &remusr,		/* remote user */
		     nil), &pv, &used);
    else
	parse(fdbchn(&confirm,		/* confirm to send list */
		     &asterix,		/* *filename */
		     &mlist,		/* @@file */
		     &indirect,		/* @filename */
		     &aliasfdb,		/* mail alias */
#ifdef undef
		     &username,		/* username */
#endif
		     &dot,		/* "." */
		     &qstr,		/* quotedstring */
		     &comment,		/* (comment) */
		     &mbox,		/* <mbox> */
		     &groupend,		/* ";" */
		     &remusr,		/* remote user */
		     nil), &pv, &used);
    if (used == &confirm) {		/* confirm: done with this parse */
	if (*this != nil) {		/* if anything had been typed */
	    add_addresslist(a,*this,ADR_ADDRESS); /* add it to the list */
	    *this = safe_free(*this);
	}
	return(0);
	
    }
    else if (used == &asterix) {	/* "*"? */
	if (directory_folders)
	    outfile._cmffl &= ~FIL_NODIR;
	else
	    outfile._cmffl |= FIL_NODIR;
	parse(fdbchn(&outfile,nil),&pv,&used);	/* get a filename */
#ifdef undef
	*this = safe_strcat(*this,"*",TRUE); /* copy it somewhere */
#endif
	*this = safe_strcat(*this,pv._pvfil[0],FALSE);
	add_addresslist(a,*this,ADR_FILE);
	return(parse_sep2(a,this,one));	/* get a comma or confirm */
    }
    else if (used == &indirect) {	/* @filename */
	FILE *newinf;			/* new input file */
	char name[100];
	parse(fdbchn(&indfile,nil),&pv,&used);
	strcpy(name,pv._pvfil[0]);
	parse(fdbchn(&confirm,nil),&pv,&used);
	newinf = fopen(name, "r");
	if (!newinf) {
	    cmxeprintf("?Could not open %s\n",name);
	    return(0);
	}
	ind_oldfds();			/* tell ccmd we are in indirect mode */
	cmseti(newinf, nil, cmcsb._cmej); /* install new input file */
	cmcsb._cmflg2 |= CM_IND;	/* set indirection bit.  EOF handled */
					/* through ccmd */
	return(1);			/* keep parsing from new file. */
    }
    else if (used == &dot) {		/* address = "." */
	*this = safe_strcat(*this,user_name,TRUE);
	return(parse_addr1(a,this,one)); /* keep parsing. */
    }
    else if (used == &qstr) {		/* "address" */
	*this = safe_strcat(*this,"\"",TRUE);
	*this = safe_strcat(*this,atmbuf,FALSE);
	*this = safe_strcat(*this,"\"",FALSE);
	return(parse_addr1(a,this,one));
    }
    else if (used == &comment) {
	parse_comment(a,this);		/* (comment) */
	return(parse_address(a,this,one)); /* comma or confirm */
    }	
    else if (used == &remusr) {
	*this = safe_strcat(*this, atmbuf,TRUE);
	return(parse_addr1(a,this,one)); /* possibly more address */
    }
    else if (used == &aliasfdb) {
#ifdef notdef
	*this = safe_strcat(*this,
			 ((keytab *)aliasfdb._cmdat)->_ktwds[pv._pvkey]._kwkwd,
			    TRUE);
#endif
	*this = safe_strcat (*this, atmbuf, TRUE);
	if (index(*this,'@'))
	    return(parse_addr4(a,this,one));
	else
	    return(parse_addr3(a,this,one)); /* maybe just a phrase */
    }
    else if (used == &username) {
	*this = safe_strcat(*this, pv._pvusr[0]->pw_name,TRUE);
	return(parse_addr1(a,this,one)); /* possibly more address */
    }
    else if (used == &mbox) {		/* <mbox> */
	parse_mbox(a,this);
	return(parse_sep1(a,this,one));	/* comma or confirm */
    }
    else if (used == &groupend) {
	if (active_label) {
	    add_addresslist(a,";",ADR_GROUPEND);
	}
	parse_sep2(a,this,one);
    }
    else if (used == &mlist) {
	parse(&listfile,&pv,&used);
	add_addresslist(a,pv._pvfil[0], ADR_LISTFILE);
	return(parse_sep1(a,this,one));
    }
}

/*
 * called when the first token of an address has been parsed, 
 * and we need to parse more and see where it leads us.
 */

parse_addr1(a,this,one)			/* parse the rest of an address */
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    parse(fdbchn(&confirm,
		 &at,			/* @ host */
		 &mail11,		/* host:: */
		 &group,		/* ":" */
		 &comment,
		 &mbox,			/* phrase <mbox> */
		 &groupend,		/* ";" to end a group */
		 &qstr,
		 &comma,
		 &field,		/* continued phrase */
		 nil), &pv, &used);
    if (used == &mbox) {
	parse_mbox(a,this);		/* finish the mbox */
	return(parse_sep1(a,this,one));	/* comma or confirm */
    }
    else if (used == &field) {
	*this = safe_strcat(*this, atmbuf,TRUE);
	return(parse_addr2(a,this,one));
    }
    else if (used == &group) {		/* a field name */
	if (active_label) {		/* alread in one?   close it off */
	    add_addresslist(a,";",ADR_GROUPEND);
	}
	add_addresslist(a,*this,ADR_GROUP);
	*this = safe_free(*this);
	return(1);			/* continue the parse */
    }
    else if (used == &mail11) {		/* MAY ACCEPT BOGUS ADDRESSES */
	*this = safe_strcat(*this, "::", FALSE);
	return(parse_address_1(a,this,one));
    }
    else if (used == &at) {		/* @ host */
 	parse_host(a,this);		/* get the host */
	return(parse_sep1(a,this,one));	/* comma or confirm */
    }
    else if (used == &comma) {		/* comma */
	add_addresslist(a,*this,ADR_ADDRESS); /* record this address */
	*this = safe_free(*this);
	return(1);			/* parse for more addresses*/
    }
    else if (used == &confirm) {	/* confirm, all done; */
	add_addresslist(a,*this,ADR_ADDRESS); /* record this address */
	*this = safe_free(*this);
	return(0);			/* pop all the way up */
    }
    else if (used == &comment) {	/* comment, */
	parse_comment(a,this);		/* get the comment */
	parse_addr1(a,this,one);		/* keep trying */
    }
    else if (used == &qstr) {		/* "address" */
	*this = safe_strcat(*this,"\"",TRUE);
	*this = safe_strcat(*this,atmbuf,FALSE);
	*this = safe_strcat(*this,"\"",FALSE);
	return(parse_addr2(a,this,one));
    }
    else if (used == &groupend) {
	add_addresslist(a,*this, ADR_ADDRESS); /* save the address */
	*this = safe_free(*this);
	add_addresslist(a,";", ADR_GROUPEND); /* post the group end */
	return(parse_sep2(a,this,one));	/* try for a comman or confirm */
    }
}

/*
 * parse a phrase so far (two words or more).
 * allow it to be made into a group name, or allow a <mbox>.  
 * a comment can be inserted, or the phrase can continue.
 */
parse_addr2(a,this,one)			/* parse a phrase and mailbox*/
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    parse(fdbchn(&group,		/* ":" */
		 &mbox,			/* phrase <mbox> */
		 &comment,
		 &qstr,
		 &field,		/* continued phrase */
		 nil), &pv, &used);
    if (used == &mbox) {
	parse_mbox(a,this);		/* finish the mbox */
	return(parse_sep1(a,this,one));	/* comma or confirm */
    }
    else if (used == &group) {		/* a field name */
	add_addresslist(a,*this,ADR_GROUP);
	*this = safe_free(*this);
	return(1);			/* continue the parse */
    }
    else if (used == &comment) {	/* comment, */
	parse_comment(a,this);		/* get the comment */
	parse_addr2(a,this,one);	/* keep trying */
    }
    else if (used == &qstr) {		/* "address" */
	*this = safe_strcat(*this,"\"",TRUE);
	*this = safe_strcat(*this,atmbuf,FALSE);
	*this = safe_strcat(*this,"\"",FALSE);
	return(parse_addr2(a,this,one));
    }
    else if (used == &field) {
	*this = safe_strcat(*this, atmbuf,TRUE);
	return(parse_addr2(a,this,one));
    }
}

/*
 * called when the first token of an address has been parsed, and is an alias
 * and we need to parse more and see where it leads us.
 */

parse_addr3(a,this,one)			/* parse the rest of an address */
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    parse(fdbchn(&confirm,
		 &comma,
		 &at,			/* @ host */
		 &group,		/* ":" */
		 &comment,
		 &mbox,			/* phrase <mbox> */
		 &groupend,		/* ";" to end a group */
		 &field,		/* continued phrase */
		 nil), &pv, &used);
    if (used == &mbox) {
	parse_mbox(a,this);		/* finish the mbox */
	return(parse_sep1(a,this,one));	/* comma or confirm */
    }
    else if (used == &field) {
	*this = safe_strcat(*this, atmbuf,TRUE);
	return(parse_addr2(a,this,one));
    }
    else if (used == &group) {		/* a group name */
	if (active_label) {		/* alread in one?   close it off */
	    add_addresslist(a,";",ADR_GROUPEND);
	}
	add_addresslist(a,*this,ADR_GROUP);
	*this = safe_free(*this);
	return(1);			/* continue the parse */
    }
    else if (used == &at) {		/* @ host */
 	parse_host(a,this);		/* get the host */
	return(parse_sep1(a,this,one));	/* comma or confirm */
    }
    else if (used == &comma) {		/* comma */
	add_addresslist(a,*this,ADR_ALIAS); /* record this address */
	*this = safe_free(*this);
	return(1);			/* parse for more addresses*/
    }
    else if (used == &confirm) {	/* confirm, all done; */
	add_addresslist(a,*this,ADR_ALIAS); /* record this address */
	*this = safe_free(*this);
	return(0);			/* pop all the way up */
    }
    else if (used == &comment) {	/* comment, */
	parse_comment(a,this);		/* get the comment */
	parse_addr3(a,this,one);		/* keep trying */
    }
    else if (used == &groupend) {
	add_addresslist(a,*this, ADR_ALIAS); /* save the address */
	*this = safe_free(*this);
	add_addresslist(a,";", ADR_GROUPEND); /* post the group end */
	return(parse_sep2(a,this,one));	/* try for a comman or confirm */
    }
}


/* 
 * after an alias with an @ in it.
 */
parse_addr4(a,this,one)			/* parse the rest of an address */
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    parse(fdbchn(&confirm,
		 &comma,
		 &comment,
		 &groupend,		/* ";" to end a group */
		 nil), &pv, &used);

    if (used == &comma) {		/* comma */
	add_addresslist(a,*this,ADR_ALIAS); /* record this address */
	*this = safe_free(*this);
	return(1);			/* parse for more addresses*/
    }
    else if (used == &confirm) {	/* confirm, all done; */
	add_addresslist(a,*this,ADR_ALIAS); /* record this address */
	*this = safe_free(*this);
	return(0);			/* pop all the way up */
    }
    else if (used == &comment) {	/* comment, */
	parse_comment(a,this);		/* get the comment */
	parse_addr4(a,this,one);	/* keep trying */
    }
    else if (used == &groupend) {
	add_addresslist(a,*this, ADR_ALIAS); /* save the address */
	*this = safe_free(*this);
	add_addresslist(a,";", ADR_GROUPEND); /* post the group end */
	return(parse_sep2(a,this,one));	/* try for a comman or confirm */
    }
}


/*
 * between addresses.   
 * a comma means get more addresses.  confirm means all done.
 * comments, as always are applicable
 */

parse_sep1(a,this,one) 
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    do {
	if (!one) 
	    parse(fdbchn(&confirm,
			 &comma, 
			 &comment,
			 &groupend,
			 nil), &pv, &used);
	else
	    parse(fdbchn(&confirm,
			 &comment,
			 &groupend,
			 nil), &pv, &used);
	if (used == &comment) 
	    parse_comment(a,this);
	else if (used == &groupend) {
	    if (*this) {
		add_addresslist(a,*this,ADR_ADDRESS);
		*this = safe_free(*this);
	    }
	    add_addresslist(a,";",ADR_GROUPEND);
	}
	else {
	    if (*this) {
		add_addresslist(a,*this,ADR_ADDRESS);
		*this = safe_free(*this);
	    }
	}
    } while (used != &comma && used != &confirm);
    return(used == &comma);
}

/*
 * sep2: parse a separator (no group ends), and don't add anything to the
 * addresslist.
 */
parse_sep2(a,this,one) 
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    do {
	if (!one) 
	    parse(fdbchn(&confirm,
			 &comma, 
			 &comment,
			 nil), &pv, &used);
	else
	    parse(fdbchn(&confirm,
			 &comment,
			 nil), &pv, &used);

	if (used == &comment) {
	    if (*this) {
		free(*this);
		*this = nil;
	    }
	    parse_comment(a,this);
	}
    } while (used == &comment);
    return(used == &comma);
}

parse_comment(a,this) 
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    *this = safe_strcat(*this,"(",TRUE);
    do {
	parse(fdbchn(&commentend,
		     &comment,
		     &commenttxt,
		     nil), &pv, &used);
	if (used == &comment) {
 	    parse_comment(a,this);
	}
	else if (used == &commentend) {
	    *this = safe_strcat(*this,")",FALSE);
	}
	else if (used == &commenttxt) {
	    if (*this && strlen(*this) > 0 && (*this)[strlen(*this)-1] !='(')
		*this = safe_strcat(*this,atmbuf,TRUE);
	    else
		*this = safe_strcat(*this,atmbuf,FALSE);
	}
    } while(used != &commentend);
}  

parse_host(a, this) 
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;
    
    parse(fdbchn(&ipaddr,
		 &hostname,
		 nil), &pv, &used);
    if (used == &ipaddr)
	parse_ipaddr(a, this);
    else {
	*this = safe_strcat(*this,"@",FALSE);
	*this = safe_strcat(*this,atmbuf,FALSE);
    }
}    

parse_ipaddr(a, this)
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;
    int i;

    *this = safe_strcat(*this,"@[",FALSE);
    for(i = 0; i < 4; i++) {
	parse(fdbchn(&ipaddrnum,nil), &pv, &used);
	*this = safe_strcat(*this, atmbuf,FALSE);
	if (pv._pvint > 255) {
	    cmxbol();
	    cmxeprintf("?Address value > 255\n");
	    ccmd_errnp(CMxOK);
	}
	if (i == 3) break;
	parse(fdbchn(&ipaddrdot, nil), &pv, &used);
	*this = safe_strcat(*this,".",FALSE);
    }
    parse(fdbchn(&ipaddrend, nil), &pv, &used);
    *this = safe_strcat(*this,"]",FALSE);
}

parse_mbox(a, this)
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    *this = safe_strcat(*this, "<",TRUE);
    parse(fdbchn(
#ifdef undef
		 &username,
#endif
		 &at,
		 &remusr,
		 nil), &pv, &used);
    if (used == &at) {
	parse_mbox2(a,this);
	parse(fdbchn(
#ifdef undef
		     &username,
#endif
		     &remusr,
		     nil), &pv, &used);
    }
    if (used == &username)
	*this = safe_strcat(*this, pv._pvusr[0]->pw_name,FALSE);
    else
	*this = safe_strcat(*this, atmbuf, false);
    parse(fdbchn(&mboxend,
		 &at,
		 nil), &pv, &used);
    if (used == &at) {
	parse_host(a,this);
	parse(fdbchn(&mboxend, nil), &pv, &used);
    }
    *this = safe_strcat(*this, ">", false);
}


/*
 * saw a [route] in a mailbox
 */
parse_mbox2(a,this) 
addresslist *a;
char **this;
{
    pval pv;
    fdb *used;

    parse_host(a,this);
    parse(fdbchn(&comma2,
		 &colon,
		 nil), &pv, &used);
    if (used == &comma2) {
	*this = safe_strcat(*this, ",", FALSE);
	parse(fdbchn(&at,nil),&pv,&used);
	parse_mbox2(a,this);
    }
    else {	
	*this = safe_strcat(*this,atmbuf, false);
    }
}


free_addresslist(a) 
addresslist *a;
{
    addr_unit *a1, *a2;
    for (a1 = a->first; a1 != a->last; ) {
	a2 = a1->next;
	free_address(a1);
	a1 = a2;
    }
    a->first = a->last = nil;
}

free_address(a) 
addr_unit *a;
{
    if (a) {
	if (a->data) {
	    free(a->data);
	}
	free(a);
    }
}

add_addresslist(a,str,type)
addresslist *a;
char *str;
int type;
{
    addr_unit *au;
    addresslist *aliasaddr;
    if (type == ADR_GROUPEND && !active_label)
	return;
    if (((!aliases_use_groups) || active_label) && (type == ADR_ALIAS)) {
	aliasaddr = (addresslist *) lookup_alias (str);
	if (aliasaddr)
	    for (au = aliasaddr->first; au; au = au->next)
		if (au->type != ADR_GROUP && au->type != ADR_GROUPEND &&
		    au->type != ADR_AL_EXPAND)
		    add_addresslist (a,au->data, au->type);
	return;
    }
    if (type != ADR_MLIST) {
	if (a->last == nil) {
	    a->last = a->first = (addr_unit *) malloc(sizeof(addr_unit));
	    a->first->next = a->first->prev = nil;
	}
	else {
	    a->last->next = (addr_unit *) malloc(sizeof(addr_unit));
	    a->last->next->prev = a->last;
	    a->last = a->last->next;
	    a->last->next = nil;
	}
	a->last->type = type;
    }

    switch (type) {
    case ADR_ADDRESS:
    case ADR_FILE:
	a->last->data = malloc(strlen(str)+1);
	strcpy(a->last->data, str);
	break;
    case ADR_GROUP:
    case ADR_AL_EXPAND:
	a->last->data = malloc(strlen(str)+1);
	strcpy(a->last->data, str);
	active_label = TRUE;
	break;
    case ADR_GROUPEND:
	if (active_label) {
	    a->last->data = malloc(strlen(str)+1);
	    strcpy(a->last->data, str);
	    active_label = FALSE;
	}
	break;
    case ADR_ALIAS: 
	adr_alias(a,str);
	break;
    case ADR_LISTFILE:
	a->last->data = malloc(MAXPATHLEN+2);
	if (str[0] == '/')		/* abs path already? */
	    strcpy(a->last->data, str);
	else {
	    getwd(a->last->data);
	    strcat(a->last->data, "/");
	    strcat(a->last->data, str);
	}
	a->last->type = ADR_MLIST;	/* make it get expanded next time. */
	break;
    case ADR_MLIST:
	adr_mlist(a, str);
	break;
    }
}

adr_mlist(a, str)
addresslist *a;
char *str;
{
    FILE *newinf;		/* new input file */
    static addresslist a1 = { nil, nil };
    csb savecsb;
    char abuf[BUFSIZ], wbuf[BUFSIZ];
    int cbuf[BUFSIZ*5];

    newinf = fopen(str, "r");
    if (!newinf) {
	cmxeprintf("Could not open %s\n",str);
    }
    else {
	savecsb = cmcsb;
	cmbufs(cbuf,10*BUFSIZ, abuf,BUFSIZ, wbuf,BUFSIZ);
	ind_oldfds();			/* set up indirect mode */
					/* install new input file */
	cmseti(newinf, nil, cmcsb._cmej);
					/* set indirection bit.  EOF handled */
	cmcsb._cmflg2 |= CM_IND;	/* through ccmd */

	parse_addresses(&a1);		/* parse from list file. */
	cmcsb = savecsb;
	fclose(newinf);
	merge_addresses(a,&a1);
	free_addresslist(&a1);
    }
}

adr_alias(a,str)
addresslist *a;
char *str;
{
    addresslist *aliasaddr;
    addr_unit *au;

    aliasaddr = lookup_alias(str);
    if (aliasaddr) {
	if (active_label) {
	    a->last->data = safe_strcpy(";");
	    a->last->type = ADR_GROUPEND;
	    active_label = false;
	    add_addresslist(a,str,ADR_AL_EXPAND);
	}
	else {
	    a->last->data = safe_strcpy(str);
	    a->last->type = ADR_AL_EXPAND;
	    active_label = true;
	}
	au = aliasaddr->first;
	for( ; au != nil; au = au->next) {
	    if (au->type != ADR_GROUP && au->type != ADR_GROUPEND
		&& au->type != ADR_AL_EXPAND)
		add_addresslist(a,au->data, au->type);
	}
    }
    if (active_label)
	add_addresslist(a,";", ADR_GROUPEND);
}


disp_addresses(fp,prefix,a,expand,newline,smail,file_asterix) 
char *prefix;
addresslist *a;
int expand;
FILE *fp;
int newline;
int smail;
{
    int linebeg;			/* any addresses on this line yet? */

    int endcol = cmcsb._cmcmx-1;	/* leave space for "," */
    addr_unit *au = a->first;
    int len = 0;

    if (a->first && prefix) {
	fprintf(fp,"%s: ",prefix);
	len = strlen(prefix)+2;
    }
    linebeg = TRUE;			/* no addresses printed yet */
    while(au) {
	if (len + disp_address(fp,au,FALSE,smail,file_asterix) > endcol)
	    /* won't fit on this line */
	    if (!linebeg) {		/* anything on this line yet? */
		len = 3;		/* 3 spaces, another one later */
		if (newline) fprintf(fp,"\n   "); /* three spaces */
		linebeg = TRUE;		/* at line beginning again */
	    }				/* else, won't fit anyway */
	if (au != a->first) {
	    len += 1;
	    fputc (' ', fp);
	}
	len += disp_address(fp,au,TRUE,smail,file_asterix);
	linebeg = FALSE;		/* we wrote something on this line */
	if ((au->type == ADR_GROUP || au->type == ADR_AL_EXPAND) && !expand) {
	    while(au->next->type != ADR_GROUPEND)
		au = au->next;
	}
	else if (!(au->type == ADR_GROUP || au->type == ADR_AL_EXPAND) &&
		 au->next && au->next->type != ADR_GROUPEND) {
	    len += 1;
	    fputc (',', fp);
	}
	if (au) au = au->next;
    }
    if (len > 0 && newline) fputc('\n', fp);
}

int
disp_address(fp,a,prflag,smail,file_asterix)
FILE *fp;
addr_unit *a; 
int prflag;
int smail;
{
    char *str;
    int len;
    switch(a->type) {
    case ADR_ADDRESS:
    case ADR_ALIAS:
	str = malloc(strlen(a->data)+1);
	if (smail && a->data[0] == '\\')
	  strcpy(str,a->data+1);
	else
	  strcpy(str,a->data);
	break;
    case ADR_FILE:
	str = malloc(strlen(a->data)+2);
	sprintf(str,"%s%s",file_asterix ? "*" : "", a->data);
	break;
    case ADR_GROUP:
    case ADR_AL_EXPAND:
	str = malloc(strlen(a->data)+2);
	strcpy(str,a->data);
	strcat(str,":");
	break;
    case ADR_GROUPEND:
	str = malloc(strlen(a->data)+1);
	strcpy(str,a->data);
	break;
    case ADR_LISTFILE:
    case ADR_MLIST:
	str = malloc(strlen(a->data)+3);
	sprintf(str,"@@%s",a->data);
	break;
    default:
	printf("Invalid flag in message\n");
	fflush(stdout);
	abort();
    }
    if (prflag)
	fprintf(fp,"%s",str);
    len = strlen(str);
    free(str);
    return(len);
}

/*
 * merge list 2 into list 1
 */

merge_addresses(a1,a2) 
addresslist *a1, *a2;
{
    addr_unit *au1 = a1->first, *au2 = a2->first;

    while(au2) {
	au1 = a1->first;
	switch(au2->type) {
	case ADR_GROUP:			/* adding a group... */
	case ADR_AL_EXPAND:
	    while(au1 && !(au1->type == ADR_GROUP ||
			   au1->type == ADR_AL_EXPAND ||
			   !ustrcmp(au1->data, au2->data)))
		au1 = au1->next;
	    if (au1 == nil) {	/* add group to end of list */
		add_addresslist(a1,au2->data,ADR_GROUP);
		au2 = au2->next;
		while(au2 && au2->type != ADR_GROUPEND) {
		    add_addresslist(a1,au2->data, ADR_ADDRESS);
		    au2 = au2->next;
		}
		add_addresslist(a1,";",ADR_GROUPEND);
	    }
	    else {			/* add to an existing group */
		addr_unit *temp, *temp2;
		while(au1->next->type != ADR_GROUPEND)
		    au1 = au1->next;	/* find end of group */
		temp = au1->next;	/* save old values */
		temp2 = a1->last;
		a1->last = au1;
		au1->next = nil;
		au2 = au2->next;	/* skip group name */
		while(au2->type != ADR_GROUPEND) { /* add new members */
		    add_addresslist(a1,au2->data, ADR_ADDRESS);
		    au2 = au2->next;
		}
		a1->last->next = temp;	/* restore old values */
		a1->last = temp2;
	    }
	    break;
	case ADR_ADDRESS:
	case ADR_ALIAS:
	case ADR_FILE:
	    add_addresslist(a1,au2->data, au2->type);
	    break;
	}
	if (au2)
	    au2 = au2->next;
    }
}

/*
 * remove addresses in list 2 from list 1.
 * entries in list 2:
 *  empty group list: delete whole group from list 1.
 *  group list: delete selected members from list 1.
 *  simple address: delete that address.
 * comments count.
 */

remove_addr(a1,a2) 
addresslist *a1, *a2;
{
    addr_unit *au1, *au2, *t1, *t2;

    au2 = a2->first;
    
    while(au2) {			/* all items to delete... */
	au1 = a1->first;		/* deleting a group */
	if (au2->type == ADR_GROUP || au2->type == ADR_AL_EXPAND) {
	    if (au2->next->type == ADR_GROUPEND) { /* a whole group */
		while(au1 && !(au1->type == ADR_GROUP || 
			       au1->type == ADR_AL_EXPAND ||
			       !ustrcmp(au1->data, au2->data)))
		    au1 = au1->next;
		if (au1 != nil) {	/* found the group */
		    t1 = au1;
		    while(t1->type != ADR_GROUPEND) {
			t1 = t1->next;
		    }			/* now t1 is the end, */
					/*  au1 is the start */
		    if (a1->first == au1)
			a1->first = t1->next;
		    if (a1->last == t1)
			a1->last = au1->prev;
		    if (au1->prev)
			au1->prev->next = t1->next;
		    if (t1->next)
			t1->next->prev = au1->prev;
		    while(au1 != t1) {
			t2 = au1->next;
			free_address(au1);
			au1 = t2;
		    }
		    free_address(au1);
		    au2 = au2->next;
		}
	    }
	    else {			/* removing selected id's from list */
		while(au1 && !(au1->type == ADR_GROUP || 
			       au1->type == ADR_AL_EXPAND ||
			       !ustrcmp(au1->data, au2->data)))
		    au1 = au1->next;
		if (au1 != nil) {	/* found the group */
		    au2 = au2->next;
		    while(au2->type != ADR_GROUPEND) {
			t1 = au1->next;
			while(t1->type != ADR_GROUPEND &&
			      addrcmp(t1->data, au2->data)) 
			    t1 = t1->next;
			if (t1->type != ADR_GROUPEND) {
			    t1->next->prev = t1->prev;
			    t1->prev->next = t1->next;
			    free_address(t1);
			}
			au2 = au2->next;
		    }
		}
	    }
	}
	else {
	    while(au1) {
		if (au1->type != au2->type  || addrcmp(au1->data, au2->data))
		    au1 = au1->next;
		else {
		    if (a1->first == au1)
			a1->first = au1->next;
		    if (a1->last == au1)
			a1->last = au1->prev;
		    if (au1->prev)
			au1->prev->next = au1->next;
		    if (au1->next)
			au1->next->prev = au1->prev;
		    t1 = au1->next;
		    free_address(au1);
		    au1 = t1;
		}
	    }
	}
	au2 = au2->next;
    }
}

/*
 * this should strip out comments, and check for <mailboxes> which match...
 */
addrcmp(a1, a2) 
char *a1, *a2;
{
    char *b, *cp, *uname, *index(), *rindex();
    int x;

    if (ustrcmp(a1,a2) == 0)		/* exact match? */
	return(0);
					/* match up to a domain? */
    if (ustrncmp(a1,a2,strlen(a2)) == 0 && a1[strlen(a2)] == '.' &&
	index(a1,'@'))
	return(0);

    b = malloc(strlen(a1)+1);		/* copy to a workspace. */
    strcpy(b,a1);
    
    if ((cp = index(b,'<')) != nil)	/* if an address in <>, use it */
	cp++;
    else cp = b;
    if ((uname = index(cp,'>')) != nil)
	*uname = '\0';

    if (ustrcmp(cp,a2) == 0)  {		/* exact match now? */
	free(b);
	return(0);
    }
					/* a match up to a domain? */
    if (ustrncmp(cp,a2,strlen(a2)) == 0 && cp[strlen(a2)] == '.' &&
	index(cp,'@')) {
	free(b);
	return(0);
    }
					/* find the user name */
    if ((uname = rindex(cp,'!')) != nil) /* to the right of any '!'s */
	uname = uname+1;
    else 
	uname = cp;

    if (ustrcmp(uname,a2) == 0)		/* exact match now? */
	return(0); 

					/* without some domains? */
    if ((ustrncmp(uname,a2,strlen(a2)) == 0) && (uname[strlen(a2)] == '.')
	&& (index(uname,'@') || index(uname,'%'))) {
	free(b);
	return(0);
    }

    if ((cp = index(uname,'@')) != nil) /* strip off host names */
	*cp = '\0';
    if ((cp = index(uname,'%')) != nil)
	*cp = '\0';
    x = ustrcmp(uname, a2);		/* match now? */
    free(b);
    return(x);
}


skip_spaces(str)
char **str;
{
    while(**str == ' ' || **str == '\t')
	(*str)++;
}
