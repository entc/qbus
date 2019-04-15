#ifndef __QBUS__ROUTE_ITEMS__H
#define __QBUS__ROUTE_ITEMS__H 1

#include "qbus_core.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_list.h"

//=============================================================================

struct QBusRouteItems_s; typedef struct QBusRouteItems_s* QBusRouteItems;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusRouteItems    qbus_route_items_new     (void);

__CAPE_LIBEX   void              qbus_route_items_del     (QBusRouteItems*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              qbus_route_items_add        (QBusRouteItems, const CapeString module, QBusConnection conn, CapeUdc*);

__CAPE_LIBEX   QBusConnection    qbus_route_items_get        (QBusRouteItems, const CapeString module);

__CAPE_LIBEX   void              qbus_route_items_rm         (QBusRouteItems, const CapeString module);

__CAPE_LIBEX   void              qbus_route_items_update     (QBusRouteItems, const CapeString module, CapeUdc*);

__CAPE_LIBEX   CapeUdc           qbus_route_items_nodes      (QBusRouteItems);

__CAPE_LIBEX   CapeList          qbus_route_items_conns      (QBusRouteItems, QBusConnection exception);

//=============================================================================

#endif

