/*
** System dependent code for comnd, BSD version.
*/

#include <sgtty.h>
#include <signal.h>

static void settty(void)
{
  struct sgttyb ttyblk;
  struct ltchars ltc;
  int fd;

  fd = fileno(stdin);

  ioctl(fd,TIOCGETP,&ttyblk);		/* get original parameters */
  ttyblk.sg_flags &= ~(RAW | ECHO | LCASE); /* no echoing or xlates */
  ttyblk.sg_flags |= CBREAK;		/* single character reads */
  ioctl(fd,TIOCSETN,&ttyblk);		/* set params, leave typeahead */

  ioctl(fd,TIOCGLTC,&ltc);		/* get current local special chars */
  ltc.t_lnextc = -1;			/* disable literal-next */
  ioctl(fd,TIOCSLTC,&ltc);		/* set the new chars in place */
}

static void sig_tstp(int signo)
{
  sigset_t mask;

  sigemptyset(&mask);
  sigaddset(&mask, SIGTSTP);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);

  signal(SIGTSTP, SIG_DFL);
  raise(SIGTSTP);
  signal(SIGTSTP, sig_tstp);
  settty();
}

void sys_init(void)
{
  (void) signal(SIGTSTP, sig_tstp);
  settty();
}
