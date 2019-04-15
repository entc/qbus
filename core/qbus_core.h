#ifndef __QBUS__CORE__H
#define __QBUS__CORE__H 1

#include "qbus_frame.h"
#include "qbus_route.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//=============================================================================

__CAPE_LIBEX   QBusConnection    qbus_connection_new      (QBusRoute, number_t channels);

__CAPE_LIBEX   void              qbus_connection_del      (QBusConnection*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              qbus_connection_onSent   (QBusConnection, void* userdata);

__CAPE_LIBEX   void              qbus_connection_onRecv   (QBusConnection, const char* bufdat, ssize_t buflen);

__CAPE_LIBEX   void              qbus_connection_send     (QBusConnection, QBusFrame*);

//-----------------------------------------------------------------------------

typedef void (__STDCALL *fct_qbus_connection_send) (void* ptr1, void* ptr2, const char* bufdat, ssize_t buflen, void* userdata);
typedef void (__STDCALL *fct_qbus_connection_mark) (void* ptr1, void* ptr2);

__CAPE_LIBEX   void              qbus_connection_cb       (QBusConnection, void* ptr1, void* ptr2, fct_qbus_connection_send, fct_qbus_connection_mark);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              qbus_connection_reg      (QBusConnection);

__CAPE_LIBEX   void              qbus_connection_set      (QBusConnection, const CapeString ident);

__CAPE_LIBEX   const CapeString  qbus_connection_get      (QBusConnection);

//-----------------------------------------------------------------------------

#endif

