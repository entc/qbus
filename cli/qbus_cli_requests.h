#ifndef __QBUS_CLI_REQUESTS__H
#define __QBUS_CLI_REQUESTS__H 1

#include "sys/cape_export.h"
#include "stc/cape_udc.h"

// includes c curses
#include <curses.h>

#include "qbus.h"

//-----------------------------------------------------------------------------

struct QBusCliRequests_s; typedef struct QBusCliRequests_s* QBusCliRequests;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusCliRequests  qbus_cli_requests_new      (QBus qbus, SCREEN* screen);
 
__CAPE_LIBEX   void             qbus_cli_requests_del      (QBusCliRequests*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int              qbus_cli_requests_init     (QBusCliRequests, CapeErr err);

__CAPE_LIBEX   void             qbus_cli_requests_refresh  (QBusCliRequests, const char* method);

//-----------------------------------------------------------------------------

#endif

