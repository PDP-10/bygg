;FILENAME:	GLOBAL.CTL
;
;PETER GARDNER, COMPUTING CENTRE, UNIVERSITY OF ESSEX,
;WIVENHOE PARK, COLCHESTER, ESSEX, CO4 3SQ.
;
;QUEUE COMMAND:	.SUBMIT GLOBAL/TIME:5:00
;
;THIS FILE PRODUCES GLOBAL.REL FOR BCPL VERSION 3B.
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
;	GLOBAL.CTL
;	BCPLIB.GET
;	GLOBAL.BCL
;
;PRODUCES THE FOLLOWING FILES
;
;	GLOBAL.REL	KEPT ON BCL:
;
;SET UP [10,7] AS LIB:
;
.R SETSRC
*M /LIB[10,7]
*T
.ASSIGN DSK SYS
.ASSIGN DSK BCL
;
;NOW COMPILE LOAD AND EXECUTE
;
.EXECUTE GLOBAL.BCL
;
;NOW SHOULD HAVE GLOBAL.REL
;
.DEASSIGN SYS
.DEASSIGN BCL
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             