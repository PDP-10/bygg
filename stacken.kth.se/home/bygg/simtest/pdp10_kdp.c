/*
** pdp10_kdp.c: KMC11/DUP11 combo.
*/

/*
** Written 2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

#include "pdp10_defs.h"

extern int32 int_req;

extern t_stat unibus_read(int32* data, int32 addr);
extern t_stat unibus_write(int32 data, int32 addr);

extern t_stat dma_read(int32 ba, uint8* data, int length);
extern t_stat dma_write(int32 ba, uint8* data, int length);

#define KMC_RDX 8		/* Octal in this incarnation. */
#define DUP_RDX 8

#include "dec_kdp.c"
