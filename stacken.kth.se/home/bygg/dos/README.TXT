This package includes the files:
	INTWIN.PH	Interrupt List phrases file for HC?? compression 
        README.TXT      This file
	FILE_ID.DIZ	Identification file

INTWIN.PH is a phrases file compiled from Ralf Brown's Interrupt List release
57 with Christian Mueller-Planitz's and Bent Lynggaard's program INT2WHLP. A
phrases file contains compression information used by the HC31 and HCP help
file compilers when compiling in high compression mode.

The compilation includes INTERRUP.LST and most of the supplementary *.LST
files in Ralf Brown's Interrupt List package. New in this compilation is the
inclution of opcode.lst. The new configuration file is included in this
package.

The reason for supplying this phrases file is that it is requires much
computer resources to compile it:
	630 kb < required DOS memory < 730 kb
        48 Mb < required free disk space < 58 Mb
which is not easily available on many computers.

1998-03-23.
Bent Lynggaard <bent.lynggaard@risoe.dk>

