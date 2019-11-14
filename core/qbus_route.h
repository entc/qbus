#ifndef __QBUS__ROUTE__H
#define __QBUS__ROUTE__H 1

#include "qbus.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"

//=============================================================================

struct QBusFrame_s; typedef struct QBusFrame_s* QBusFrame;

//=============================================================================

struct QBusRoute_s; typedef struct QBusRoute_s* QBusRoute;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusRoute         qbus_route_new           (QBus qbus, const CapeString name);

__CAPE_LIBEX   void              qbus_route_del           (QBusRoute*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              qbus_route_conn_reg      (QBusRoute, QBusConnection);

__CAPE_LIBEX   void              qbus_route_conn_rm       (QBusRoute, QBusConnection);

__CAPE_LIBEX   void              qbus_route_conn_onFrame  (QBusRoute, QBusConnection, QBusFrame*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              qbus_route_meth_reg      (QBusRoute, const char* method, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm);

__CAPE_LIBEX   int               qbus_route_request       (QBusRoute, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage, int cont, CapeErr err);

__CAPE_LIBEX   void              qbus_route_response      (QBusRoute, const char* module, QBusM msg, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc           qbus_route_modules       (QBusRoute);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusConnection const  qbus_route_module_find (QBusRoute, const char* module_origin);

__CAPE_LIBEX   void              qbus_route_conn_request  (QBusRoute, QBusConnection const, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, int cont);

__CAPE_LIBEX   void*             qbus_route_add_on_change     (QBusRoute, void* ptr, fct_qbus_on_route_change);

__CAPE_LIBEX   void              qbus_route_rm_on_change      (QBusRoute, void* obj);

__CAPE_LIBEX   void              qbus_route_run_on_change     (QBusRoute, CapeUdc* p_modules);

__CAPE_LIBEX   void              qbus_route_methods           (QBusRoute, const char* module, void* ptr, fct_qbus_on_methods);

//=============================================================================

#endif
