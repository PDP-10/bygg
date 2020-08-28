/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /amd/sherluck/sh/src0/sun4.bin/cucca/mm/RCS/misc.c,v 2.6 95/03/30 15:59:44 howie Exp $";
#endif

/*
 * misc.c - miscellaneous commands
 */

#include "mm.h"
#include "parse.h"
#include "help.h"
#include "rd.h"

#if defined(HAVE_QUOTAS) && !defined(F_QUOTACTL)
#   ifdef HAVE_QUOTACTL
#	include <ufs/quota.h>
#	include <mntent.h>
#   else
#	include <sys/quota.h>
#   endif
#endif


int
cmd_version (n)
int n;
{
    confirm ();
    printf ("%s\n", mm_version);
    puts("Copyright (c) 1986, 1990\n\
\tThe Trustees of Columbia University in the City of New York");
    printf ("Compiled %s\n", mm_compiled);
    printf ("Report bugs to %s\n", BUGSTO);
}

#ifdef HAVE_QUOTAS
#define ST_GOODQUOTA 0
#define ST_BADQUOTA  1
#define ST_NOQUOTA   2
#endif

int
cmd_status (n)
int n;
{
    int verbose = FALSE;
#ifdef HAVE_QUOTAS
    int quotaflag = ST_GOODQUOTA;
    struct dqblk qblk;
#ifdef HAVE_QUOTACTL
    char *bdev, *getspecialname();
#endif
#endif
    struct stat sbuf;
    static fdb conffdb = { _CMCFM, CM_SDH, NULL, NULL, 
			       "confirm for status summary ", 
			       NULL, NULL};
    static keywrd ver_key[] = {
    	{ "verbose", 0 , (keyval) 0 },
    };
    static keytab vertab = { sizeof (ver_key) / sizeof (keywrd),
			      ver_key };
    static fdb verswi = { _CMKEY, 0, NULL, (pdat) &vertab };


    parse (fdbchn (&verswi, &conffdb, NULL), &pv, &used);
    if (used == &verswi) {
	confirm();
	verbose = TRUE;
    }

    show_route(TRUE);			/* show mail routing */

    if (cf) {
	int i, nrec = 0, ndel = 0, nuns = 0;
	for (i = 0; i < cf->count; i++)	{
	    if (cf->msgs[i+1].flags & M_DELETED)
		ndel++;
	    if (cf->msgs[i+1].flags & M_RECENT)
		nrec++;
	    if (!(cf->msgs[i+1].flags & M_SEEN))
		nuns++;
	}
	printf (" File %s (%s%s%s)\n",cf->filename, msg_ops[cf->type].typename,
		(cf->flags&(MF_DIRTY|MF_MODIFIED) ? ", modified" : ""),
		(cf->flags&MF_RDONLY) ? ", read-only" : "");
	printf (" %d messages, %d old, %d deleted, %d unseen, %dk Bytes\n",
		cf->count, cf->count - nrec, ndel, nuns, (cf->size+512)/1024);
	printf (" Currently at message %d\n", cf->current);
    }
    else
	printf (" No current mail file\n");
    if (!verbose)
	return;				/* done */
#ifdef HAVE_QUOTAS
    if (cf == NULL)
	quotaflag = ST_NOQUOTA;
    else {
	if (fstat (fileno(cf->filep), &sbuf) != 0) {
	    perror (cf->filename);
	    quotaflag = ST_BADQUOTA;
	}
#ifdef HAVE_QUOTACTL
	else if ((bdev = getspecialname (sbuf.st_dev)) == NULL) {
	    fprintf (stderr, "%s: couldn't get block special device\n", 
		     cf->filename);
	    quotaflag = ST_BADQUOTA;
	}
	else if (quotactl (Q_GETQUOTA, bdev, UID, &qblk) != 0)
	    quotaflag = ST_NOQUOTA;	/* assume this, random error codes */
#else
	else if (quota (Q_GETDLIM, UID, (int) sbuf.st_dev, &qblk) != 0)
	    quotaflag = ST_NOQUOTA;	/* assume this, random error codes */
#endif
	if (quotaflag != ST_NOQUOTA) {
	    printf (" Blocks free: ");
	    if (quotaflag == ST_BADQUOTA)
		printf ("unknown\n");
	    else {				/* ST_GOODQUOTA */
		u_long i;
		i = qblk.dqb_bhardlimit - qblk.dqb_curblocks;
		printf ("%ld (%ld kb)\n", i, (dbtob(i)>>10)); /* 2^10 = 1 kb */
	    }
	}
    }
#endif
    printf (" User: %s <%s@%s>\n", 
	    real_personal_name ? real_personal_name : "",
	    user_name, fullhostname);
    printf (" Process ID = %d, User ID = %d\n", PID, UID);
}

#ifdef HAVE_QUOTAS
#ifdef HAVE_QUOTACTL
/*
 * getspecialname:
 * get the block special device name for this major/minor number
 * cache the result, since they'll probably run stat on the same file
 * over and over (or at least the same filesystem)
 */
char *
getspecialname(filedev)
dev_t filedev;
{
    FILE *mntf;
    struct mntent *ent;
    struct stat statb;
    static dev_t oldfiledev = 0;
    static char *oldspecialname = NULL;

    if (oldfiledev == filedev)		/* same partition as last time */
	return (oldspecialname);

    if ((mntf = setmntent (MNTTAB, "r")) == NULL)
	return (NULL);
    while ((ent = getmntent(mntf)) != NULL) {
	if (stat(ent->mnt_dir, &statb) != 0) /* look at device */
	    return (NULL);
	if (statb.st_dev == filedev) {	/* the right raw device? */
	    endmntent(mntf);		/* be neat */
	    safe_free (oldspecialname);
	    if ((oldspecialname = malloc (strlen (ent->mnt_fsname) +1))
		!= NULL){
		strcpy (oldspecialname, ent->mnt_fsname); /* cache it */
		oldfiledev = filedev;
	    }
	    else
		oldfiledev = 0;		/* flag no cached name */
	    return (ent->mnt_fsname);
	}
    }
    endmntent(mntf);
    return (NULL);			/* didn't find it */
}
#endif /* HAVE_QUOTACTL */
#endif /* HAVE_QUOTAS */

blank ()
{
    if (cmcsb._cmoj)
	return (cmcls ());
    /* XXX shouldn't have to do it this way, but cmcsb._cmoj is
       NULL when we're throwing away output inside init files, but
       we want to let users clear the screen in that context */
    cmcsb._cmoj = stdout;
    cmcls();
    cmcsb._cmoj = (FILE *) NULL;
}

int
cmd_blank (cmd)
int cmd;
{
    confirm ();
    blank ();
}

int
cmd_daytime (n)
int n;
{
    buffer buf;

    static fdb seq_fdb_tad = { _CMTAD };

    parse (fdbchn (&seq_fdb_tad, &cfm_fdb, nil), &pv, &used);
    if (used == &seq_fdb_tad) {
	time_t t = datimetogmt (&pv._pvtad);
	confirm ();
	printf ("Local time: %s\n", daytime (t));
	return;
    }
    printf ("%s\n", daytime ((time_t) 0));
}

int
cmd_echo (n)
int n;
{
    parse_text ("string to echo", nil);
    printf ("%s\n", atmbuf);
}

ustrncmp(str1, str2, n)
char *str1, *str2;
int n;
{
    char s1, s2;
    while(n > 0) {
 	s1 = islower(*str1) ? toupper(*str1) : *str1;
 	s2 = islower(*str2) ? toupper(*str2) : *str2;

	if (s1 == s2) {
	    if (s1 == '\0') return(0);
	    else {
		str1++; str2++; n--;
	    }
	    continue;
	}
	if (s1 > s2) return(1);
	if (s1 < s2) return(-1);
    }
    return(0);
}

int 
cmd_cd (n)
int n;
{
    pval parseval;			/* ccmd parse return structure */
    fdb *used;				/* which fdb was used */
    static fdb conffdb = { _CMCFM, CM_SDH, NULL, NULL, 
			       "confirm to connect to your home directory", 
			       NULL, NULL};
    static fdb dirfdb = { _CMFIL, FIL_DIR, 
			      NULL, NULL, "directory", NULL, NULL };

    noise ("to directory");

    dirfdb._cmdef = HOME;		/* set default */
    parse (fdbchn(&dirfdb,&conffdb,NULL), &parseval, &used);
    if (used == &dirfdb) {
	confirm();
	if (chdir (parseval._pvfil[0]) != 0) {
	    perror ("cd");
	    return;
	}
	return;
    }
    /* else cd to HOME */
    if (HOME == NULL) {
	fprintf (stderr, "Can't find home directory.\n");
	return;
    }
    if (chdir (HOME) != 0) {
	perror ("cd");
	return;
    }
}


int
cmd_pwd (n)
int n;
{
  char wd[MAXPATHLEN];
  char *getwd();

  confirm();
  if (getwd(wd) == NULL) {
    fprintf (stderr, "%s\n", wd);
    return;
  }
  else
    printf ("%s\n", wd);
}


#define CS (cf->sequence)
#define PS (cf->prev_sequence)
#define RS (cf->read_sequence)

#define ST_DATE 0x01
#define ST_SUBJ 0x02
#define ST_FROM 0x04
#define ST_SIZE 0x08
#define ST_END  0

#define MAXSORTKEYS 100
int sortkeys[MAXSORTKEYS];

static char *
shtext(s1, s2)
char *s1, *s2;
{
    char *r, *htext();
    r = htext(s1, s2);
    if (r == NULL)
	r = (char *)calloc(1,1);
    return r;
}

msgcmp(a,b)
message *a, *b;
{
    int i,r;
    char *cp1, *cp2, *bp1, *bp2;

    for(i = 0; sortkeys[i] != ST_END; i++) {
	switch (sortkeys[i]) {
	case ST_DATE:
	    if (a->date < b->date)
		return(-1);
	    if (a->date > b->date)
		return(1);
	    break;
	case ST_SIZE:
	    if (a->size < b->size)
		return(-1);
	    if (a->size > b->size)
		return(1);
	    break;
	    
	case ST_SUBJ:
	    bp1 = cp1 = shtext("subject", a->text);
	    bp2 = cp2 = shtext("subject", b->text);
	    if (strncasecmp(cp1, "re:", 3) == 0) {
		bp1+=3;
		while(isspace(*bp1)) bp1++;
	    }
	    if (strncasecmp(cp2, "re:", 3) == 0) {
		bp2+=3;
		while(isspace(*bp2)) bp2++;
	    }
	    r = strcasecmp(bp1, bp2);
	    free(cp1);
	    free(cp2);
	    if (r != 0)
		return r;
	    break;
	case ST_FROM:
	    bp1 = cp1 = shtext("from", a->text);
	    bp2 = cp2 = shtext("from", b->text);
	    r = strcasecmp(bp1, bp2);
	    free(cp1);
	    free(cp2);
	    if (r != 0)
		return r;
	    break;
	}
    }
    return(0);
}

cmd_sort(n)
{
    int i;
    int sortflds=0;
    static fdb conffdb = { _CMCFM, CM_SDH, NULL, NULL, 
			       "confirm to sort", 
			       NULL, NULL};
    
    static keywrd sortkw[] = {
	{ "date", 0, (keyval) ST_DATE },
	{ "subject", 0, (keyval) ST_SUBJ },
	{ "sender", 0, (keyval) ST_FROM },
	{ "size", 0, (keyval) ST_SIZE },
    };

    static keytab sorttab = { sizeof (sortkw) / sizeof (keywrd),
			      sortkw };

    static fdb sortfdb = { _CMKEY, 0, NULL, (pdat)&sorttab,
			       "Field to sort by, ", NULL,NULL };

    noise("by");
    sortflds = 0;
    sortkeys[sortflds] = ST_END;
    do {
	if (sortflds == 0)
	    sortfdb._cmdef = "date";
	else {
	    noise("and");
	    sortfdb._cmdef = NULL;
	}	    
	if (sortflds < MAXSORTKEYS - 1) 
	    parse(fdbchn(&sortfdb, &conffdb, NULL), &pv, &used);
	else
	    parse(&conffdb, &pv, &used);
	if (used == &sortfdb)
	    sortkeys[sortflds++] = (int) pv._pvkey;
    } while (used != &conffdb);
    if (sortflds == 0)
	sortkeys[sortflds++] = (int) ST_DATE;
    sortkeys[sortflds++] = (int) ST_END;

    if (!check_cf(O_RDWR))		/* precheck for file */
	return;
    if (!check_cf(O_WRONLY))		/* check for writeable file */
        return;
    qsort(&cf->msgs[1], cf->count, sizeof(message), msgcmp);
    clear_sequence(CS);
    clear_sequence(RS);
    clear_sequence(PS);
    cf->flags |= MF_DIRTY;
    for(i = 0; i < cf->count; i++)
	cf->msgs->flags |= M_MODIFIED;
}

cmd_alias (n)
int n;
{
    confirm();
    printhelp ("alias", HELP_TOP);
}


cmd_finger(n)
int n;
{
    extern string finger_command;
    static fdb cmdfdb = { _CMTXT };
    char cmd[BUFSIZ];

    parse (&cmdfdb, &pv, &used);	/* line of text */
    if (strlen(finger_command) == 0)
	cmerr ("no finger command defined - use SET FINGER-COMMAND to define one");
    strcpy(cmd, finger_command);
    strcat(cmd, " ");
    strcat(cmd, pv._pvstr);
    shell(cmd);
}


/*
 * check the messages in cf, and see if they look ok...
 * something is trashing fuat's mail file.  maybe we can spot it.
 */


void
debug_validate_msgvec(str)
char *str;
{
    int i;
    char *cp, *hfind();
    int uhoh = 0;
    extern int memdebug;

    if (cf == NULL || cf->msgs == NULL)
	return;
    if (memdebug) {
	for (i = 1; i <= cf->count; i++) {
	    cp = hfind("from", cf->msgs[i].text);
	    if (cp == NULL) {
		if (uhoh == 0)
		    fprintf(stderr,"%s\n",str);
		fprintf(stderr,
			"***** Message %d no longer has a from field!!!\n",i);
		uhoh++;
	    }
	}
	if (uhoh) {
	    maybe_abort("Some corrupted messages");
	}
#ifdef MDEBUG
	m_checkranges();
#endif /* MDEBUG */
    }
}
