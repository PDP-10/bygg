;FILENAME:	BUILD.CTL
;
;PETER GARDNER, COMPUTING CENTRE, UNIVERSITY OF ESSEX,
;WIVENHOE PARK, COLCHESTER, ESSEX, C04 3SQ.
;
;QUEUE COMMAND: .SUBMIT BUILD/TIME:1:00
;
;THIS FILE CREATES A BCPL SYSTEM FROM THE FILES ON THE
;RELEASE TAPE. IT ALSO CONTAINS THE INFORMATION FOR
;EXTRACTING THE SOURCE FILES NECESSARY TO COMPILE THE
;COMPILER AND LIBRARY.
;
;ALL THE OTHER LIBRARY FILES ON THE TAPE MAY BE PUT ONTO
;BCL: IF/WHEN REQUIRED. THEY ARE MORE LIKELY TO BE USED
;IF THEY PERMANENTLY RESIDE THERE.
;
;*********************************************************************;
;                                                                     ;
; THE FOLLOWING MODS TO THE SYSTEM/CUSPS ARE ESSENTIAL TO THE SMOOTH  ;
; RUNNING OF BCPL PROGRAMS:                                           ;
;                                                                     ;
; 1. CHECK THE CODE IN LINK RECOGNISES COMPILER CODE #13 AS REQUIRING ;
;    SYS:BCPLIB.REL AS THE DEFAULT LIBRARY. CODE IS IN LINK V2 ONWARD.;
; 2. CHECK COMPIL RECOGNISES THE EXTENSION .BCL AND .BCP AS REQUIRING ;
;    THE BCPL COMPILER. COM22C.SCM IS A FILCOM FILE OF NECESSARY MODS.;
; 3. GET A BCPL LIBRARY AREA BCL: ALLOCATED AND KNOWN TO THE MONITOR. ;
;                                                                     ;
;*********************************************************************;
;
;REQUIRES THE FOLLOWING FILES ON SYS:
;
;	COMPIL
;	PIP
;
;REQUIRES THE FOLLOWING FILES ON DSK:
;
;	BUILD.CTL
;
;	BCPL.MAS
;	BCPLIB.MAS
;	BCPLIB.REL
;	GLOBAL.REL
;	LOWLIB.REL
;	BCPL.EXE
;	BCPL00.EXE
;	BCPL01.EXE
;	BCPL02.EXE
;	BCPL03.EXE
;	SUBFIL.EXE
;
;PUTS THE FOLLOWING FILES ON SYS:
;
;	BCPL.EXE
;	BCPL00.EXE
;	BCPL01.EXE
;	BCPL02.EXE
;	BCPL03.EXE
;	SUBFIL.EXE
;	BCPL.HLP
;	BCPL.TXT
;	BCPLIB.REL
;
;PUTS THE FOLLOWING FILES ON BCL:
;
;	ACS.GET
;	BCPLIB.GET
;	BITS.GET
;	ERRORS.GET
;	IOLIB.GET
;	IOPACK.GET
;	IOUUO.GET
;	JOBDAT.GET
;	SCB.GET
;	USEFUL.GET
;	KERNEL.GET
;	BCPLIB.MAS
;	GLOBAL.REL
;	LOWLIB.REL
;
;EXTRACT THOSE FILES REQUIRED FOR SYS:
;
.RU SUBFIL
*BCPL.HLP_BCPL.MAS
*BCPL.TXT_BCPL.MAS
;
;NOW COPY SYS: FILES
;
.COPY SYS:=BCPL.EXE
.COPY SYS:=BCPL00.EXE
.COPY SYS:=BCPL01.EXE
.COPY SYS:=BCPL02.EXE
.COPY SYS:=BCPL03.EXE
.COPY SYS:=SUBFIL.EXE
.COPY SYS:=BCPL.HLP
.COPY SYS:=BCPL.TXT
.COPY SYS:=BCPLIB.REL
;
;NOW DELETE UNWANTED FILES JUST CREATED
;
.DELETE BCPL.HLP,BCPL.TXT
;
;EXTRACT THOSE FILES REQUIRED FOR BCL:
;
.R SUBFIL
*ACS.GET_BCPLIB.MAS
*BCPLIB.GET_BCPLIB.MAS
*BITS.GET_BCPLIB.MAS
*ERRORS.GET_BCPLIB.MAS
*IOLIB.GET_BCPLIB.MAS
*IOPACK.GET_BCPLIB.MAS
*IOUUO.GET_BCPLIB.MAS
*JOBDAT.GET_BCPLIB.MAS
*SCB.GET_BCPLIB.MAS
*USEFUL.GET_BCPLIB.MAS
*KERNEL.GET_BCPLIB.MAS
;
;NOW COPY BCL: FILES
;
.COPY BCL:=ACS.GET
.COPY BCL:=BCPLIB.GET
.COPY BCL:=BITS.GET
.COPY BCL:=ERRORS.GET
.COPY BCL:=IOLIB.GET
.COPY BCL:=IOPACK.GET
.COPY BCL:=IOUUO.GET
.COPY BCL:=JOBDAT.GET
.COPY BCL:=SCB.GET
.COPY BCL:=USEFUL.GET
.COPY BCL:=KERNEL.GET
.COPY BCL:=BCPLIB.MAS
.COPY BCL:=GLOBAL.REL
.COPY BCL:=LOWLIB.REL
;
;NOW DELETE UNWANTED FILES JUST CREATED
;
.DELETE ACS.GET,BCPLIB.GET,BITS.GET,ERRORS.GET,IOLIB.GET,IOPACK.GET
.DELETE IOUUO.GET,JOBDAT.GET,SCB.GET,USEFUL.GET,KERNEL.GET
;
;NOW WE HAVE A STANDARD BCPL SYSTEM
;
;******************************************************************;
;                                                                  ;
; WARNING: BEFORE TRYING TO RUN ANY OF THE .CTL FILES FOR BUILDING ;
; BCPL, BCPLIB OR LOWLIB THE FOLLOWING MUST BE DONE:               ;
;                                                                  ;
; .R SUBFIL-BCPL.MAS                                               ;
; .R SUBFIL-BCPLIB.MAS                                             ;
;                                                                  ;
;******************************************************************;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        