#ifndef __QBUS__H
#define __QBUS__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//=============================================================================

struct QBus_s; typedef struct QBus_s* QBus; // use a simple version

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBus               qbus_new               (const char* module);

__CAPE_LIBEX   void               qbus_del               (QBus*);

__CAPE_LIBEX   int                qbus_wait              (QBus, CapeUdc bind, CapeUdc remotes, CapeErr);

//-----------------------------------------------------------------------------

#define QBUS_MTYPE_NONE         0
#define QBUS_MTYPE_JSON         1

struct QBusMessage_s
{
  number_t mtype;

  CapeUdc clist;    // list of all parameters
  
  CapeUdc cdata;    // object as parameters
  
  CapeUdc rinfo;
  
  CapeErr err;
  
  CapeString chain_key;  // don't change this key
  
  CapeString sender;     // don't change this
  
}; typedef struct QBusMessage_s* QBusM;

//-----------------------------------------------------------------------------

typedef int    (__STDCALL         *fct_qbus_onMessage)   (QBus, void* ptr, QBusM qin, QBusM qout, CapeErr);
typedef void   (__STDCALL         *fct_qbus_onRemoved)   (void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                qbus_register          (QBus, const char* method, void* ptr, fct_qbus_onMessage, fct_qbus_onRemoved, CapeErr);

__CAPE_LIBEX   int                qbus_send              (QBus, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage, CapeErr);

__CAPE_LIBEX   int                qbus_continue          (QBus, const char* module, const char* method, QBusM qin, void** p_ptr, fct_qbus_onMessage, CapeErr);

__CAPE_LIBEX   int                qbus_response          (QBus, const char* module, QBusM msg, CapeErr);

__CAPE_LIBEX   const CapeString   qbus_name              (QBus);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString   qbus_config_s          (QBus, const char* name, const CapeString default_val);

__CAPE_LIBEX   number_t           qbus_config_n          (QBus, const char* name, number_t default_val);

__CAPE_LIBEX   double             qbus_config_f          (QBus, const char* name, double default_val);

__CAPE_LIBEX   int                qbus_config_b          (QBus, const char* name, int default_val);

//-----------------------------------------------------------------------------

struct QBusConnection_s; typedef struct QBusConnection_s* QBusConnection;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusConnection const  qbus_find_conn      (QBus, const char* module);

__CAPE_LIBEX   void               qbus_conn_request      (QBus, QBusConnection const, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusM              qbus_message_new       (const CapeString key, const CapeString sender);

__CAPE_LIBEX   void               qbus_message_del       (QBusM*);

__CAPE_LIBEX   void               qbus_message_clr       (QBusM);

//-----------------------------------------------------------------------------

typedef int      (__STDCALL     *fct_qbus_on_init) (QBus, void* ptr, void** p_ptr, CapeErr);
typedef int      (__STDCALL     *fct_qbus_on_done) (QBus, void* ptr, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_instance          (const char* name, void* ptr, fct_qbus_on_init, fct_qbus_on_done, int argc, char *argv[]);

//=============================================================================

#endif
