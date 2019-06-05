#ifndef __QBUS_CLI_METHODS__H
#define __QBUS_CLI_METHODS__H 1

#include "sys/cape_export.h"
#include "stc/cape_udc.h"

// includes c curses
#include <curses.h>

#include "qbus.h"

//-----------------------------------------------------------------------------

struct QBusCliMethods_s; typedef struct QBusCliMethods_s* QBusCliMethods;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusCliMethods   qbus_cli_methods_new    (QBus qbus, SCREEN* screen);

__CAPE_LIBEX   void             qbus_cli_methods_del    (QBusCliMethods*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int              qbus_cli_methods_init   (QBusCliMethods, CapeErr);

__CAPE_LIBEX   void             qbus_cli_methods_stdin  (QBusCliMethods, const char* bufdat, number_t buflen);

__CAPE_LIBEX   void             qbus_cli_methods_set    (QBusCliMethods, const CapeString module);

//-----------------------------------------------------------------------------

#endif
