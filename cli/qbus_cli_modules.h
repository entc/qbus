#ifndef __QBUS_CLI_MODULES__H
#define __QBUS_CLI_MODULES__H 1

#include "sys/cape_export.h"
#include "stc/cape_udc.h"

// includes c curses
#include <curses.h>

#include "qbus.h"

//-----------------------------------------------------------------------------

struct QBusCliModules_s; typedef struct QBusCliModules_s* QBusCliModules;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusCliModules   qbus_cli_modules_new    (QBus qbus, SCREEN* screen);

__CAPE_LIBEX   void             qbus_cli_modules_del    (QBusCliModules*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int              qbus_cli_modules_init   (QBusCliModules, CapeErr);

__CAPE_LIBEX   void             qbus_cli_modules_stdin  (QBusCliModules, const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------

#endif
