#include "qbus.h"

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
  QBusCli cli = CAPE_NEW (struct QBusCli_s);

  // set user pointer
  *p_ptr = cli;
  
  cli->fd = fopen ("/dev/tty", "r+");

  cli->scr = newterm (NULL, cli->fd, cli->fd);
  
  curs_set(0);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL cli_on_done (QBus qbus, void* ptr, CapeErr err)
{
  QBusCli cli = ptr;
  
  delscreen(cli->scr);

  fclose (cli->fd);
  
  endwin ();
  
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
