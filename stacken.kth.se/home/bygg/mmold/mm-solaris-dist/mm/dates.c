/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/dates.c,v 2.1 90/10/04 18:23:52 melissa Exp $";
#endif

/*
 * dates.c - miscellaneous routines to manipulate date/time formats
 */

#include "mm.h"
#if BSD
#include <sys/timeb.h>			/* XXX use gettimeofday */
#endif

static int dayspermonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

char *month_names[] = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};
static char *day_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
    "Saturday"
};

int minutes_west;			/* offset from gmt; set in init.c */

#define dysize(y) \
    ((((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0) ? 366 : 365)

time_t
itime(t)
struct tm *t;				/* as returned by localtime(3) */
{
    int i;
    unsigned long x = 0;

    int dst = (t->tm_isdst ? 1 : 0);	/* how much to subtract for dst */

    /* count days in previous years back to 1970 */
    for (i = (1900 + (t->tm_year) - 1); i >= 1970; i--)
	x += dysize(i);

    /* add in days in previous months for this year */
    for (i = t->tm_mon; i > 0; i--)
	x += dayspermonth[i-1];

    /* include Feb 29th if it has occurred this year */
    if ((dysize(t->tm_year) == 366) && (t->tm_mon >= 2))
	x++;

    x += t->tm_mday - 1;		/* add days this month */

    x = (24*x) + t->tm_hour - dst;	/* convert and add hours */
    x = (60*x) + t->tm_min + minutes_west; /* add minutes, convert to gmt */
    x = (60*x) + t->tm_sec;		/* seconds */
    return (time_t) x;
}

/*
 * 1 Jan 70
 */
char *
cdate(it)
time_t it;
{
    struct tm *t;
    static char str[16];

    t = localtime(&it);
    sprintf(str, "%d %3.3s %d", t->tm_mday, month_names[t->tm_mon],
	    (t->tm_year < 100) ? t->tm_year : t->tm_year + 1900);
    return (char *) str;
}

/*
 * ctad - convert internal time to ascii
 *
 * Similar to ctime, but the output format is "11 Oct 86 12:42am"
 * Returns current time if the argument is zero.
 */

char *
ctad(it)
time_t it;
{
    struct tm *t;
    static char str[32];

    if (it == 0)
	(void) time (&it);

    t = localtime(&it);
    sprintf(str,"%d %3.3s %d %d:%02d%s",
	    t->tm_mday, month_names[t->tm_mon],
	    (t->tm_year < 100) ? t->tm_year : t->tm_year + 1900,
	    (((t->tm_hour % 12) == 0) ? 12 : (t->tm_hour % 12)),
	    t->tm_min, ((t->tm_hour < 12) ? "am" : "pm"));
    return (char *) str;
}

/*
 * fdate - convert internal date/time to message separator format used
 * in mail files
 *
 * 01-Jan-70 00:00:00-GMT
 */

char *
fdate (it)
time_t it;
{
    struct tm *t;
    static char str[32];

    t = gmtime (&it);
    sprintf (str, "%2d-%3.3s-%d %2d:%02d:%02d-GMT",
		 t->tm_mday, month_names[t->tm_mon],
		 (t->tm_year < 100) ? t->tm_year : t->tm_year + 1900,
		 t->tm_hour, t->tm_min, t->tm_sec);
    return (char *) str;
}

/*
 * hdate - convert internal date to ascii
 *
 * return a date string for use in displaying headers in the form
 * nn-mmm
 */

char *
hdate (it)
time_t it;
{
    struct tm *t;
    static char str[32];
    extern int thisyear;

    t = localtime(&it);
    /* %2.0d works.  it should be %2.2d, but that comes out as "07" */
    sprintf(str,"%2.0d-%3.3s",
	    t->tm_mday, month_names[t->tm_mon]);
    return (char *) str;
}


/*
 * More verbose version of ctad; format is "Thursday, 31 July 1986 11:20PM"
 *
 * If the argument is zero, the current date is returned.
 */
char *
daytime(it)
time_t it;
{
    struct tm *t;
    static char str[64];

    if (it == 0)
	(void) time (&it);

    t = localtime(&it);
    sprintf(str,"%s, %d %s %d %d:%02d%s",
	    day_names[t->tm_wday],
	    t->tm_mday, month_names[t->tm_mon], t->tm_year + 1900,
	    (((t->tm_hour % 12) == 0) ? 12 : (t->tm_hour % 12)),
	    t->tm_min, ((t->tm_hour < 12) ? "AM" : "PM"));
    return (char *) str;
}


/*
 * rfc822-acceptable date, like:
 * Thu, 1 Jan 70 00:00:00 GMT
 */
char *
rfctime(it)
time_t it;
{
    struct tm *t,t1;
    static char str[64];
#if BSD
    struct timeb tb;
#endif
#if SYSV
    extern char *tzname[2];
#endif

    if (it == 0)
	(void) time (&it);

    t = localtime(&it);
#if BSD
    ftime (&tb);
#endif
#if SYSV
    asctime(t);
#endif    
    sprintf(str,"%3.3s, %d %3.3s %d %d:%02d:%02d %s",
	    day_names[t->tm_wday],
	    t->tm_mday, month_names[t->tm_mon], t->tm_year /*+ 1900*/,
	    t->tm_hour, t->tm_min, t->tm_sec,
#if BSD
	    timezone(tb.timezone, t->tm_isdst)
#endif
#if SYSV
	    tzname[t->tm_isdst ? 1 : 0]
#endif
	    );
    return (char *) str;
}
