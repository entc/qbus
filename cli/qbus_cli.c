#include "qbus.h"
#include "sys/cape_log.h"

#include <stdlib.h>
#include <ncurses.h>

struct QBusCli_s
{
  SCREEN* scr;
  
  FILE* fd;
  
}; typedef struct QBusCli_s* QBusCli;

//-----------------------------------------------------------------------------

static int __STDCALL cli_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  QBusCli cli = CAPE_NEW (struct QBusCli_s);

  cli->fd = fopen ("/dev/tty", "r+");
  
  if (cli->fd == NULL)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  cli->scr = newterm (NULL, cli->fd, cli->fd);

  if (cli->scr == NULL)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  set_term (cli->scr);

  cape_log_msg (CAPE_LL_TRACE, "QBUS", "cli", "switched to NCURSES screen");
  
  // set user pointer
  *p_ptr = cli;
  cli = NULL;
  
  
  mvprintw(0, 0, "hello ncurses");
  
  refresh();
  
exit_and_cleanup:

  if (cli)
  {
    
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL cli_on_done (QBus qbus, void* ptr, CapeErr err)
{
  QBusCli cli = ptr;
  
  // return the term
  endwin ();

  // frees storage associated with the SCREEN data structure
  delscreen(cli->scr);

  fclose (cli->fd);
    
  CAPE_DEL (&cli, struct QBusCli_s);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("CLI", NULL, cli_on_init, cli_on_done, argc, argv);
  
  return 0;
}

//-----------------------------------------------------------------------------
