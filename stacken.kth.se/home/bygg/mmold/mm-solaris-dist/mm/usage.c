/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/usage.c,v 2.1 90/10/04 18:26:58 melissa Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <pwd.h>
#include <grp.h>

static long u_begtime= -1, u_endtime= -1;
static time_t cpu_time = 0;
char *whoami(), *ctime();
static char *usertype(), *pbtime(), *petime(), *pxtime(), *grpnam();

usage_start() 
{
    u_begtime = time(0);
}

usage_stop(logfile)
{
    FILE *fp;
    struct tms buffer;
    time_t new_time;

    if (u_begtime == -1)
	return;
    fp = fopen(logfile, "a");
    if (fp == NULL)
	return;
    u_endtime = time(0);
    times(&buffer);			/* get cpu usage */
    new_time = buffer.tms_utime + buffer.tms_stime;
    fprintf(fp, "%-8s %-8s (pid %5d) %s -- %s (%s) [%0.1fs]\n", whoami(),
	    usertype(whoami()), getpid(),
	    pbtime(u_begtime), petime(u_endtime), pxtime(u_endtime-u_begtime),
	    (float) (new_time - cpu_time)/60);
    cpu_time = new_time;		/* save the most recent one */
    fclose(fp);
}

static char*
usertype(uname) {
    struct passwd *p;
    
    p = getpwnam(uname);
    if (p == NULL) return(grpnam(getgid()));
    return(grpnam(p->pw_gid));
}

static char *
grpnam(gid) {
    struct group *g;
    static char buf[20];
    g = getgrgid(gid);
    if (g != NULL)
	return(g->gr_name);
    sprintf(buf,"%d", gid);
    return(buf);
}

static char *
pbtime(t)
long t;
{
    static char buf[26];

    strcpy (buf,ctime(&t));
    buf[16] = '\0';
    return(buf);
}

static char *
petime(t)
long t;
{
    static char buf[26];

    strcpy (buf,ctime(&t));
    buf[16] = '\0';
    return(buf+11);
}

static char *
pxtime(t)
long t;
{
    static char buf[20];
    int hr, min, sec;
    hr = t/3600;
    min = (t % 3600)/60;
    sec = t % 60;
    sprintf(buf,"%d:%02d:%02d", hr, min, sec);
    return(buf);
}
