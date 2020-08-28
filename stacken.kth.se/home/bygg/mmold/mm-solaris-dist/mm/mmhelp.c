/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/mmhelp.c,v 2.1 90/10/04 18:25:02 melissa Exp $";
#endif

/*
 * mmhelp.c:
 */

#ifdef undef
        EXTERN .GENERAL,.HSET   ;External help routines
#endif /* undef */

/*
 * MM command help strings:
 * help_xxx are for the top-level 
 * read_help_xxx are for read-mode
 * send_help_xxx are for send-mode
 * set_help_xxx are set sub-help strings
 */

char *help_keywords =
"\n\
The KEYWORDS command takes two arguments, a keywords list and a message\n\
sequence.  It will then mark the messages in that sequence as being\n\
included in the keyword.  To define a keyword, put a line in your MM.INIT\n\
of the form:\n\
\tKEYWORDS list-of-keywords\n\
This feature is useful for classifying old messages.\n";

char *help_unkeywords =
"\n\
The UNKEYWORDS command takes two arguments, a keywords list and a message\n\
sequence.  It will then mark the messages in that sequence as not being\n\
included in the keyword.\n";

char *read_help_keywords =
"\n\
The KEYWORDS command takes a keywords list as an argument and marks\n\
the current message as being included in the keyword.\n";

char *read_help_unkeywords =
"\n\
The UNKEYWORDS command takes a keywords list as an argument and unmarks\n\
the current message so that it is no longer included in the keyword.\n";

char *help_logout =
"\n\
The LOGOUT command will stop MM, expunge your message file, and log you\n\
out from the system.\n";

char *help_answer =
"\n\
The ANSWER command takes 1 argument, a message sequence you would like to\n\
answer. So, to reply to message 3 you would say ANSWER 3 where 3 is the\n\
message number. Or you could answer any other message sequence. After\n\
typing ANSWER 3 and then carriage return it asks you \"Reply message \#3 to:\"\n\
and awaits one of \"ALL\" or \"SENDER\". If you respond with ALL, then your\n\
answer will go to everyone in the header of the message: the person who\n\
sent it to you and everyone else.  If you respond with SENDER, then your\n\
answer will only go to the sender of the message.  The REPLY command is a\n\
synonym for this command.\n";

char *help_append =
"\n\
The APPEND command takes a message sequence, and appends those messages\n\
together into one message.\n";


char *send_help_deliveryoptions =
"\n\
The DELIVERY-OPTIONS command takes one argument, a delivery option name.\n\
This decides whether to mail the message and/or send it to the recipient's\n\
terminal.\n";

char *send_help_after =
"\n\
The AFTER command takes one argument, a date/time parameter in any\n\
reasonable format, and requests the system mailer to suppress delivery of\n\
this message until after the specified time.\n";

char *send_help_userheader =
"\n\
The USER-HEADER command takes two arguments; a header keyword as\n\
defined in the USER-HEADERS line in the MM.INIT file, and a header\n\
text line, and inserts the line with that name in the message\n\
header.\n";

char *help_alias =
"\n\
The ALIAS command takes a single argument, a user name.  It then\n\
causes MM to behave as if you were that user; all mail sent will be\n\
\"from\" that user (your login name will be the \"sender\"), MM will\n\
read the aliased user's mail file, and MM will use the aliased user's\n\
MM.INIT.\n";

/*****/
char *help_enable =
"\n\
The ENABLE command enables your capabilities (if you have any) and attempts\n\
to make any \"read-only\" file be read-write.\n";

/*****/
char *help_disable =
"\n\
The DISABLE command disables your capabilities (if you had any) and makes\n\
the current file read-only.\n";

/*****/
#define read_help_netmail help_netmail
char *help_netmail =
"\n\
The NET-MAIL command will attempt to send any messages that may be queued\n\
in your directory.\n";

char *read_help_spell =
"\n\
The SPELL command invokes the SPELL program on the whole message.\n\
See the SPELL program's documentation for how to use it.\n";

char *send_help_spell =
"\n\
The SPELL command invokes the SPELL program in the TEXT field of the message.\n\
See the SPELL program's documentation for how to use it.\n";


/*****/
char *help_dired =
"\n\
The DIRED command takes a list of message sequences, and starts the\n\
DIRED subsystem of the MMAIL package to maintain your message file\n\
ala disk DIRED (the message headers are your mail file's \"directory\").\n\
\n\
To use DIRED, your editor must be EMACS and you must load the MMAIL\n\
library.  The default EMACS.INIT will do this for you.\n";


#ifdef undef
;;; MM SET command help strings

;Structure of INIVTB is STR-ADDR,,[[INIDTA,,HLPMSG],,VAR-ADDR]
; produced by VARH string,var-addr,hlpmsg,inidta

DEFINE VARH (STR,VAR,HLP,DTA) <[ASCIZ/STR/],,[[DTA,,HLP],,VAR##]>

	EXTERN INIBB,INIDEC,INIKEY,INILNS,INIUNM

INIVTB::NINVRS,,NINVRS		;User variables
	VARH BBOARD-BEHAVIOR-ON-RESCAN,RSCFLG,H.BRS
	VARH BBOARD-FILES,BBTAB,H.BB,INIBB
	VARH BLANK-SCREEN-STARTUP,BLSCST,H.BLNK
	VARH CONTROL-E-EDITOR,EDTFLG,H.CTLE
	VARH CONTROL-N-ABORT,ABOFLG,H.CTLN
	VARH DEFAULT-BBOARD,DEFBBD,H.DBBD,-47
	VARH DEFAULT-BCC-LIST,DEFBCL,H.DBCC,-117
	VARH DEFAULT-CC-LIST,DEFCCL,H.DCC,-117
	VARH DONT-TYPE-HEADERS,SPRHDR,H.NOHD,INIKEY
	VARH ESCAPE-AUTOMATIC-SEND,ESCSND,H.ESC
	VARH FLAGGED-MESSAGES-AUTOTYPE-SUPPRESS,FLMAUT,H.FMAT
	VARH GET-CONNECTED-DIRECTORY,GTCNDR,H.GCD
	VARH HEADER-OPTIONS,USRHDR,H.HDOP,INILNS
	VARH KEYWORDS,KEYTBL,H.HKEY,INIKEY
	VARH LIST-CONFIRM-SUPPRESS,LPTCFM,H.LCS
	VARH LIST-DEVICE,LSTDEV,H.LDEV,-47
	VARH LIST-INCLUDE-HEADERS,LSTHDR,H.LHD
	VARH LIST-ON-SEPARATE-PAGES,LSTPAG,H.LPG
	VARH MAIL-COPY-FILE,MCPFIL,H.MCP,-247
	VARH MESSAGE-SEQUENCE-PROMPT,MSPRMT,H.MSP,-47
	VARH MORE-MODE,MORFLG,H.MMP ; CU10
	VARH NEW-FILE-PROTECTION,DEFPRO,H.PRO
	VARH ONLY-TYPE-HEADERS,ONLHDR,H.OHDR,INIKEY
	VARH PERSONAL-NAME,PERNAM,H.PNAM,-117
	VARH PROMPT-FOR-BCC,ASKBCC,H.ABCC
	VARH READ-PROMPT,REPRMT,H.RPRM,-47
	VARH REPLY-CC-OTHERS,RCCOTH,H.RCCO
	VARH REPLY-INCLUDE-ME,RINCME,H.RINM
	VARH REPLY-INITIAL-DISPLAY,REPDIS,H.RIND
	VARH REPLY-INSERT-CURRENT-MESSAGE-DEFAULT,INSMSG,H.INSM
	VARH REPLY-SENDER-ONLY-DEFAULT,RFMDEF,H.RSEN
	VARH SAVED-MESSAGES-FILE,SAVFIL,H.SAVM,-247
	VARH SEND-PROMPT,SEPRMT,H.SENP,-47
	VARH SEND-RETURN-SENDS,CRSEND,H.RSND
	VARH SEND-VERBOSE-FLAG,SNDVBS,H.SVER
	VARH SHORT-MESSAGE-LENGTH,DFSHML,H.SHML,INIDEC
	VARH TERSE-TEXT-PROMPT,TRSTPR,H.TRS
	VARH TOP-LEVEL-PROMPT,TOPRMT,H.TPLV,-47
	VARH USE-EDITOR-AUTOMATICALLY,USEEDT,H.UEDT
	VARH USER-HEADERS,USRHTB,H.USHD,INIKEY
	VARH USER-NAME,MAUSRS,H.USNM,INIUNM
	VARH VERBOSE-BBOARD-MESSAGE,VBSBBD,H.VBS
NINVRS==:.-INIVTB-1

#endif /*undef */

char *set_help_terse =
"\n\
SET TERSE-TEXT-PROMPT takes a single numeric argument.  If zero,\n\
the default, MM prompts for message text input with a list of the\n\
various control characters to exit text input and what they do.\n\
If non-zero, MM simply prompts with \"Msg:\".\n";

char *set_help_blank =
"\n\
SET BLANK-SCREEN-STARTUP takes a numeric argument.  If non-zero,\n\
the default, the screen is cleared at startup and before each\n\
message typed out when in READ mode.\n";

char *set_help_controleeditor =
"\n\
SET CONTROL-E-EDITOR takes a numeric argument.  If negative, never\n\
enter the editor on ^E; if zero, ask if should enter the editor; if\n\
positive, the default, always enter the editor.\n";

char *set_help_controlnabort =
"\n\
SET CONTROL-N-ABORT takes a numeric argument.  If negative, never\n\
abort on ^N; if zero, the default, ask if should abort; if\n\
positive, always abort.\n";

char *set_help_defaultbcc =
"\n\
SET DEFAULT-BCC-LIST takes a list of addresses as an argument,\n\
and specifies a default list to always bcc your outgoing messages\n\
to.\n";

char *set_help_defaultcc =
"\n\
SET DEFAULT-CC-LIST takes a list of addresses as an argument, and\n\
specifies a default list to always cc your outgoing messages to.\n";

char *set_help_donttypeheaders =
"\n\
SET DONT-TYPE-HEADERS takes a keyword list as an argument, and\n\
specifies a list of header keywords which should be suppressed by\n\
TYPE and related commands.\n";

char *set_help_escapeautosend =
"\n\
SET ESCAPE-AUTOMATIC-SEND takes a numeric argument.  If zero, the\n\
default, then both escape and ^Z in message text input mode will\n\
return to send level unless MM was invoked from the EXEC via a\n\
command such as \"MM SEND\", \"MAIL\", or \"SNDMSG\", in which case\n\
escape enters send level and ^Z sends the message.  If positive,\n\
then escape sends the message and ^Z returns to send level.  If\n\
negative, then ^Z sends the message and escape returns to send\n\
level.\n";

char *set_help_fmat =
"\n\
SET FLAGGED-MESSAGES-AUTOTYPE-SUPPRESS takes a numeric\n\
argument.  If non-zero, flagged messages are not automatically\n\
shown when an automatic headers list of recent messages is done\n\
(e.g. when reading in a mail file or if new messages come in).\n\
The default is zero.\n";

char *set_help_getwd =
"\n\
SET GET-CONNECTED-DIRECTORY takes a numeric argument.  If zero,\n\
the default, ask where to read in the mail file from if connected\n\
to a different directory from your login or postbox directory.\n\
If positive then read from the connected directory always; if\n\
negative, then read from the postbox directory always.\n";

char *set_help_headeroptions =
"\n\
SET HEADER-OPTIONS takes a text line as an argument and specifies\n\
a header to be inserted by default in a message.\n";

char *set_help_keywords =
"\n\
SET KEYWORDS takes a keyword list as an argument, and specifies a\n\
list of keywords by which you wish to tag your messages using the\n\
KEYWORD command.\n";

char *set_help_listconfirmsuppress =
"\n\
SET LIST-CONFIRM-SUPPRESS take a single numeric argument.  If\n\
zero, the default, LIST commands require a confirmation before\n\
outputting to the list device (typically the lineprinter).  If\n\
non-zero no confirmation is required.\n";

/*****/
char *set_help_listdevice =
"\n\
SET LIST-DEVICE takes a device name and specifies the device to\n\
use for the LIST command.  The default is LPT:.\n";

char *set_help_listincludeheaders =
"\n\
SET LIST-INCLUDE-HEADERS takes a numeric argument.  If non-zero,\n\
the default, output a list of headers at the beginning of a\n\
listing made by the LIST command.\n";

char *set_help_listonseparatepages =
"\n\
SET LIST-ON-SEPARATE-PAGES takes a numeric argument.  If\n\
non-zero, each message is listed on a separate page.  The default\n\
is zero.\n";

char *set_help_mailcopyfile =
"\n\
SET MAIL-COPY-FILE takes a file name argument, and specifies a\n\
new file into which the text of an outgoing message is copied.\n\
This differs from a SAVED-MESSAGES-FILE in that a mail\n\
copy file is a temporary file, consists solely of the text of the\n\
message (e.g. does not include the message header), and an\n\
individual copy is made for each message.  This is useful for\n\
backup purposes or for sending the same message to multiple\n\
recipients under separate cover.  The default is MAIL.CPY on your\n\
login directory; a null name disables this feature.\n";

char *set_help_messagesequenceprompt =
"\n\
SET MESSAGE-SEQUENCE-PROMPT takes a string argument and specifies\n\
the prompt meaning you're in msg-sequence mode.  The default is\n\
M>.\n";

char *set_help_moremode =
"\n\
SET MORE-MODE takes a numeric argument.  If non-zero, \"more-mode\"\n\
processing will be done in the TYPE and READ commands.\n";

/*****/
char *set_help_newfileprotection =
"\n\
SET NEW-FILE-PROTECTION takes an octal protection code as an\n\
argument and specifies the default protection to be given to text\n\
files created by MOVE, COPY, etc.  The default is the system\n\
default protection.\n";

char *set_help_onlytypeheaders =
"\n\
SET ONLY-TYPE-HEADERS takes a keyword list as an argument, and\n\
specifies a list of headers that are the only ones to be typed\n\
out by TYPE and related commands.\n";

char *set_help_personalname =
"\n\
SET PERSONAL-NAME takes a string argument and specifies a\n\
personal name to be included in the From: item in outgoing\n\
network mail messages.  The default is the name from /etc/passwd.\n";

char *set_help_promptforbcc =
"\n\
SET PROMPT-FOR-BCC takes a numeric argument.  If non-zero, then\n\
bcc recipients will be prompted for in the SEND command.\n";

char *set_help_readprompt =
"\n\
SET READ-PROMPT takes a string argument and specifies the prompt\n\
meaning you're in read mode.  The default is R>.\n";

char *set_help_replyccothers =
"\n\
SET REPLY-CC-OTHERS takes a numeric argument.  If non-zero, the\n\
default, REPLY to ALL cc's everyone other than from.  If zero,\n\
then people in the to-list are to'd, not cc'd.  Most people find\n\
it confusing to receive a reply when the to-list has other than\n\
the from address being replied to.\n";

char *set_help_replyincludeme =
"\n\
SET REPLY-INCLUDE-ME takes a numeric argument.  If positive, then\n\
include yourself in replies, if negative then if message was\n\
moved or copied to a file then the reply will go to that file as\n\
well.  If zero, the default, you aren't included in replies.\n";

char *set_help_replyinitialdisplay =
"\n\
SET REPLY-INITIAL-DISPLAY takes a numeric argument.  If non-zero\n\
then display text of reply initially.  The default is zero.\n";

char *set_help_replyinsertmessage =
"\n\
SET REPLY-INSERT-CURRENT-MESSAGE-DEFAULT takes a numeric\n\
argument.  If non-zero then insert the current message into a\n\
reply by default.  The default is zero.\n";

char *set_help_replysender =
"\n\
SET REPLY-SENDER-ONLY-DEFAULT takes a numeric argument.  If\n\
non-zero, the default, then default to replying only to the\n\
sender of the message.\n";

char *set_help_savedmessagesfile =
"\n\
SET SAVED-MESSAGES-FILE takes a file name argument, and specifies\n\
a file to receive copies of your outgoing messages.  The file is\n\
written in mail file format; you can use MM's GET command to read\n\
a SAVED-MESSAGES-FILE.  If the file does not already exist MM\n\
will ask if you want to create it.\n";

char *set_help_sendprompt =
"\n\
SET SEND-PROMPT takes a string argument and specifies the prompt\n\
meaning you're in send mode.  The default is S>.\n";

char *set_help_sendreturnsends =
"\n\
SET SEND-RETURN-SENDS takes a numeric argument.  If zero there is\n\
no default command at SEND level so an explicit SEND command must\n\
be done to send the message.  If non-zero, the default, the\n\
default command at SEND level is SEND, so that just return will\n\
send the message.\n";

char *set_help_sendverboseflag =
"\n\
SET SEND-VERBOSE-FLAG takes a numeric argument.  If negative,\n\
then superterse, i.e. say nothing about sending mail.  If 0 then\n\
tell of local delivery; if positive, the default, then\n\
superverbose, i.e. tell of the disposition of all messages.\n";

char *set_help_shortmessagelength =
"\n\
SET SHORT-MESSAGE-LENGTH takes a decimal numeric argument and specifies\n\
the default message length in characters separating \"short\" and \"long\"\n\
messages.  The default is 1500 characters.\n";

char *set_help_toplevelprompt =
"\n\
SET TOP-LEVEL-PROMPT takes a string argument and specifies the\n\
prompt meaning you're at top level.  The default is MM>.\n";

char *set_help_useeditorautomatically =
"\n\
SET USE-EDITOR-AUTOMATICALLY takes a numeric argument.  If\n\
non-zero, then go straight into the editor on any message text\n\
input.  If zero, the default, go into normal text input allowing\n\
the editor by command.\n";

char *set_help_userheaders =
"\n\
SET USER-HEADERS takes a keyword list as an argument, and\n\
specifies a list of special headers you may want to generate.\n\
The send-mode USER-HEADER command will add it to the current\n\
message.\n";

char * /*****/
set_help_username =
"\n\
SET USER-NAME takes a user name string and defaults to your\n\
logged-in user name.  This variable is MM's internal idea of your\n\
\"login user name\".  You are not allowed to set this variable to\n\
other than your \"real\" user name (your logged-in name or as\n\
established by ALIAS).  It is alright to use SET USER-NAME to\n\
specify how your user name should be cased in outgoing mail\n\
(e.g. user SMITH may want to do \"SET USER-NAME Smith\").\n";
