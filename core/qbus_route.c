#include "qbus_route.h"
#include "qbus_core.h"
#include "qbus_route_items.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_mutex.h"
#include "sys/cape_log.h"
#include "stc/cape_map.h"
#include "fmt/cape_json.h"

// c includes
#include <stdio.h>

//-----------------------------------------------------------------------------

#define QBUS_METHOD_TYPE__REQUEST      1
#define QBUS_METHOD_TYPE__RESPONSE     2
#define QBUS_METHOD_TYPE__FORWARD      3
#define QBUS_METHOD_TYPE__METHODS      4

struct QBusMethod_s
{
  int type;
  
  void* ptr;
  
  fct_qbus_onMessage onMsg;
  
  fct_qbus_onRemoved onRm;
  
  // for continue
  
  CapeString chain_key;
  
  CapeString chain_sender;
  
  CapeUdc rinfo;
};

typedef struct QBusMethod_s* QBusMethod;

//-----------------------------------------------------------------------------

QBusMethod qbus_method_new (int type, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm)
{
  QBusMethod self = CAPE_NEW (struct QBusMethod_s);

  self->type = type;
  
  self->ptr = ptr;
  self->onMsg = onMsg;
  self->onRm = onRm;
  
  self->chain_key = NULL;
  self->chain_sender = NULL;
  
  self->rinfo = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_method_del (QBusMethod* p_self)
{ 
  QBusMethod self = *p_self;
  
  if (self->onRm)
  {
    self->onRm (self->ptr);
  }
  
  if (self->rinfo)
  {
    cape_udc_del (&(self->rinfo));
  }
  
  cape_str_del (&(self->chain_key));
  cape_str_del (&(self->chain_sender));

  CAPE_DEL (p_self, struct QBusMethod_s);
}

//-----------------------------------------------------------------------------

void qbus_method_continue (QBusMethod self, CapeString* p_chain_key, CapeString* p_chain_sender, CapeUdc* p_rinfo)
{
  cape_str_replace_mv (&(self->chain_key), p_chain_key);
  cape_str_replace_mv (&(self->chain_sender), p_chain_sender);
  
  self->rinfo = *p_rinfo;
  *p_rinfo = NULL;
}

//-----------------------------------------------------------------------------

int qbus_method_call_request (QBusMethod self, QBus qbus, QBusFrame frame, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {
    // convert the frame content into the input message (expensive)
    QBusM qin = qbus_frame_qin (frame);

    // create an empty output message
    QBusM qout = qbus_message_new (NULL, NULL);

    // call the original callback    
    res = self->onMsg (qbus, self->ptr, qin, qout, err);

    // override the frame content with the output message (expensive)
    qbus_frame_set_qmsg (frame, qout, err);
    
    // cleanup    
    qbus_message_del (&qin);
    qbus_message_del (&qout);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_request__msg (QBusMethod self, QBus qbus, QBusM qin, QBusM qout, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {    
    // call the original callback    
    res = self->onMsg (qbus, self->ptr, qin, qout, err);    
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response__continue_chain (QBusMethod self, QBus qbus, QBusRoute route, QBusFrame frame_original, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  // convert the frame content into the input message (expensive)
  QBusM qin = qbus_frame_qin (frame_original);

  if (qin->rinfo == NULL)
  {
    // transfer ownership
    qin->rinfo = self->rinfo;
    self->rinfo = NULL;
  }
  
  // create an empty output message
  QBusM qout = qbus_message_new (NULL, NULL);
  
  // correct chainkey and sender
  // this is important, if another continue was used
  cape_str_replace_cp (&(qin->chain_key), self->chain_key);
  cape_str_replace_cp (&(qin->sender), self->chain_sender);
  
  switch (self->onMsg (qbus, self->ptr, qin, qout, err))
  {
    case CAPE_ERR_CONTINUE:
    {
      break;
    }
    default:
    {
      // use the correct chain key
      cape_str_replace_cp (&(qout->chain_key), self->chain_key);
      
      // send the response
      qbus_route_response (route, self->chain_sender, qout, err);
      
      break;
    }
  }

  // cleanup
  qbus_message_del (&qin);
  qbus_message_del (&qout);

  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response__response (QBusMethod self, QBus qbus, QBusRoute route, QBusFrame frame, CapeErr err)
{
  int res;
  
  // convert the frame content into the input message (expensive)
  QBusM qin = qbus_frame_qin (frame);

  // call the original callback
  res = self->onMsg (qbus, self->ptr, qin, NULL, err);

  // cleanup
  qbus_message_del (&qin);

  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response (QBusMethod self, QBus qbus, QBusRoute route, QBusFrame frame, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {
    if (self->chain_key)
    {
      return qbus_method_call_response__continue_chain (self, qbus, route, frame, err);
    }
    else
    {
      return qbus_method_call_response__response (self, qbus, route, frame, err);
    }
  }
  
  return res;
}

//-----------------------------------------------------------------------------

struct QBusOnChangeCallback_s
{
  fct_qbus_on_route_change fct;
  
  void* ptr;
  
}; typedef struct QBusOnChangeCallback_s* QBusOnChangeCallback;

//-----------------------------------------------------------------------------

static void __STDCALL qbus_route_callbacks_on_del (void* ptr)
{
  QBusOnChangeCallback cc = ptr;
  
  CAPE_DEL(&cc, struct QBusOnChangeCallback_s);
}

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  QBus qbus;   // reference 
  
  CapeString name;
  
  CapeMap methods;
  
  CapeMap chains;
  
  CapeMutex chain_mutex;
  
  QBusRouteItems route_items;  
  
  // for on change
  
  CapeList on_changes_callbacks;
  
  CapeMutex on_changes_mutex;
  
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_methods_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethod qmeth = val; qbus_method_del (&qmeth);
  }
}

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new (QBus qbus, const CapeString name)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);
  
  self->qbus = qbus;
  
  self->name = cape_str_cp (name);
  self->methods = cape_map_new (NULL, qbus_route_methods_del, NULL);
  
  self->chain_mutex = cape_mutex_new ();
  self->chains = cape_map_new (NULL, qbus_route_methods_del, NULL);
  
  self->route_items = qbus_route_items_new ();
  
  self->on_changes_callbacks = cape_list_new (qbus_route_callbacks_on_del);
  self->on_changes_mutex = cape_mutex_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  QBusRoute self = *p_self;
  
  cape_str_del (&(self->name));
  cape_map_del (&(self->methods));
  
  cape_mutex_del (&(self->chain_mutex));
  cape_map_del (&(self->chains));
  
  qbus_route_items_del (&(self->route_items));
  
  cape_list_del (&(self->on_changes_callbacks));
  cape_mutex_del (&(self->on_changes_mutex));
  
  CAPE_DEL (p_self, struct QBusRoute_s);
}

//-----------------------------------------------------------------------------

void qbus_route_conn_reg (QBusRoute self, QBusConnection conn)
{
  // log
  cape_log_msg (CAPE_LL_TRACE, "QBUS", "conn reg", "new connection");
  
  // send first frame
  {
    QBusFrame frame = qbus_frame_new ();
        
    qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_REQ, NULL, NULL, NULL, self->name);
    
    // finally send the frame
    qbus_connection_send (conn, &frame);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_send_updates (QBusRoute self, QBusConnection conn_origin)
{
  CapeList list_of_all_connections = qbus_route_items_conns (self->route_items, conn_origin);
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (list_of_all_connections, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      QBusConnection conn = cape_list_node_data (cursor->node);
      
      QBusFrame frame = qbus_frame_new ();
      
      qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_UPD, NULL, NULL, NULL, self->name);
      
      CapeUdc route_nodes = qbus_route_items_nodes (self->route_items);
      
      {
        CapeString h = cape_json_to_s (route_nodes);
        
        // log
        cape_log_fmt (CAPE_LL_TRACE, "QBUS", "route update", "send route update: %s -> %s", h, qbus_connection_get (conn));
        
        cape_str_del(&h);
      }
      
      if (route_nodes)
      {
        // set the payload frame
        qbus_frame_set_udc (frame, QBUS_MTYPE_JSON, &route_nodes);
      }
      
      // finally send the frame
      qbus_connection_send (conn, &frame);
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_list_del (&list_of_all_connections);
}

//-----------------------------------------------------------------------------

void qbus_route_conn_rm (QBusRoute self, QBusConnection conn)
{
  const CapeString module = qbus_connection_get (conn);
  
  // log
  cape_log_msg (CAPE_LL_TRACE, "QBUS", "conn reg", "connection dropped");
  
  if (module)
  {
    qbus_route_items_rm (self->route_items, module);    
  }
  else
  {
    
  }
  
  qbus_route_send_updates (self, conn);  
  
  {
    CapeUdc modules = qbus_route_items_nodes (self->route_items);

    qbus_route_run_on_change (self, &modules);
  }
}

//-----------------------------------------------------------------------------

QBusConnection const qbus_route_module_find (QBusRoute self, const char* module_origin)
{
  return qbus_route_items_get (self->route_items, module_origin);
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_request (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  qbus_frame_set_type (frame, QBUS_FRAME_TYPE_ROUTE_RES, self->name);
    
  CapeUdc route_nodes = qbus_route_items_nodes (self->route_items);

  if (route_nodes)
  {
    // set the payload frame
    qbus_frame_set_udc (frame, QBUS_MTYPE_JSON, &route_nodes);
  }
  
  // finally send the frame
  qbus_connection_send (conn, p_frame);
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_response (QBusRoute self, QBusConnection conn, QBusFrame frame)
{
  CapeUdc route_nodes = qbus_frame_get_udc (frame);
  
  qbus_route_items_add (self->route_items, qbus_frame_get_sender (frame), conn, &route_nodes);
  
  // tell the others the new nodes
  qbus_route_send_updates (self, conn);  

  {
    CapeUdc modules = qbus_route_items_nodes (self->route_items);
    
    qbus_route_run_on_change (self, &modules);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_update (QBusRoute self, QBusConnection conn, QBusFrame frame)
{
  const CapeString module = qbus_connection_get (conn);
  
  if (module)
  {
    CapeUdc route_nodes = qbus_frame_get_udc (frame);

    if (route_nodes)
    {
      qbus_route_items_update (self->route_items, module, &route_nodes);
    }

    {
      CapeUdc modules = qbus_route_items_nodes (self->route_items);
      
      qbus_route_run_on_change (self, &modules);
    }
  }
}

//-----------------------------------------------------------------------------

typedef struct
{
  
  CapeString chain_key;
  
  CapeString sender;
  
} QBusForwardData;

//-----------------------------------------------------------------------------

typedef struct
{
  void* ptr;
  
  fct_qbus_on_methods on_methods;

} QBusMethodsData;

//-----------------------------------------------------------------------------

void qbus_route_on_msg_foward (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;

  QBusForwardData* qbus_fd = CAPE_NEW (QBusForwardData);
  
  // create a copy of the chain key
  qbus_fd->chain_key = cape_str_cp (qbus_frame_get_chainkey  (frame));
  qbus_fd->sender = cape_str_cp (qbus_frame_get_sender  (frame));

  // create a new chain key
  CapeString chain_key = cape_str_uuid();
  
  cape_mutex_lock (self->chain_mutex);
  
  {
    QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__FORWARD, qbus_fd, NULL, NULL);
    
    // transfer ownership of chain_key to the map
    cape_map_insert (self->chains, (void*)chain_key, (void*)qmeth);
  }
  
  cape_mutex_unlock (self->chain_mutex);
  
  // chain key
  {
    CapeString h = cape_str_cp (chain_key);
    
    qbus_frame_set_chainkey (frame, &h);
  }
    
  // sender
  {
    CapeString sender = cape_str_cp (self->name);
    
    qbus_frame_set_sender (frame, &sender);
  }
    
  // forward the frame
  qbus_connection_send (conn, p_frame);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_method (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  CapeString method = cape_str_cp (qbus_frame_get_method (frame));
  
  cape_str_to_lower (method);
  
  CapeMapNode n = cape_map_find (self->methods, method);    
  if (n)
  {
    QBusMethod qmeth = cape_map_node_value (n);
    
    switch (qmeth->type)
    {
      case QBUS_METHOD_TYPE__REQUEST:
      {
        CapeErr err = cape_err_new ();
        
        switch (qbus_method_call_request (qmeth, self->qbus, frame, err))
        {
          case CAPE_ERR_CONTINUE:
          {
            break;
          }
          default:
          {
            qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
            
            // finally send the frame
            qbus_connection_send (conn, p_frame);
          }
        }
        
        cape_err_del (&err);        
       
        break;
      }
    }    
  }
  else
  {
    CapeErr err = cape_err_new ();
    
    cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", qbus_frame_get_method (frame));
    
    qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
    
    qbus_frame_set_err (frame, err);
    
    cape_err_del (&err);
    
    // finally send the frame
    qbus_connection_send (conn, p_frame);
  }
  
  cape_str_del (&method);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_request (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  const CapeString module = qbus_frame_get_module (frame);
  
  // check if the message was sent to us
  if (cape_str_equal (module, self->name))
  {
    qbus_route_on_msg_method (self, conn, p_frame);
  }
  else  // the message was not send to us -> forward it 
  {
    // try to find a connection which might reach the destination module
    QBusConnection conn_forward = qbus_route_items_get (self->route_items, module);
    if (conn_forward)
    {
      qbus_route_on_msg_foward (self, conn_forward, p_frame);
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "no route to %s", module);
      
      qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
      
      qbus_frame_set_err (frame, err);
      
      cape_err_del (&err);
      
      // finally send the frame
      qbus_connection_send (conn, p_frame);
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_forward (QBusRoute self, QBusFrame* p_frame, QBusForwardData** p_qbus_fd)
{
  QBusFrame frame = *p_frame;
  
  QBusForwardData* qbus_fd = *p_qbus_fd;
  
  // try to find a connection which might reach the destination module
  QBusConnection conn_forward = qbus_route_items_get (self->route_items, qbus_fd->sender);
  if (conn_forward)
  {
    qbus_frame_set_chainkey (frame, &(qbus_fd->chain_key));
    qbus_frame_set_sender (frame, &(qbus_fd->sender));
    
    // forward the frame
    qbus_connection_send (conn_forward, p_frame);
  }
  else
  {
    // log
    cape_log_msg (CAPE_LL_ERROR, "QBUS", "msg forward", "forward message can't be returned");
  }
  
  cape_str_del (&(qbus_fd->chain_key));
  cape_str_del (&(qbus_fd->sender));
  
  CAPE_DEL(p_qbus_fd, QBusForwardData);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_methods (QBusRoute self, QBusFrame* p_frame, QBusMethodsData** p_qbus_methods)
{
  QBusFrame frame = *p_frame;

  QBusMethodsData* qbus_methods = *p_qbus_methods;
  
  if (qbus_methods->on_methods)
  {
    CapeErr err = cape_err_new ();
    
    CapeUdc methods = qbus_frame_get_udc (frame);
    
    qbus_methods->on_methods (self->qbus, qbus_methods->ptr, methods, err);

    cape_udc_del (&methods);
    
    cape_err_del (&err);
  }
  
  CAPE_DEL(p_qbus_methods, QBusMethodsData);
}

//-----------------------------------------------------------------------------

void qbus_route_on_msg_response (QBusRoute self, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  const CapeString chain_key = qbus_frame_get_chainkey (frame);
  
  if (chain_key)
  {
    CapeMapNode n;
   
    cape_mutex_lock (self->chain_mutex);
    
    n = cape_map_find (self->chains, (void*)chain_key);
    
    if (n)
    {
      cape_map_extract (self->chains, n);
    }
    
    cape_mutex_unlock (self->chain_mutex);

    if (n)
    {
      QBusMethod qmeth = cape_map_node_value (n);
      
      switch (qmeth->type)
      {
        case QBUS_METHOD_TYPE__REQUEST:
        {
          // this should not happen
          
          break;
        }
        case QBUS_METHOD_TYPE__RESPONSE:
        {
          CapeErr err = cape_err_new ();
          
          qbus_method_call_response (qmeth, self->qbus, self, frame, err);

          cape_err_del (&err);
          
          break;
        }
        case QBUS_METHOD_TYPE__FORWARD:
        {
          QBusForwardData* qbus_fd = qmeth->ptr;
          
          qbus_route_on_msg_forward (self, p_frame, &qbus_fd);
          
          break;
        }
        case QBUS_METHOD_TYPE__METHODS:
        {
          QBusMethodsData* qbus_methods = qmeth->ptr;
          
          qbus_route_on_msg_methods (self, p_frame, &qbus_methods);
         
          break;
        }
      }
      
      // cleanup
      cape_map_del_node (self->chains, &n);
    }
    else
    {
      
      
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route__generate_modules_list (QBusRoute self)
{
  // encode methods into list
  CapeUdc method_list = cape_udc_new (CAPE_UDC_LIST, NULL);

  // iterate through all methods
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->methods, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      //QBusMethod qmeth = cape_map_node_value (cursor->node);
     
      //CapeUdc method_node = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      //cape_udc_add_s_cp (method_node, "name", cape_map_node_key (cursor->node));
      
      //cape_udc_add (method_list, &method_node);
      
      cape_udc_add_s_cp (method_list, NULL, cape_map_node_key (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  return method_list;
}

//-----------------------------------------------------------------------------

void qbus_route_on_route_methods_request (QBusRoute self, QBusConnection conn, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  const CapeString module = qbus_frame_get_module (frame);
  
  // check if the message was sent to us
  if (cape_str_equal (module, self->name))
  {
    // encode methods into list
    CapeUdc method_list = qbus_route__generate_modules_list (self);

    /*
    {
      CapeString h = cape_json_to_s (method_list);
      
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "methods req", "send methods: %s", h);
      
      cape_str_del(&h);
    }
    */

    qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
    qbus_frame_set_udc (frame, QBUS_FRAME_TYPE_MSG_RES, &method_list);
    
    // send the frame back
    qbus_connection_send (conn, p_frame);
  }
  else  // the message was not send to us -> forward it 
  {
    // try to find a connection which might reach the destination module
    QBusConnection conn_forward = qbus_route_items_get (self->route_items, module);
    if (conn_forward)
    {
      qbus_route_on_msg_foward (self, conn_forward, p_frame);
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "no route to %s", module);
      
      qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
      
      qbus_frame_set_err (frame, err);
      
      cape_err_del (&err);
      
      // finally send the frame
      qbus_connection_send (conn, p_frame);
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_conn_onFrame (QBusRoute self, QBusConnection connection, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  switch (qbus_frame_get_type (frame))
  {
    case QBUS_FRAME_TYPE_ROUTE_REQ:
    {
      qbus_route_on_route_request (self, connection, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_ROUTE_RES:
    {
      qbus_route_on_route_response (self, connection, frame);
      break;
    }
    case QBUS_FRAME_TYPE_ROUTE_UPD:
    {
      qbus_route_on_route_update (self, connection, frame);
      break;
    }
    case QBUS_FRAME_TYPE_MSG_REQ:
    {
      qbus_route_on_msg_request (self, connection, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_MSG_RES:
    {
      qbus_route_on_msg_response (self, p_frame);
      break;
    }
    case QBUS_FRAME_TYPE_METHODS:
    {
      qbus_route_on_route_methods_request (self, connection, p_frame);
      break;
    }
  }
  
  qbus_frame_del (p_frame);    
}

//-----------------------------------------------------------------------------

void qbus_route_meth_reg (QBusRoute self, const char* method_origin, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm)
{
  CapeString method = cape_str_cp (method_origin);
  
  cape_str_to_lower (method);

  QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__REQUEST, ptr, onMsg, onRm);
  
  cape_map_insert (self->methods, (void*)method, (void*)qmeth);
}

//-----------------------------------------------------------------------------

void qbus_route_no_route (QBusRoute self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  // log
  cape_log_fmt (CAPE_LL_WARN, "QBUS", "msg forward", "no route to module %s", module);
  
  if (onMsg)
  {
    if (msg->err)
    {
      cape_err_del (&(msg->err));
    }
    
    // create a new error object
    msg->err = cape_err_new ();
    
    // set the error
    cape_err_set (msg->err, CAPE_ERR_NOT_FOUND, "no route to module");
    
    {
      // create a temporary error object
      CapeErr err = cape_err_new ();
      
      int res = onMsg (self->qbus, ptr, msg, NULL, err);
      if (res)
      {
        // TODO: handle error
        
      }
      
      cape_err_del (&err);
    }      
  }  
}

//-----------------------------------------------------------------------------

void qbus_route_conn_request (QBusRoute self, QBusConnection const conn, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, int cont)
{
  // create a new frame
  QBusFrame frame = qbus_frame_new ();
  
  {
    QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__RESPONSE, ptr, onMsg, NULL);
    
    CapeString h = cape_str_uuid();

    // add default content
    qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_REQ, h, module, method, self->name);
    
    // add message content
    msg->rinfo = qbus_frame_set_qmsg (frame, msg, NULL);

    if (cont && msg->chain_key)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "add chainkey '%s' for continue", msg->chain_key);
      
      qbus_method_continue (qmeth, &(msg->chain_key), &(msg->sender), &(msg->rinfo));
    }
    
    cape_mutex_lock (self->chain_mutex);
    
    cape_map_insert (self->chains, (void*)h, (void*)qmeth);

    cape_mutex_unlock (self->chain_mutex);
  }
  
  // finally send the frame
  qbus_connection_send (conn, &frame);
}

//-----------------------------------------------------------------------------

int qbus_route_request__find_method_and_call (QBusRoute self, const char* method_origin, QBusM msg, QBusM qout, CapeErr err)
{
  int res;
  
  // const local objects
  CapeMapNode n;
  QBusMethod qmeth;
  
  // local objects
  CapeString method = cape_str_cp (method_origin);
  
  // convert into lower case
  cape_str_to_lower (method);
  
  // try to find the method
  n = cape_map_find (self->methods, method);    
  if (n == NULL)
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method);
    goto exit_and_cleanup;
  }
  
  // get the methods object
  qmeth = cape_map_node_value (n);
  
  switch (qmeth->type)
  {
    case QBUS_METHOD_TYPE__REQUEST:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call method '%s'", method_origin);
      
      res = qbus_method_call_request__msg (qmeth, self->qbus, msg, qout, err);
      break;
    }
    default:
    {
      res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method);
      break;
    }
  }
  
exit_and_cleanup:
  
  cape_str_del (&method);  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_route_request__local_chains (QBusRoute self, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__RESPONSE, ptr, onMsg, NULL);

  CapeString h = cape_str_uuid();
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "add chainkey '%s' for continue", h);
  
  qbus_method_continue (qmeth, &(msg->chain_key), &(msg->sender), &(msg->rinfo));
  
  cape_mutex_lock (self->chain_mutex);
  
  cape_map_insert (self->chains, (void*)h, (void*)qmeth);
  
  cape_mutex_unlock (self->chain_mutex);
}

//-----------------------------------------------------------------------------

void qbus_route_request__local_response (QBusRoute self, QBusM msg, QBusM qin, void* ptr, fct_qbus_onMessage onMsg)
{
  CapeErr err = cape_err_new ();
  QBusM qout = qbus_message_new (NULL, NULL);
  
  int res = CAPE_ERR_NONE;
  
  if (onMsg)
  {
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call method callback");
    
    // debug
    if (qin->cdata)
    {
      CapeString h = cape_json_to_s (qin->cdata);
      
      printf ("QIN CDATA: %s\n", h);

      cape_str_del(&h);
    }
    // debug
    if (msg->cdata)
    {
      CapeString h = cape_json_to_s (msg->cdata);
      
      printf ("MSG CDATA: %s\n", h);
      
      cape_str_del(&h);
    }
    
    // TODO: workaround
    if (qin->cdata)
    {
      cape_udc_merge_cp (qin->cdata, msg->cdata);
    }
    else
    {
      cape_udc_replace_cp (&(qin->cdata), msg->cdata);
    }
    
    res = onMsg (self->qbus, ptr, qin, qout, err);
  }
  
  switch (res)
  {
    case CAPE_ERR_CONTINUE:
    {
      //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request callback", "call returned a continued state");
      
      // don't do anything
      break;
    }
    default:
    {
      // transfer ownership
      cape_udc_replace_mv (&(msg->cdata), &(qout->cdata));
      
      // TODO:
      
      //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request callback", "call returned a terminated state with chais key = '%s'", msg->chain_key);
      
      break;
    }
  }
  
  // cleanup
  qbus_message_del (&qout);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qbus_route_request__local_request (QBusRoute self, const char* method_origin, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  CapeErr err = cape_err_new ();
  QBusM qout = qbus_message_new (NULL, NULL);
  
  // set default message type
  qout->mtype = QBUS_MTYPE_JSON;
  
  int res = qbus_route_request__find_method_and_call (self, method_origin, msg, qout, err);
  
  switch (res)
  {
    case CAPE_ERR_CONTINUE:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call returned a continued state");
      
      // this request shall not continue and another request was created instead
      // we need to add the callback and ptr to the chains list, for response
      qbus_route_request__local_chains (self, msg, ptr, onMsg);
      
      break;
    }
    default:
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "call returned a terminated state");
      
      if (res)
      {
        // tranfer ownership
        qout->err = err;
        
        // create an empty error object
        err = cape_err_new ();
      }
      
      // this requests ends here, now send the results back
      qbus_route_request__local_response (self, msg, qout, ptr, onMsg);
      
      break;
    }
  }
  
  // cleanup
  qbus_message_del (&qout);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

int qbus_route_request (QBusRoute self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, int cont)
{
  /*
  if (cape_str_compare (module, self->name))
  {
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "request", "execute local request on '%s'", module);
    
    qbus_route_request__local_request (self, method, msg, ptr, onMsg);
    
    return CAPE_ERR_NONE;
  }
  else
   */
  {
    QBusConnection const conn = qbus_route_module_find (self, module);
    
    if (conn)
    {
      qbus_route_conn_request (self, conn, module, method, msg, ptr, onMsg, cont);
      
      return CAPE_ERR_CONTINUE;
    }
    else
    {
      qbus_route_no_route (self, module, method, msg, ptr, onMsg);

      return CAPE_ERR_NONE;
    }
  }  
}

//-----------------------------------------------------------------------------

void qbus_route_response (QBusRoute self, const char* module, QBusM msg, CapeErr err)
{
  QBusConnection const conn = qbus_route_module_find (self, module);
  
  if (conn)
  {
    // create a new frame
    QBusFrame frame = qbus_frame_new ();
    
    // add default content
    qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_RES, msg->chain_key, module, NULL, self->name);
    
    // add message content
    qbus_frame_set_qmsg (frame, msg, err);
    
    // finally send the frame
    qbus_connection_send (conn, &frame);
  }
  else
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "route response", "no route for response '%s'", module);
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_modules (QBusRoute self)
{
  return qbus_route_items_nodes (self->route_items);
}

//-----------------------------------------------------------------------------

void* qbus_route_add_on_change (QBusRoute self, void* ptr, fct_qbus_on_route_change on_change)
{
  CapeListNode n = NULL;
  
  cape_mutex_lock (self->on_changes_mutex);

  {
    QBusOnChangeCallback cc = CAPE_NEW (struct QBusOnChangeCallback_s);
    
    cc->fct = on_change;
    cc->ptr = ptr;
    
    n = cape_list_push_back (self->on_changes_callbacks, cc);
  }
  
  cape_mutex_unlock (self->on_changes_mutex);
  
  return n;
}

//-----------------------------------------------------------------------------

void qbus_route_run_on_change (QBusRoute self, CapeUdc* p_modules)
{
  cape_mutex_lock (self->on_changes_mutex);
  
  {
    CapeListCursor cursor; cape_list_cursor_init (self->on_changes_callbacks, &cursor, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (&cursor))
    {
      QBusOnChangeCallback cc = cape_list_node_data (cursor.node);
      
      if (cc->fct)
      {
        cc->fct (self->qbus, cc->ptr, *p_modules);
      }
    }    
  }
  
  cape_mutex_unlock (self->on_changes_mutex);
  
  cape_udc_del (p_modules);
}

//-----------------------------------------------------------------------------

void qbus_route_rm_on_change (QBusRoute self, void* obj)
{
  CapeListNode ret = obj;
  
  cape_mutex_lock (self->on_changes_mutex);
  
  cape_list_node_erase (self->on_changes_callbacks, ret);
  
  cape_mutex_unlock (self->on_changes_mutex);
}

//-----------------------------------------------------------------------------

void qbus_route_methods (QBusRoute self, const char* module, void* ptr, fct_qbus_on_methods on_methods)
{
  QBusConnection const conn = qbus_route_module_find (self, module);
  
  if (conn)
  {
    // create a new frame
    QBusFrame frame = qbus_frame_new ();

    // create a new chain key
    CapeString chain_key = cape_str_uuid();

    QBusMethodsData* qbus_methods = CAPE_NEW (QBusMethodsData);
    
    qbus_methods->ptr = ptr;
    qbus_methods->on_methods = on_methods;
    
    cape_mutex_lock (self->chain_mutex);
    
    {
      QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__METHODS, qbus_methods, NULL, NULL);
      
      // transfer ownership of chain_key to the map
      cape_map_insert (self->chains, (void*)chain_key, (void*)qmeth);
    }
    
    cape_mutex_unlock (self->chain_mutex);

    qbus_frame_set (frame, QBUS_FRAME_TYPE_METHODS, chain_key, module, NULL, self->name);
        
    // send the frame
    qbus_connection_send (conn, &frame);
  }
  else
  {
    if (on_methods)
    {
      CapeErr err = cape_err_new ();

      // set the error
      cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "no route to module: %s", module);
      
      // callback
      on_methods (self->qbus, ptr, NULL, err);
      
      cape_err_del (&err);
    }
  }
}

//-----------------------------------------------------------------------------
