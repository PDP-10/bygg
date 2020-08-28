/*
** System dependent code for comnd, solaris version.
*/

#include <sgtty.h>
#define BSD_COMP 1
#include <sys/ioctl.h>
#include <sys/ttold.h>

void sys_init(void)
{
  struct sgttyb ttyblk;
  struct ltchars ltc;
  int fd;

  fd = fileno(stdin);

  ioctl(fd,TIOCGETP,&ttyblk);           /* get original parameters */
  ttyblk.sg_flags &= ~(RAW | ECHO | LCASE); /* no echoing or xlates */
  ttyblk.sg_flags |= CBREAK;            /* single character reads */
  ioctl(fd,TIOCSETN,&ttyblk);           /* set params, leave typeahead */

  ioctl(fd,TIOCGLTC,&ltc);              /* get current local special chars */
  ltc.t_lnextc = -1;                    /* disable literal-next */
  ioctl(fd,TIOCSLTC,&ltc);              /* set the new chars in place */
}
