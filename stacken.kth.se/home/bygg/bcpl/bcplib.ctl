;FILENAME:	BCPLIB.CTL
;
;PETER GARDNER, COMPUTING CENTRE, UNIVERSITY OF ESSEX,
;WIVENHOE PARK, COLCHESTER, ESSEX, CO4 3SQ.
;
;QUEUE COMMAND:	.SUBMIT BCPLIB/TIME:20:00
;
;THIS FILE PRODUCES LIBRARIES FOR BCPL VERSION 3H.
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
;	MACRO
;	PIP
;	FUDGE2
;	SUBFIL
;
;REQUIRES THE FOLLOWING FILES ON DSK:
;
;	BCPLIB.CTL
;
;	BCPLIB.CMD
;	LOWLIB.CMD
;
;	IOLIB.GET
;	BCPLIB.GET
;	IOPACK.GET
;	IOUUO.GET
;	USEFUL.GET
;	ERRORS.GET
;	SCB.GET
;	BITS.GET
;	ACS.GET
;	JOBDAT.GET
;	KERNEL.GET
;	LOW.GET
;	HIGH.GET
;
;	BCLFIT.BCP
;	BCLCRT.BCP
;	BCLAPP.BCP
;	BCLCRE.BCP
;	BCLFIN.BCP
;	BCLUPD.BCP
;	BCLDOF.BCP
;	BCLDEL.BCP
;	BCLRNA.BCP
;	BCLIOE.BCP
;	BCLIOM.BCP
;	BCLFTT.BCP
;	BCLGET.BCP
;	BCLSWI.BCP
;	BCLRDA.BCP
;	BCLRDB.BCP
;	BCLWRB.BCP
;	BCLRDD.BCP
;	BCLWRD.BCP
;	BCLERS.BCP
;	BCLEOF.BCP
;	BCLSTR.BCP
;	BCLNEW.BCP
;	BCLSPA.BCP
;	BCLREW.BCP
;	BCLINF.BCP
;	BCLRDF.BCP
;	BCLREF.BCP
;	BCLINN.BCP
;	BCLRDN.BCP
;	BCLREN.BCP
;	BCLPUT.BCP
;	BCLSTO.BCP
;	BCLSER.BCP
;	BCLJOB.BCP
;	BCLOTI.BCP
;	BCLWTI.BCP
;	BCLODA.BCP
;	BCLWDA.BCP
;	BCLOUT.BCP
;	BCLWRI.BCP
;	BCLOVZ.BCP
;	BCLWRX.BCP
;	BCLOOP.BCP
;	BCLWOP.BCP
;	BCLOUF.BCP
;	BCLWRF.BCP
;	BCLOU8.BCP
;	BCLWR8.BCP
;	BCLOUI.BCP
;	BCLWIN.BCP
;	BCLONO.BCP
;	BCLWNO.BCP
;	BCLOUS.BCP
;	BCLWRS.BCP
;	BCLOHX.BCP
;	BCLWHX.BCP
;	BCLOOC.BCP
;	BCLWOC.BCP
;	BCLOU6.BCP
;	BCLWR6.BCP
;	BCLOAS.BCP
;	BCLWAS.BCP
;	BCLOAZ.BCP
;	BCLWAZ.BCP
;	BCLGOP.BCP
;	BCLRUN.BCP
;	BCLR50.BCP
;	BCLSIX.BCP
;	BCLLOO.BCP
;	BCLENT.BCP
;	BCLOPE.BCP
;	BCLREL.BCP
;	BCLOBF.BCP
;	BCLIBF.BCP
;	BCLINU.BCP
;	BCLOUU.BCP
;	BCLWAU.BCP
;	BCLCLU.BCP
;	BCLUGF.BCP
;	BCLSTS.BCP
;	BCLGST.BCP
;	BCLMTA.BCP
;	BCLUSI.BCP
;	BCLUSO.BCP
;	BCLLHS.BCP
;	BCLNUM.BCP
;	BCLTTY.BCP
;	BCLINC.BCP
;	BCLRDC.BCP
;	BCLREA.BCP
;	BCLOUC.BCP
;	BCLWRC.BCP
;	BCLCLO.BCP
;	BCLSCB.BCP
;	BCLPAK.BCP
;	BCLUNP.BCP
;	BCLIOU.BCP
;	BCLAPL.BCP
;	BCLVEC.BCP
;	BCLIOW.BCP
;	BCLRES.BCP
;	BCLECH.BCP
;	BCLNOE.BCP
;	BCLWOR.BCP
;	BCLCOS.BCP
;	BCLSIN.BCP
;	BCLL10.BCP
;	BCLLNZ.BCP
;	BCLLG2.BCP
;	BCLRAN.BCP
;	BCLLEV.BCP
;	BCLLAB.BCP
;	BCLJUM.BCP
;	BCLLJU.BCP
;	BCLCON.BCP
;	BCLDEV.BCP
;	BCLERR.BCP
;	BCLEXT.BCP
;	BCLFIL.BCP
;	BCLINP.BCP
;	BCLMON.BCP
;	BCLOPU.BCP
;	BCLPPN.BCP
;	BCLSBC.BCP
;	BCLSWO.BCP
;	BCLTTZ.BCP
;	BCLTRA.BCP
;	BCLTBF.BCP
;	BCLOV2.MAC
;	BCLOVR.BCP
;	BCLKER.BCP
;	BCLUUO.BCP
;	BCLSYZ.BCP
;	BCLERZ.BCP
;	BCLSOZ.BCP
;	BCLZER.BCP
;	BCLTTO.BCP
;	BCLTTN.BCP
;	BCLTTS.BCP
;	BCLTT8.BCP
;	BCLSTA.BCP
;	BCLFLZ.BCP
;	BCLSLZ.BCP
;	BCLSOV.BCP
;	BCLDEB.BCP
;	BCLSPB.BCP
;	BCLCOZ.BCP
;	BCLBOO.BCP
;	BCLOWN.BCP
;	BCLCCL.BCP
;	BCLBAT.BCP
;	BCLSTK.BCP
;	BCLFNZ.BCP
;	BCLPDL.BCP
;
;	MBOOTS.MAC
;
;	BCPLIB.SUB
;
;PRODUCES THE FOLLOWING FILES
;
;	ACS.GET		KEPT ON BCL:
;	BCPLIB.GET	KEPT ON BCL:
;	BITS.GET	KEPT ON BCL:
;	ERRORS.GET	KEPT ON BCL:
;	IOLIB.GET	KEPT ON BCL:
;	IOPACK.GET	KEPT ON BCL:
;	IOUUO.GET	KEPT ON BCL:
;	JOBDAT.GET	KEPT ON BCL:
;	KERNEL.GET	KEPT ON BCL:
;	SCB.GET		KEPT ON BCL:
;	USEFUL.GET	KEPT ON BCL:
;
;	BCPLIB.REL	KEPT ON SYS:
;	LOWLIB.REL	KEPT ON BCL:
;
;AND THE FOLLOWING LISTING FILES
;
;	BCPLIB.MAS	QUEUED LISTING
;
;SET UP [10,7] AS LIB:
;
.R SETSRC
*LIB[10,7]
*T
.ASSIGN DSK SYS
.ASSIGN DSK BCL
;
;NOW COMPILE AND FUDGE THE NORMAL LIBRARY
;
.TYPE BCPLIB.CMD
.COMPILE/COMPILE/FUDGE:BCPLIB @BCPLIB
.FUDGE
;
;NOW FUDGE IN AN INDEX
;
.R FUDGE2
*BCPLIB/X_BCPLIB^[
;
;NOW COMPILE AND FUDGE THE LOWCODE LIBRARY
;
.TYPE LOWLIB.CMD
.COMPILE/COMPILE/FUDGE:LOWLIB @LOWLIB
.FUDGE
;
;NOW FUDGE IN AN INDEX
;
.R FUDGE2
*LOWLIB/X_LOWLIB^[
;
;NOW PRODUCE BCPLIB.MAS
;
.R SUBFIL-BCPLIB.SUB
;
;NOW TIDY UP AND QUEUE BCPLIB.MAS
;
.DEASSIGN SYS
.DEASSIGN BCL
.QUEUE BCPLIB.MAS
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           