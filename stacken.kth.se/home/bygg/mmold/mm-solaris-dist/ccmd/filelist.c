/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Howie Kaye
*/
/*
 * Given a vector of directories, build a table of all files in those
 * directories
 * If the path passed here is the same as the previous one (help, and the 
 * completion, uses previous listing for speed.
 *
 * All buffers are either malloc'ed or realloc'ed when needed.  If 
 * these fail, bad things can happen -- i'm not yet sure what to do in this 
 * case.
 *
 */

#include "ccmdlib.h"
#include "filelist.h"

#ifdef NODIRLIB
typedef struct {
    int size;
    struct direct *files;
    struct direct *current;
} DIR;
#endif

#if defined(ns32000) && defined(BSD)
#undef rewinddir
#define rewinddir(dirp) seekdir((dirp), (long)0)
DIR *OPENDIR(fn) { DIR *d = opendir(fn); d->dd_hashed_dir = 0; return(d); }
#else
#define OPENDIR opendir
DIR *opendir();
#endif



#ifdef DIRENTLIB
#define direct dirent			/* XXX kludge */
#endif

#if unix
#ifndef _CMUSR
#include <pwd.h>			/* for "~" completion */
#if SYSV
struct passwd *getpwent(), *getpwnam(), *getpwuid();
#endif /* SYSV */
#endif /*  !_CMUSR */
#endif /*  unix */

/* make sure toupper does what we want */
#undef toupper
#define toupper(c) (islower(c) ? (c)-'a'+'A' : (c))
#define BUFLEN 120

char *tilde_expand();

char *namebuf;				/* hold filenames here */
char *prevnamebuf;			/* previous name buffer */

dirfile *files;				/* list of files */
dirfile *prevfiles;			/* list from previous pass */

dirfile **fileindex;			/* vector of files for sorting */
dirfile **previndex;			/* index from previous pass */

int filecount;				/* number of files entries used */
int prevfilecount;			/* from previous pass */
int maxfiles;				/* number of file entries malloc'ed */

char *curfile;				/* current file in namelist */
int namelen;				/* used length of namebuf */
int maxnamelen;				/* malloc'ed length of namebuf */
char **prevpath=NULL;			/* previous search path  */
char **usepath=NULL;			/* expanded path to use */
int currentpath= 0;			/* current spot in usepath */
int usepathlen=0;
int redofiles = TRUE;
int *modtimes = NULL;
int suffix_modtime = 0;
char *oldsuffix = NULL;

char *malloc(), *realloc(), *index();
void qsort();				/* quick sort routine */

int dirfilecmp();			/* predicate for qsort */

/*
 * chng_suffix:
 * determine whether or not suffix has changed.  if it has, need
 * to reread the directory.
 * if suffix and oldsuffix are not equal, or they are equal but the modtimes
 * are different, return TRUE, else return FALSE
 */

chng_suffix (suffix, oldsuffix)
char *suffix, *oldsuffix;
{
  struct stat sbuf;

  if (strcmp (suffix, oldsuffix) != 0)
    return (TRUE);
  if (stat (suffix, &sbuf) != 0)	/* don't know, be safe */
    return (TRUE);
  if (suffix_modtime == sbuf.st_mtime)
    return (FALSE);
  suffix_modtime = sbuf.st_mtime;
  return (TRUE);
}


/*
 * cmfileini:
 * (re)initialize file vars.  Useful in making new file appear for
 * file parse.
 */
cmfileini()
{
    redofiles = TRUE;
}

/*
 * search_path:
 * takes a NULL terminated vector of directories.
 * returns a pointer to a listing of the files.
 * this listing is guaranteed to stay intact until the function is
 * called again.
 * fcount is set to the number of files returned
 */

dirfile **
search_path(path,suffix,fcount) char **path; char *suffix; int *fcount; {
  int pathlen;				/* length of the path */
  char **p;
  int i;
  register char **p1, **p2;		/* as last time */
  struct stat sbuf;
  int *m,goodlen = 0;

  p1 = path;
  p2 = prevpath;
  m = modtimes;

  if ((suffix == NULL && oldsuffix == NULL) ||
      (suffix != NULL && oldsuffix != NULL 
       && !chng_suffix (suffix, oldsuffix)))
      {
      for (;!redofiles;) {		/* check if it is same path */
	  if (p1 == NULL || p2 == NULL) break; /* NULL list? */
	  if (*p1 == NULL && *p2 != NULL) break; /* one null? */
	  if (*p1 != NULL && *p2 == NULL) break;
	  if ((*p1 != NULL) && (*p2 != NULL))
	      if (strcmp(*p1,*p2))
		  break;		/* or different entry...rebuild list */
	  if (*p1 == NULL) {		/* end of list?  they are the same */
					/* same list as last time...so */
	      namebuf = prevnamebuf;	/* make everything point to last set*/
	      *fcount = filecount = prevfilecount;
	      files = prevfiles;
	      fileindex = previndex;
	      return(previndex);	/* and return prev index */
	  }
	  if (stat(*p1,&sbuf) != 0)	/* if a directory changed, rebuild */
	      break;
	  if (sbuf.st_mtime != *m) break;
	  p1++,p2++;				/* keep walking the list */
      }
  }
  redofiles = FALSE;
					/* different path...free up old */
					/* buffers */

  if (prevnamebuf != NULL)		/* free old name buffer */
    free(prevnamebuf);
  if (prevfiles != NULL)		/* and old file list */
    free(prevfiles);
  if (previndex != NULL)		/* old index */
    free(previndex);
  if (prevpath != NULL) {		/* and old path */
    for (p = prevpath; *p != NULL; p++)	/* have to free each name in path */
      free(*p);
    free(prevpath);
  }
  if (modtimes != NULL)
      free(modtimes);
  filecount = prevfilecount = 0;	/* no previous files */
  fileindex = NULL;			/* or current files */
  files = NULL;
  namebuf = NULL;
  curfile = NULL;
  namelen = 0;
  maxnamelen = 0;
  maxfiles = 0;
					/* now copy the current path */
  pathlen = 0;
  for (p = path; *p != NULL; p++)	/* first find it's length */
    pathlen++;
  prevpath = (char **) malloc((pathlen+1)*sizeof(char *)); /* build vector */
  modtimes = (int *) malloc((pathlen)*sizeof(int));
  for (i = 0,p = path; *p != NULL; i++,p++) { /* then copy the names */
    prevpath[i] = malloc((1+strlen(*p)) * sizeof(char));
    strcpy(prevpath[i],*p);
    if (stat(*p,&sbuf) == 0)
	modtimes[i] = sbuf.st_mtime;
    else modtimes[i] = -1;
  }
  prevpath[pathlen] = NULL;		/* NULL termination */

  if (usepath != NULL) {		/* and expanded path */
    for (i = 0,p = usepath; i < usepathlen;p++,i++) {
      if (*p != NULL) free(*p);
    }
    free(usepath);
  }
  usepath = (char **) malloc((pathlen+1)*sizeof(char *)); /* build vector */
  usepathlen=pathlen;			/* and remember length */

  goodlen = 0;
  for (i = 0; i < usepathlen; i++) {
    int xlen = 0;

    if (suffix) 
	xlen = strlen(suffix);
					/* if it starts with a /, use */
    if (suffix && (suffix[0] == '/' || strlen(suffix) == 0 ||
		   suffix[0] == '~' || (strcmp(suffix,".") == 0) ||
		   (strcmp(suffix,"..") == 0))) {	
      int j;				/* as an absolute path */
      char *term = "";
      if (suffix[strlen(suffix)-1] != '/')
	term = "/";
      usepath[i] = malloc(xlen + 2);
      sprintf(usepath[i],"%s%s",suffix,"/");
      goodlen = 1;
      for (j = 1; j < usepathlen; j++)
	usepath[j] = NULL;
      break;
    }
    else {
      usepath[i] = malloc((1+xlen+sizeof("/")+
			   strlen(prevpath[i]))*sizeof(char));
      strcpy(usepath[i],prevpath[i]);
      if (suffix) {
	if (usepath[i][strlen(usepath[i])-1] != '/')
	    strcat(usepath[i], "/");
	strcat(usepath[i],suffix);
      }
      if (stat(usepath[i],&sbuf) == 0)
	  goodlen++;
    }
  }
  usepath[usepathlen] = NULL;
  for (i =0; i < usepathlen; i++) {	/* now build up list */
    currentpath = i;
    if (usepath[i] != NULL && modtimes[i] != -1) 
	search_dir(usepath[i],goodlen <= 1);
  }
					/* build up index into list */
  if (oldsuffix)
      free(oldsuffix);
  if (suffix) {
    struct stat sbuf;

    oldsuffix = malloc(strlen(suffix)+1);
    strcpy(oldsuffix,suffix);
    if (stat(suffix,&sbuf) == 0)
      suffix_modtime = sbuf.st_mtime;
  }
  else oldsuffix = NULL;
  fileindex = (dirfile **) malloc(sizeof(dirfile *) * filecount);
  for (i = 0; i < filecount; i++) fileindex[i] = &files[i];
					/* and sort it */
  qsort(fileindex,filecount,sizeof(dirfile *), dirfilecmp);
 
  previndex = fileindex;			/* save these for next time */
  prevnamebuf = namebuf;
  prevfiles = files;
  *fcount = prevfilecount = filecount;
  return(previndex);
}

/*
 * search_dir:
 * builds up a file list for a single directory
 * if a directory is wild, expands it.
 * if directory is CWD, then includes ".", and ".."
 */

search_dir(dirname,showdot) char *dirname; int showdot; {
  struct stat sbuf;
  DIR *dirp, *dotdirp;			/* directory pointers */
  register struct direct *d;		/* file entries in a dir */
  struct direct *dotd, *readdir();
  int i;
  char *dirr=dirname;			/* actual directory name to use */
  struct passwd *user;			/* passwd structure for "~" files */
  int ispwd;				/* flag if looking at connect dir */
  register dirfile *f;
  int dirstrlen;
#ifdef MSDOS
  static char pwdbuf[51];
  char *getcwd();
  int ustrcmp();
#endif
  if (iswild(dirname)) {
    expandwilddir(dirname);    
    return;
  }
#if unix
  if (dirname[0] == '~') {		/* "~user"? */ 
    dirr = tilde_expand(dirname);	/* expand it */
  }
#endif
  dirstrlen = strlen(dirr);
  ispwd = !strcmp(dirr,".");		/* if looking at "." then pwd */

					/* otherwise, we have to check */
  if (stat(dirr,&sbuf) != 0)
      return;
  if (!((sbuf.st_mode & S_IFMT) == S_IFDIR)) {
      return;
  }
  if (!ispwd) {
#ifdef MSDOS
    if (index(STRUCTTERM,dirr[strlen(dirr)-1])) { /* on msdos, */
      char buf[70];			/* check if the directory is */
      int unit;				/* the same as the connected */
      union REGS sregs, oregs;		/* by doing a DOS call to get the */
      struct SREGS segregs;		/* connected dir */
      int i;
      
      segread(&segregs);		/* set segment registers */
      unit = 0;				/* use default structure */
      if (isalpha(*dirr))
	unit = toupper(*dirr) - 'A' + 1; /* no use the specified struct */

      strcpy(buf,dirr);			/* copy the directory name */
      strcat(buf,"/");			/* add a "/" */
      sregs.h.ah = 0x47;		/* DOS int get cwd */
      sregs.x.si = (int) &buf[strlen(buf)];
      sregs.h.dl = unit;
      intdosx(&sregs, &oregs, & segregs); /* do the interrupt */
      for (i = 0; i < strlen(buf); i++)	/* and lowercase the string */
	if (isupper(buf[i])) buf[i] = tolower(buf[i]);
      dirp = OPENDIR(buf);		/* open up directory */
    }
    else
      dirp = OPENDIR(dirr);		/* open directory we are looking at */
#else
    dirp = OPENDIR(dirr);		/* open directory we are looking at */
#endif
    if (dirp == NULL) {
	perror(dirr);
	return;				/* can't...just return */
    }
    if (defstruct(dirr)) {		/* same structure? */
      dotdirp = OPENDIR(".");		/* open up "." */
      if (dotdirp != NULL) {
	d = readdir(dirp);		/* read dir entries */
	dotd = readdir(dotdirp);
	if (d != NULL && dotd != NULL)
#if unix
	  ispwd = (d->d_ino == dotd->d_ino); /* are they the same? */
#endif
#ifdef MSDOS
          ispwd = (!ustrcmp(d->d_name,getcwd(pwdbuf,50)));
#endif
	closedir(dotdirp);
        rewinddir(dirp);
      }
      else {
	perror(".");
      }
    }					/* if different struct, different */
  }
  else {
    dirp = OPENDIR(dirr);		/* open up directory */
    if (dirp == NULL) { 
      perror(dirr);
      return;				/* couldn't...go home */
    }
  }
					/* directory is now open */
  if (!ispwd && !showdot) {		/* if not CWD, skip ".", ".." */
      do d = readdir (dirp);
      while (d && d->d_name[0] == '.' &&
	     (d->d_name[1] == 0 ||
	      (d->d_name[1] == '.' && d->d_name[2] == 0)));
  }
  else {
      d = readdir(dirp);
  }
  if (d == NULL) {
      perror("readdir");
  }
  for (; d != NULL; d = readdir(dirp)) {
    int fnlength;
					/* do we need more files? */

    if (filecount == maxfiles) {
      maxfiles += FILEINCR;
      files =(dirfile *)cmrealloc(files, sizeof(dirfile)*(maxfiles));
    }
					/* do we need more name space  */
#if MSDOS || defined(NDIRLIB) || defined(DIRENTLIB)
    fnlength = strlen(d->d_name);
#else /* MSDOS */
#if BSD
    fnlength = d->d_namlen;
#else
    fnlength = strlen(d->d_name);
    if (fnlength > 14)
	fnlength = 14;
#endif /* !BSD */
#endif /* !MSDOS */
    if (maxnamelen - (curfile - namebuf) < (fnlength + 1)) {
      int offset = curfile - namebuf;
      maxnamelen += NAMEINCR;
      namebuf = cmrealloc(namebuf,sizeof(char)*maxnamelen);
      curfile = namebuf + offset;
    }
    f = &files[filecount];		/* set up pointer to current file */
    f->offset = curfile-namebuf;	/* it's offset into namebuf */

    strncpy(curfile, d->d_name, fnlength);
    curfile[fnlength] = 0;
    curfile += fnlength + 1;		/* bump current pointer by namelen */

    f->directory = dirname;		/* point to it's directory */
    f->flags = 0;			/* no flags */
    filecount++;
  }
  closedir(dirp);
}
  

/*
 * dirfilecmp:
 * predicate to compare two dirfile struct's for qsort
 */
dirfilecmp(a,b) register dirfile **a,**b; {
  return(strcmp(&namebuf[(*a)->offset],&namebuf[(*b)->offset]));
}


/*
 * ustrcmp:
 * case insensitive string comparison
 */

ustrcmp(s1,s2) char *s1, *s2; {
  register char c1, c2;
  for (;;s1++,s2++) {
    c1 = toupper(*s1);
    c2 = toupper(*s2);
    if (c1 < c2) return(-1);
    if (c1 > c2) return(1);
    if (c1 == '\0') return(0);
  }
}

/* 
 * expand the current directory, and place expansion in 
 * usepath vector after current loc.
 * expand usepath as we go.
 */

expandwilddir(dirname) char *dirname; {
  DIR *dirp;
  struct direct *d;
  static char basename[BUFLEN],fname[BUFLEN],rest[BUFLEN];
  char c;
  static char temp[BUFLEN];
  
  int i,base;
  struct stat s;
  
  base = -1;				/* find the non wild base */
  for (i = 0; i < strlen(dirname); i++) {
    c=dirname[i];
    if (index(DIRSEP,c)) {
      base = i;
      continue;
    }
    if (index(WILDCHARS,c)) break;
  }
  if (base == -1) {			/* no base dir.   use dot */
    strcpy(basename,".");
  }
  else {				/* a base dir.  use it */
    strncpy(temp,dirname,base);
    temp[base] = '\0';
    strcpy(basename,tilde_expand(temp));
  }

  if (strlen(basename) == 0) strcpy(basename,"/");

  fname[0] = '\0';			/* the current wild part */
  for (i = base+1; i < strlen(dirname); i++) {
    if (index(DIRSEP,dirname[i])) break;
    fname[i-(base+1)] = dirname[i];
  }
  fname[i-(base+1)] = '\0';

  i++;					/* get past dirsep */
  if (i < strlen(dirname))
    strcpy(rest,&dirname[i]);		/* and the rest of the string */
  else rest[0] =  '\0';
  if (rest[strlen(rest)-1] == '/') rest[strlen(rest)-1] = '\0';

  if (strcmp(basename,"/") && strcmp(basename,".") && strcmp(basename,"..")) {
    					/* we know these are dirs */
    if (stat(basename,&s) == -1) {	/* and root breaks the msdos stat */
      fprintf(stderr,"stat(%s) failed\n",basename); /* function */
      return;
    }
    if (!((s.st_mode & S_IFMT) == S_IFDIR)) {
      fprintf(stderr,"%s not a dir\n",basename);
      return;
    }
  }
  dirp = OPENDIR(basename);
  if (dirp == NULL) return;
					/* insert directories into path */
  d = readdir(dirp);			/* kill off . and .. */
  if (!strcmp(d->d_name,"."))
    readdir(dirp);
  else rewinddir(dirp);
  for(d = readdir(dirp); d!= NULL; d = readdir(dirp)) {
    if ((d->d_name[0] != '.') || (d->d_name[0] == '.' && fname[0] == '.')) {
      if (fmatch(d->d_name,fname)) {	/* if we match the wildcard */
	static char buf[BUFLEN];
	if (!strcmp(basename,".")) 
	  basename[0] = '\0';
	else {
	  strcpy(buf,basename);
	}
	if (strlen(buf) > 0)
	  if (strcmp(buf,"/")) strcat(buf,"/");
	strcat(buf,d->d_name);
	if (strcmp(buf,".") && strcmp(buf,"..")) {
	  if (stat(buf,&s) == -1) continue;
	  if (!((s.st_mode & S_IFMT) == S_IFDIR)) continue;
	}
	if (strlen(rest) > 0) {
	  strcat(buf,"/");
	  strcat(buf,rest);
	}
	addtopath(buf);			/* add to the path vector */
      }
    }
  }
  closedir(dirp);			/* all done */
}

/*
 * actually add directories to the path in use.
 */
addtopath(dirname) char *dirname; {
  int i;

					/* grow the path */
  usepath =(char**) cmrealloc(usepath,(usepathlen+2)*sizeof(char *));
					/* move everything else down one */
  for (i = usepathlen; i > currentpath; i--) {
    usepath[i+1] = usepath[i];
  }
  usepathlen++;				/* make space for this string */
  usepath[currentpath+1] = malloc(strlen(dirname)+1);
  strcpy(usepath[currentpath+1],dirname); /* and copy it in */
}

#ifdef notdef
/*
 * index function
 * returns index of char c in string s
 */
char *
index(sp,c)				/* get index into a string */
register char *sp, c;
{
  for (; *sp && *sp != c; sp++);
  if (*sp == '\0') return(NULL);
  else return(sp);
}
#endif

/*
 * routine to check if a string is wild
 * match all chars in string against all wild chars
 */
 
iswild(str) char *str; {
  int hit[128],i;
  int len1, len2;
  for (i = 0; i < 128; i++) hit[i] = 0;	/* no hits yet */
  len1 = strlen(str);
  for(i = 0; i < len1; i++)		/* mark hits */
    hit[str[i]] = 1;
  len2 = strlen(WILDCHARS);
  for (i = 0; i < len2; i++)		/* and check for them */
    if (hit[WILDCHARS[i]]) return(TRUE);
  return(FALSE);
}

/*
 * check if directory is on connected structure
 */
defstruct(dirname)
char *dirname;
{
#ifdef MSDOS
  int i;
  char buf[100];
  
  getcwd(buf,100);			/* get connected dir */
  for(i = 0; i < strlen(dirname); i++)
    if (index(STRUCTTERM,dirname[i]))	/* a structure delimiter? */
      return(toupper(dirname[i-1]) == toupper(buf[0]));
#endif
  return(TRUE);
}


#if unix
/* 
 * WHOAMI:
 * 1) Get real uid
 * 2) See if the $USER environment variable is set
 * 3) If $USER's uid is the same as realuid, realname is $USER
 * 4) Otherwise get logged in user's name
 * 5) If that name has the same uid as the real uid realname is loginname
 * 6) Otherwise, get a name for realuid from /etc/passwd
 */

PASSEDSTATIC
char *
whoami () {
  static char realname[256];		/* user's name */
  static int realuid = -1;		/* user's real uid */
  char loginname[256], envname[256];	/* temp storage */
  char *getlogin(), *getenv(), *c;
  struct passwd *p, *getpwnam(), *getpwuid(), *getpwent();

  if (realuid != -1)
    return(realname);

  realuid = getuid ();			/* get our uid */

  /* how about $USER? */
  if ((c = getenv("USER")) != NULL) {	/* check the env variable */
    strcpy (envname, c);		
    p = getpwnam(envname);
    if (p->pw_uid == realuid) { /* get passwd entry */
					/* for envname */
      strcpy (realname, envname);	/* if the uid's are the same */
      return(realname);
    }
  }

  /* can we use loginname() ? */
  if ((c =  getlogin()) != NULL) {	/* name from utmp file */
    strcpy (loginname, c);	
    if ((p = getpwnam(loginname)) != NULL) /* get passwd entry */
      if (p->pw_uid == realuid) {	/* for loginname */ 
	strcpy (realname, loginname);	/* if the uid's are the same */
	return(realname);
      }
  }

  /* Use first name we get for realuid */
  if ((p = getpwuid(realuid)) == NULL) { /* name for uid */
    realname[0] = '\0';			/* no user name */
    realuid = -1;
    return(NULL);
  }
  strcpy (realname, p->pw_name);	
  return(realname);
}
#endif

/*
 * expand ~user to the user's home directory.
 */
char *
tilde_expand(dirname) char *dirname; {
#ifdef MSDOS
  return(dirname);			/* no users in msdos */
#endif /*  MSDOS */
#if unix
  struct passwd *user, *getpwuid(), *getpwnam();
  static char olddir[BUFLEN];
  static char oldrealdir[BUFLEN];
  static char temp[BUFLEN];
  int i;
  char *whoami();

  if (dirname[0] != '~') return(dirname); /* not a tilde...return param */
  if (!strcmp(olddir,dirname)) return(oldrealdir); /* same as last time. */
					/* so return old answer */
  else {
    for (i = 0; i < strlen(dirname); i++) /* find username part of string */
      if (!index(DIRSEP,dirname[i]))
	temp[i] = dirname[i];
      else break;
    temp[i] = '\0';			/* tie off with a NULL */
    if (strlen(temp) == 1) {		/* if just a "~" */
      user = getpwnam(whoami());	/*  get info on current user */
    }
    else {
      user = getpwnam(&temp[1]);	/* otherwise on the specified user */
    }
  }
  if (user != NULL) {			/* valid user? */
    strcpy(olddir, dirname);		/* remember the directory */
    strcpy(oldrealdir,user->pw_dir);	/* and their home directory */
    strcat(oldrealdir,&dirname[i]);
    return(oldrealdir);
  }
  else {				/* invalid? */
    strcpy(olddir, dirname);		/* remember for next time */
    strcpy(oldrealdir, dirname);
    return(oldrealdir);
  }
#endif
}

#ifdef notdef
bcopy(src,dest,len)
register char *src,*dest;
register int len;
{
  if (src > dest)               /* Could source be overwritten? */
    for(; len > 0; len--)       /* No */
      *dest++ = *src++;         /* Simple forward copy */
  else
    for(; len >= 0; len--)       /* Yes */
      *(dest+len) = *(src+len); /* Backwards copy */
}

#endif

#ifdef NODIRLIB

#ifdef opendir
#undef opendir
#endif
DIR *
opendir(name) {
    DIR *dir;
    int fd,open();
    struct stat sbuf;
    int x;

    if (stat(name,&sbuf) != 0) return(NULL);
    if (!(sbuf.st_mode & S_IFDIR)) {
	errno = ENOTDIR;
	return(NULL);
    }
    fd = open(name,O_RDONLY,0);
    if (fd == -1)
	perror("open");
    if (fd == -1) return(NULL);
    if ((dir = (DIR *)malloc(sizeof(DIR))) == NULL) {
	close(fd);
	return(NULL);
    }
    dir->size = sbuf.st_size;
    if ((dir->files = (struct direct *) malloc(dir->size)) == 0) {
	close(fd);
	free(dir);
	return(NULL);
    }
    x = read(fd,dir->files,dir->size);
    if (x == -1) perror("read");
    dir->current = dir->files;
    close(fd);
    return(dir);
}

struct direct *
readdir(dirp) DIR *dirp; {
    int s = dirp->size / sizeof(struct direct);
    struct direct *temp;
    if (dirp->current >= dirp->files + s)
	return(NULL);
    while (dirp->current->d_ino == 0)
	if (dirp->current >= dirp->files + s)
	    return(NULL);
	else dirp->current++;
	    if (dirp->current >= dirp->files + s)
	return(NULL);
    temp = dirp->current++;
    return(temp);
}
    
rewinddir(dirp) DIR *dirp; {
    dirp->current = dirp->files;
}

closedir(dirp) DIR *dirp;
{
    free(dirp->files);
    free(dirp);
}
#endif
