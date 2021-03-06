;FILENAME:	SUBFIL.CTL
;
;PETER GARDNER, COMPUTING CENTRE, UNIVERSITY OF ESSEX,
;WIVENHOE PARK, COLCHESTER, ESSEX, CO4 3SQ.
;
;QUEUE COMMAND:	.SUBMIT SUBFIL/TIME:5:00
;
;THIS FILE PRODUCES SUBFIL (3B) FOR BCPL VERSION 3F.
;
;REQUIRES THE FOLLOWING FILES ON SYS:
;
;	SETSRC
;	QUEUE
;	QMANGR
;
;REQUIRES THE FOLLOWING FILES ON LIB:
;
;	BCPL
;	BCPL00
;	COMPIL
;	LINK
;	LNK???
;
;REQUIRES THE FOLLOWING FILES ON DSK:
;
;	SUBFIL.CTL
;	BCPLIB.GET
;	BITS.GET
;	ERRORS.GET
;	IOPACK.GET
;	RFS.GET
;	SUBFIL.BCL
;
;PRODUCES THE FOLLOWING FILES
;
;	SUBFIL.EXE	KEPT ON SYS:
;
;AND THE FOLLOWING LISTING FILES
;
;	SUBFIL.BCL	QUEUED LISTING
;
;SET UP [10,7] AS LIB:
;
.R SETSRC
*M /LIB[10,7]
*T
.ASSIGN DSK SYS
.ASSIGN DSK BCL
;
;NOW COMPILE AND LOAD
;
.LOAD /COMPILE SUBFIL.BCL(RFSSSS)
.VERSION
.CORE
.NSSAVE SUBFIL
;
;NOW SHOULD HAVE SUBFIL (3B)
;
.DEASSIGN SYS
.DEASSIGN BCL
.QUEUE SUBFIL.BCL
                                                                                                                                                                                                                                                                                                                                               