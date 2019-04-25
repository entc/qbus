#include "qbus_route.h"
#include "qbus_core.h"
#include "qbus_route_items.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_mutex.h"
#include "stc/cape_map.h"
#include "fmt/cape_json.h"

// c includes
#include <stdio.h>

//-----------------------------------------------------------------------------

#define QBUS_METHOD_TYPE__REQUEST      1
#define QBUS_METHOD_TYPE__RESPONSE     2
#define QBUS_METHOD_TYPE__FORWARD      3

struct QBusMethod_s
{
  int type;
  
  void* ptr;
  
  fct_qbus_onMessage onMsg;
  
  fct_qbus_onRemoved onRm;
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
  
  CAPE_DEL (p_self, struct QBusMethod_s);
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
    qbus_frame_set_qmsg (frame, qout);
    
    // cleanup    
    qbus_message_del (&qin);
    qbus_message_del (&qout);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_method_call_response (QBusMethod self, QBus qbus, QBusFrame frame, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  if (self->onMsg)
  {
    // convert the frame content into the input message (expensive)
    QBusM qin = qbus_frame_qin (frame);
    
    // call the original callback    
    res = self->onMsg (qbus, self->ptr, qin, NULL, err);
    
    // cleanup    
    qbus_message_del (&qin);
  }
  
  return res;
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
  
  CAPE_DEL (p_self, struct QBusRoute_s);
}

//-----------------------------------------------------------------------------

void qbus_route_conn_reg (QBusRoute self, QBusConnection conn)
{
  printf ("NEW QBUS CONNECTION\n");
  
  QBusFrame frame = qbus_frame_new ();
  
  qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_REQ, NULL, NULL, NULL, self->name);
  
  // finally send the frame
  qbus_connection_send (conn, &frame);
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
        
        printf ("SEND ROUTE UPDATE: %s -> %s\n", h, qbus_connection_get (conn));
        
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
  printf ("QBUS CONNECTION DROPPED\n");
  
  const CapeString module = qbus_connection_get (conn);
  
  if (module)
  {
    qbus_route_items_rm (self->route_items, module);    
  }
  else
  {
    
  }
  
  qbus_route_send_updates (self, conn);  
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
  }
}

//-----------------------------------------------------------------------------

typedef struct
{
  
  CapeString chain_key;
  
  CapeString sender;
  
} QBusForwardData;

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
  //printf ("ROUTE: MESSAGE REQUEST\n");
  
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
          case CAPE_ERR_NONE:   // callback returned to finish / return the chain bus message
          {
            qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
            
            // finally send the frame
            qbus_connection_send (conn, p_frame);
            
            break;
          }
          default:
          {
            qbus_frame_set_type (frame, QBUS_FRAME_TYPE_MSG_RES, self->name);
            
            qbus_frame_set_err (frame, err);
            
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
    printf ("ERROR: forward message can't be returned\n");
  }
  
  cape_str_del (&(qbus_fd->chain_key));
  cape_str_del (&(qbus_fd->sender));
  
  CAPE_DEL(p_qbus_fd, QBusForwardData);
}


//-----------------------------------------------------------------------------

void qbus_route_on_msg_response (QBusRoute self, QBusFrame* p_frame)
{
  QBusFrame frame = *p_frame;
  
  //printf ("ROUTE: MESSAGE RESPONSE\n");
  
  const CapeString chain_key = qbus_frame_get_chainkey (frame);
  
  if (chain_key)
  {
    CapeMapNode n;
   
    //printf ("LOOK FOR KEY: %s in %p\n", chain_key, self->chains);
    
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
          
          qbus_method_call_response (qmeth, self->qbus, frame, err);
          
          cape_err_del (&err);
          
          break;
        }
        case QBUS_METHOD_TYPE__FORWARD:
        {
          QBusForwardData* qbus_fd = qmeth->ptr;
          
          qbus_route_on_msg_forward (self, p_frame, &qbus_fd);
          
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
  printf ("no route to module %s\n", module);
  
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

void qbus_route_conn_request (QBusRoute self, QBusConnection const conn, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  // create a new frame
  QBusFrame frame = qbus_frame_new ();
  
  {
    CapeString h = cape_str_uuid();
    
    // add default content
    qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_REQ, h, module, method, self->name);
    
    // add message content
    qbus_frame_set_qmsg (frame, msg);
    
    cape_mutex_lock (self->chain_mutex);
    
    {
      QBusMethod qmeth = qbus_method_new (QBUS_METHOD_TYPE__RESPONSE, ptr, onMsg, NULL);
      
      cape_map_insert (self->chains, (void*)h, (void*)qmeth);
      
      //printf ("CHAIN ADD: %s in %p\n", h, self->chains);
    }
    
    cape_mutex_unlock (self->chain_mutex);
  }
  
  // finally send the frame
  qbus_connection_send (conn, &frame);
}

//-----------------------------------------------------------------------------

void qbus_route_request (QBusRoute self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  QBusConnection const conn = qbus_route_module_find (self, module);
  
  if (conn)
  {
    qbus_route_conn_request (self, conn, module, method, msg, ptr, onMsg);
  }
  else
  {
    qbus_route_no_route (self, module, method, msg, ptr, onMsg);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_response (QBusRoute self, const char* module, QBusM msg)
{
  QBusConnection const conn = qbus_route_module_find (self, module);
  
  if (conn)
  {
    // create a new frame
    QBusFrame frame = qbus_frame_new ();

    // add default content
    qbus_frame_set (frame, QBUS_FRAME_TYPE_MSG_RES, msg->chain_key, module, NULL, self->name);
    
    // add message content
    qbus_frame_set_qmsg (frame, msg);
    
    // finally send the frame
    qbus_connection_send (conn, &frame);
  }
  else
  {
    
  }
}

//-----------------------------------------------------------------------------
