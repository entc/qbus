#include "qbus_cli_modules.h"

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_file.h"
#include <aio/cape_aio_file.h>

// c includes
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

//-----------------------------------------------------------------------------

struct QBusCli_s
{
  SCREEN* scr;
  
  FILE* fd;
  
  int stdout;
  
  CapeFileHandle fh;
  
  QBusCliModules cli_modules;
  
}; typedef struct QBusCli_s* QBusCli;

//-----------------------------------------------------------------------------

static void __STDCALL cli_on_keypressed (void* ptr, CapeAioFileReader freader, const char* bufdat, number_t buflen)
{
  QBusCli cli = ptr;
  
  qbus_cli_modules_stdin (cli->cli_modules, bufdat, buflen);
}

//-----------------------------------------------------------------------------

static int __STDCALL cli_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  QBusCli cli = CAPE_NEW (struct QBusCli_s);

  // create a new temporary file for the output
  cli->fh = cape_fh_new ("/tmp", "qbus_cli_log.txt");
  
  // open the file
  res = cape_fh_open (cli->fh, O_TRUNC | O_CREAT | O_WRONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  {
    CapeAioFileReader fr = cape_aio_freader_new (STDIN_FILENO, cli, cli_on_keypressed);
    
    if (cape_aio_freader_add (&fr, qbus_aio (qbus)) == FALSE)
    {
      res = cape_err_lastOSError (err);
      goto exit_and_cleanup;
    }
  }
  
  // store the current stdout
  cli->stdout = dup(STDOUT_FILENO);
  
  // reset the current stdout to the file
  dup2((number_t)cape_fh_fd (cli->fh), STDOUT_FILENO);
  
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
  
  cli->cli_modules = qbus_cli_modules_new (qbus, cli->scr);

  cape_log_msg (CAPE_LL_TRACE, "QBUS", "cli", "switched to NCURSES screen");

  // set the terminal screen
  set_term (cli->scr);

  res = qbus_cli_modules_init (cli->cli_modules, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
      
  // set user pointer
  *p_ptr = cli;
  cli = NULL;
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  if (cli)
  {
    
  }
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL cli_on_done (QBus qbus, void* ptr, CapeErr err)
{
  QBusCli cli = ptr;
  
  // cleanup GUI modules
  qbus_cli_modules_del (&(cli->cli_modules));
  
  // return the term
  endwin ();

  // frees storage associated with the SCREEN data structure
  delscreen(cli->scr);

  fclose (cli->fd);
    
  fsync(STDOUT_FILENO);
  dup2(cli->stdout , STDOUT_FILENO);
  
  cape_fh_del (&(cli->fh));

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
