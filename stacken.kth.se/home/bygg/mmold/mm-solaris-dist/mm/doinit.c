/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/doinit.c,v 2.3 90/10/10 14:58:03 melissa Exp $";
#endif

#include "mm.h"
#include "set.h"
#include "parse.h"

cmd_create_init(n)
int n;
{
    confirm();
    write_init();
}

cmd_profile(n) 
int n;
{
    confirm();
    ask_profile_questions();
    write_init();
}

ask_profile_questions() {
    int ask;
    reply_include_me =
	yesno("Do you want to receive copies of your replies to messages? ",
	      "no");
    printf("By default, the REPLY and ANSWER commands send replies only to\n\
to the address in the \"From:\" header of the original message.\n\
However, you can specify that replies should, by default, be sent to\n\
everyone listed in the \"To:\" and \"cc:\" headers as well.\n");
    reply_all = yesno ("Do you want replies to go to everyone? ",
		       reply_all ? "yes" : "no");
    clear_screen = yesno(
"Do you want to have the screen cleared at startup and between messages? ",
			 clear_screen ? "yes" : "no");
    printf("Normally, typing Control-N causes MM to stop what it's \
doing and ask\n\
if the current command should be aborted.\n");
    ask = yesno("Do you want Control-N to abort without asking? ",
		control_n_abort == SET_ASK ? "no" : "yes");
    if (ask)
	control_n_abort = SET_ALWAYS;	/* always abort */
    else
	control_n_abort = SET_ASK;	/* ask first */

    printf("Other profile options may be set by using the SET command to set\n\
the option, and SAVE-INIT to update your \".mminit\" file.  You may\n\
also edit .mminit with an editor.  Use the \"HELP SET variable-name\"\n\
command for descriptions of individual .mminit options, and the SHOW\n\
command to list the complete environment.\n");
}

write_init() {
    FILE *fp;
    int i;
    struct stat sbuf;
    int gotstat;
    char filename[BUFSIZ];
    char ofile[BUFSIZ];
    extern variable set_variables[];
    extern int set_variable_count;
    extern int write_fast_init_file();

    if (HOME == NULL)
	return false;
    sprintf (filename, "%s/.mminit", HOME);
    if (stat(filename,&sbuf) == 0) {
	sprintf(ofile,"%s~", filename);
	rename(filename,ofile);
	gotstat = TRUE;
    }
    else
	gotstat = FALSE;
    fp = fopen(filename, "w");
    for (i = 0; i < set_variable_count; i++) {
	if (!set_variables[i].changed)
	    continue;
	if (i != SET_MAIL_ALIASES)
	    show_variable (fp, i, true);
    }
    for (i = 0; i < mail_aliases.count; i++)
	disp_alias (fp, i, true,false);
    if (gotstat) {			/* old file, keep protection */
	if (chmod (filename, sbuf.st_mode & 07777) != 0)
	    perror ("Trouble fixing file mode of new init file");
    }	
    fclose(fp);

    if ((fast_init_file == SET_YES) || /* Only write it if asked to! */
	((fast_init_file == SET_ASK) &&
	 (yesno ("Write fast init file too? ", "yes"))))
      return write_fast_init_file();
    else
      return true;		/* Otherwise do nothing */
}

var_not_empty(n) 
{
    extern variable set_variables[];
    variable *v = &set_variables[n];
    switch(v->type) {
    case VAR_TEXT:
    case VAR_FILE:
    case VAR_OFILE:
    case VAR_COMMAND:
    case VAR_QUOTEDSTR:
    case VAR_USERNAME:
	if (strlen(v->addr) == 0) return(false);
	break;
    case VAR_KEYLIST:
    case VAR_FILES:
	if (v->addr == NULL) return(false);
	if (*v->addr == NULL) return(false);
	break;
    case VAR_ADDRLIST:
	if (v->addr == NULL) return(false);
	if (((addresslist *)v->addr)->first == nil) return(false);
    case VAR_KEYWORDS:
	if (v->addr == NULL || ((setkey *)v->addr)->current == NULL)
	    return(false);
	break;
    }
    return(true);
}
