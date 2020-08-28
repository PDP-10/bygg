/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/dt.c,v 2.1 90/10/04 18:24:01 melissa Exp $";
#endif

#include "mm.h"
#include "ccmd.h"

#ifdef howmany
#undef howmany
#endif

static char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
			      "Aug", "Sep", "Oct", "Nov", "Dec" };
static int monthlens[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

struct tmz {
    char *name;
    char *dstname;
    int minwest;
} zones[] = {
    { "GMT", "GMT",       0*60 },
    { "AST", "ADT",       4*60 },
    { "EST", "EDT",       5*60 },
    { "CST", "CDT",       6*60 },
    { "MST", "MDT",       7*60 },
    { "PST", "PDT",       8*60 },
    { "YST", "YDT",       9*60 },
    { "HST", "HDT",      10*60 },
    { "BST", "BDT",      11*60 },
    { "WET", "WET DST",   0*60 },
    { "MET", "MET DST",  -1*60 },
    { "EET", "EET DST",  -2*60 },
    { "EST", "EST",     -10*60 },
    { "CST", "CST", -(9*60+30) },
    { "WST", "WST",      -8*60 },
};

lookupzone(z) {
    int i;
    for(i = 0; i < sizeof(zones)/sizeof(struct tmz); i++) {
	if (ustrcmp(z,zones[i].name) == 0)
	    return(zones[i].minwest);
	if (ustrcmp(z,zones[i].dstname) == 0)
	    return(zones[i].minwest-60);
    }
    return(-1);
}

mbox_date(str) 
char *str;
{
    int n;
    int t,i;
    char mon[4],dow[4];
    int day, month, year, hr, min, sec,isaleap;
    
    n = sscanf(str, "%3s %3s %2d %2d:%2d:%2d %4d\n",
	       dow,mon,&day,&hr,&min,&sec,&year);
    if (n != 7)
	return(stringtotime(str));
    if ((month = whatmonth(mon)) == -1)
	return(stringtotime(str));
    t = mkdate(year,month,day,hr,min,sec,NULL);
    return(t);
}

mtxt_date(str) 
char *str;
{
    int n;
    int t,i;
    char mon[4],tz[10];
    int day, month, year, hr, min, sec,isaleap;
    
    n = sscanf(str, "%2d-%3s-%d %2d:%2d:%d-%3s\n",&day,mon,&year,&hr,&min,
	       &sec,tz);
    if (n != 7)
	return(stringtotime(str));
    if ((month = whatmonth(mon)) == -1)
	return(stringtotime(str));
    if (year < 100) year += 1900;
    t = mkdate(year,month,day,hr,min,sec,tz);
    return(t);
}

babyl_date(str) 
char *str;
{
    int n;
    int t,i;
    char mon[4],tz[4],dow[4];
    int day, month, year, hr, min, sec,isaleap;
    int howmany = 8;

    n = sscanf(str, "%3s, %2d %3s %d %2d:%2d:%2d %3s\n",
	       dow,&day,mon,&year,&hr,&min,&sec,tz);
    if (n == 8) {
	if ((month = whatmonth(mon)) == -1)
	    return(stringtotime(str));
	if (year < 100) year += 1900;
	return(mkdate(year,month,day,hr,min,sec,tz));
    }
    n = sscanf(str, "%3s, %2d %3s %d, %2d:%2d:%2d %3s\n",
	       dow,&day,mon,&year,&hr,&min,&sec,tz);
    if (n == 8) {
	if ((month = whatmonth(mon)) == -1)
	    return(stringtotime(str));
	if (year < 100) year += 1900;
	return(mkdate(year,month,day,hr,min,sec,tz));
    }
    n = sscanf(str, "%3s %2d %3s %d %2d:%2d:%2d %3s\n",
	       dow,&day,mon,&year,&hr,&min,&sec,tz);
    if (n == 8)  {
	if ((month = whatmonth(mon)) == -1)
	    return(stringtotime(str));
	if (year < 100) year += 1900;
	return(mkdate(year,month,day,hr,min,sec,tz));
    }
    n = sscanf(str, "%3s, %2d %3s %d %2d:%2d %3s\n",
	       dow,&day,mon,&year,&hr,&min,tz);
    if (n == 7) {
	if ((month = whatmonth(mon)) == -1)
	    return(stringtotime(str));
	if (year < 100) year += 1900;
	return(mkdate(year,month,day,hr,min,0,tz));
    }
    n = sscanf(str, "%3s %2d %3s %d %2d:%2d %3s\n",
	       dow,&day,mon,&year,&hr,&min,tz);
    if (n == 7) {
	if ((month = whatmonth(mon)) == -1)
	    return(stringtotime(str));
	if (year < 100) year += 1900;
	return(mkdate(year,month,day,hr,min,0,tz));
    }
    return(stringtotime(str));
}

whatmonth(mon)
char *mon;
{
    int i;
    for(i = 0; i < 12; i++) {
	if (strcmp(months[i], mon) == 0) {
	    return(i);
	}
    }
    return(-1);
}

mkdate(yr,mon,day,hr,min,sec,z)
int yr,mon,day,hr,min,sec;
char *z;
{
    int t,i;
    int isaleap;
    datime dt;
    struct tm *tm;
#ifdef BSD
    struct timeval tp;
    struct timezone tz;
#endif
    int zone;

    if (z) 
	zone = lookupzone(z);
    else
	zone = -1;
#ifdef BSD
    gettimeofday(&tp, &tz);
#endif
    dt._dtsec = sec;
    dt._dtmin = min;
    dt._dthr = hr;
    dt._dtday = day-1;
    dt._dtmon = mon;
    dt._dtyr = yr;
    dt._dtdst = 0;
    dt._dttz = 0;
    t = datime_to_time(&dt);
    tm = localtime(&t);
#if SYSV
    asctime(tm);
    t += (zone != -1) ? zone*60 : timezone;
#endif
#if BSD
    t += (zone != -1) ? zone*60 : tz.tz_minuteswest*60;
#endif
    if ((zone == -1) && (tm->tm_isdst)) t -= 3600;
    return(t);
}

#ifdef undef
main() {
    mbox_date("Jan  1 00:00:00 1970");
    mtxt_date("01-Jan-70 00:00:00-GMT");
    babyl_date("Thu, 1 Jan 70 00:00:00 EST");
    mbox_date("Jun  1 00:00:00 1970");	/* some IN dst */
    mtxt_date("01-Jun-70 00:00:00-GMT");
    babyl_date("Thu, 1 Jun 70 00:00:00 EST");
}

#endif
