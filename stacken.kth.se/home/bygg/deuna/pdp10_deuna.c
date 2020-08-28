/*
** pdp10_deuna.c: DEUNA ethernet controller.
*/

#include "pdp10_defs.h"
#include "dec_deuna.c"

extern d10 *M;						/* main memory */
extern int32 ubmap[UBANUM][UMAP_MEMSIZE];		/* UBA map */
extern int32 int_req;

/*
** set/clear the deuna interrupt:
*/

void deuna_setint(void)
{
  int_req |= INT_DEUNA;
}

void deuna_clrint(void)
{
  int_req &= ~INT_DEUNA;
}

/*
** read main memory via UBA:
*/

#define XBA_MBZ  0400000	/* addr<17> must be 0 */

t_stat unibus_read(int32* data, int32 addr)
{
  int32 vpn;
  a10 pa10;

  vpn = PAG_GETVPN (addr >> 2);			/* get PDP-10 page number */
  if ((vpn >= UMAP_MEMSIZE) || (addr & XBA_MBZ) ||
      ((ubmap[1][vpn] & UMAP_VLD) == 0)) {		/* invalid map? */
    return SCPE_NXM;
  }

  pa10 = (ubmap[1][vpn] + PAG_GETOFF (addr >> 2)) & PAMASK;
  if (MEM_ADDR_NXM (pa10)) {			/* nxm? */
    return SCPE_NXM;
  }

  *data = (int32) ((M[pa10] >> ((addr & 2)? 0: 18)) & 0177777);

  return SCPE_OK;
}

/*
** we don't handle byte size writes.  Also note that the real UBA does not
** preserve the rest of the pdp10 word when writing the first byte/word of
** data.  Maybe we should emulate this truthfully.
**
** Maybe this routine should move into pdp10_ksio.c for general use?
*/

t_stat unibus_write(int32 data, int32 addr)
{
  int32 vpn;
  a10 pa10;
  d10 wd;

  vpn = PAG_GETVPN (addr >> 2);			/* get PDP-10 page number */
  if ((vpn >= UMAP_MEMSIZE) || (addr & XBA_MBZ) ||
      ((ubmap[1][vpn] & UMAP_VLD) == 0)) {		/* invalid map? */
    return SCPE_NXM;
  }

  pa10 = (ubmap[1][vpn] + PAG_GETOFF (addr >> 2)) & PAMASK;
  if (MEM_ADDR_NXM (pa10)) {			/* nxm? */
    return SCPE_NXM;
  }

  wd = M[pa10];
  data &= 0177777;

  if (addr & 2) {
    wd &= LMASK;
    wd |= data;
  } else {
    wd &= RMASK;
    wd |= (d10) ((d10) data << 18);
  }

  M[pa10] = wd;

  return SCPE_OK;
}
