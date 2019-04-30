#ifndef __QBUS__PROTOCOL__H
#define __QBUS__PROTOCOL__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_stream.h"

#include "qbus_route.h"

//-----------------------------------------------------------------------------

#define QBUS_FRAME_TYPE_NONE         0
#define QBUS_FRAME_TYPE_ROUTE_REQ    1
#define QBUS_FRAME_TYPE_ROUTE_RES    2
#define QBUS_FRAME_TYPE_MSG_REQ      3
#define QBUS_FRAME_TYPE_MSG_RES      4
#define QBUS_FRAME_TYPE_ROUTE_UPD    5

//=============================================================================

__CAPE_LIBEX   QBusFrame         qbus_frame_new           ();

__CAPE_LIBEX   void              qbus_frame_del           (QBusFrame*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              qbus_frame_set           (QBusFrame, number_t ftype, const char* chain_key, const char* module, const char* method, const char* sender);

__CAPE_LIBEX   void              qbus_frame_set_type      (QBusFrame, number_t ftype, const char* sender);

__CAPE_LIBEX   void              qbus_frame_set_chainkey  (QBusFrame, CapeString* p_chain_key);

__CAPE_LIBEX   void              qbus_frame_set_sender    (QBusFrame, CapeString* p_sender);

__CAPE_LIBEX   void              qbus_frame_set_err       (QBusFrame, CapeErr);

// returns the rinfo if available
__CAPE_LIBEX   CapeUdc           qbus_frame_set_udc       (QBusFrame, number_t msgType, CapeUdc* p_payload);

// returns the rinfo if available
__CAPE_LIBEX   CapeUdc           qbus_frame_set_qmsg      (QBusFrame, QBusM, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   number_t          qbus_frame_get_type      (QBusFrame);

__CAPE_LIBEX   const CapeString  qbus_frame_get_module    (QBusFrame);

__CAPE_LIBEX   const CapeString  qbus_frame_get_method    (QBusFrame);

__CAPE_LIBEX   const CapeString  qbus_frame_get_sender    (QBusFrame);

__CAPE_LIBEX   const CapeString  qbus_frame_get_chainkey  (QBusFrame);

__CAPE_LIBEX   CapeUdc           qbus_frame_get_udc       (QBusFrame);

__CAPE_LIBEX   QBusM             qbus_frame_qin           (QBusFrame);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int               qbus_frame_decode        (QBusFrame, const char* bufdat, number_t buflen, number_t* written);

__CAPE_LIBEX   void              qbus_frame_encode        (QBusFrame, CapeStream cs);

//=============================================================================

#endif
