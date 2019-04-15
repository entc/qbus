#ifndef __QBUS__ENGINE__TCP__H
#define __QBUS__ENGINE__TCP__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "aio/cape_aio_sock.h"

#include "qbus_route.h"

//=============================================================================

struct EngineTcpInc_s; typedef struct EngineTcpInc_s* EngineTcpInc;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   EngineTcpInc      qbus_engine_tcp_inc_new      (CapeAioContext aio, QBusRoute, const CapeString host, number_t port);

__CAPE_LIBEX   void              qbus_engine_tcp_inc_del      (EngineTcpInc*);

__CAPE_LIBEX   int               qbus_engine_tcp_inc_listen   (EngineTcpInc, CapeErr err);

//=============================================================================

struct EngineTcpOut_s; typedef struct EngineTcpOut_s* EngineTcpOut;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   EngineTcpOut      qbus_engine_tcp_out_new      (CapeAioContext aio, QBusRoute, const CapeString host, number_t port);

__CAPE_LIBEX   void              qbus_engine_tcp_out_del      (EngineTcpOut*);

__CAPE_LIBEX   int             qbus_engine_tcp_out_reconnect  (EngineTcpOut, CapeErr);

//-----------------------------------------------------------------------------

#endif
