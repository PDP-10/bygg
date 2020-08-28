/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifdef RCSID
#ifndef lint
static char *cmds_rcsid = "$Header: /amd/watsun/w/src1/sun4.bin/cucca/mm/RCS/cmds.H,v 2.1 90/10/04 18:23:39 melissa Exp $";
#endif
#endif /* RCSID */

/*
 * cmds.h - indices into the command dispatch table
 *
 * The value of the symbols below must correspond to the offset of
 * the corresponding command in the cmd_fn[] array!
 */

/*
 * These are also indices into the help string index.  Help topics
 * which are not commands should be #def'd at the end BEFORE NUMTOPICS
 */

#number 0
#def CMD_ALIAS       
#def CMD_ANSWER
#def CMD_BACKTRACK
#def CMD_BCC         
#def CMD_BLANK       
#def CMD_BROWSE      
#def CMD_BUG         
#def CMD_CC          
#def CMD_CD          
#def CMD_CHECK       
#def CMD_CONTINUE    
#def CMD_COPY        
#def CMD_COUNT       
#def CMD_CREATE_INIT 
#def CMD_DAYTIME     
#def CMD_DEBUG       
#def CMD_MEMDEBUG    
#def CMD_DEFINE      
#def CMD_DELETE      
#def CMD_DISPLAY     
#def CMD_ECHO        
#def CMD_EDIT        
#def CMD_ERASE       
#def CMD_EXAMINE     
#def CMD_EXIT        
#def CMD_EXPUNGE     
#def CMD_FCC
#def CMD_FINGER
#def CMD_FLAG        
#def CMD_FOLLOW      
#def CMD_FORWARD     
#def CMD_FROM        
#def CMD_GET         
#def CMD_HEADERS     
#def CMD_HELP        
#def CMD_INSERT      
#def CMD_IN_REPLY_TO 
#def CMD_JUMP        
#def CMD_KEYWORD     
#def CMD_KILL        
#def CMD_LIST	 
#def CMD_LITERAL
#def CMD_MARK         
#def CMD_MOVE         
#def CMD_NEXT         
#def CMD_PREVIOUS     
#def CMD_PRINT        
#def CMD_PROFILE      
#def CMD_PUSH         
#def CMD_PWD          
#def CMD_QUIT         
#def CMD_QQUIT        
#def CMD_READ         
#def CMD_REMAIL       
#def CMD_REMOVE       
#def CMD_REPLY        
#def CMD_REPLY_TO     
#def CMD_RESTORE_DRAFT        
#def CMD_REVIEW
#def CMD_ROUTE        
#def CMD_SAVE_DRAFT   
#def CMD_SEND         
#def CMD_SET          
#def CMD_SHOW
#def CMD_SMAIL
#def CMD_SORT         
#def CMD_SPELL        
#def CMD_STATUS       
#def CMD_SUBJECT      
#def CMD_SUSPEND      
#def CMD_TAKE         
#def CMD_TEXT         
#def CMD_TO           
#def CMD_TYPE         
#def CMD_UNANSWER     
#def CMD_UNDELETE     
#def CMD_UNFLAG       
#def CMD_UNKEYWORD    
#def CMD_UNMARK       
#def CMD_USER_HEADER  
#def CMD_VERSION      
#def CMD_WHO          
#def CMD_WRITE        
#def CMD_Z            

#def CMD_HEADER       
#def CMD_EXPAND       

#def CMD_SENDER       
#def CMD_ALL          

#def CMD_INCLUDE      
#def CMD_NOINCLUDE     

#def HLP_CCMD
#def HLP_MESSAGE_SEQUENCE
#def HLP_MMINIT
#def HLP_MM
#def HLP_SHELL

#def HLP_TYPE_BABYL
#define HLP_TYPE_START HLP_TYPE_BABYL
#def HLP_TYPE_MBOX
#def HLP_TYPE_MH
#def HLP_TYPE_MTXT
#def HLP_TYPE_POP2
#def HLP_TYPE_POP3

#def NUMTOPICS
