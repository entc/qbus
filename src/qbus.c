#include "qbus.h" 
#include "qbus_route.h"

// c includes
#include <signal.h>

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_types.h"
#include "stc/cape_str.h"
#include "aio/cape_aio_sock.h"
#include "fmt/cape_args.h"

// engines
#include "../engines/tcp/engine_tcp.h"

//-----------------------------------------------------------------------------

struct QBus_s
{
  CapeAioContext aio;
  
  CapeString name;
  
  QBusRoute route;
  
  // TODO -> into list
  EngineTcpInc engine_tcp_inc;
  
  // TODO -> into list
  EngineTcpOut engine_tcp_out;
};

//-----------------------------------------------------------------------------

QBus qbus_new (const char* module)
{
  QBus self = CAPE_NEW(struct QBus_s);

  self->route = qbus_route_new (self, module);
  
  self->aio = cape_aio_context_new ();
  
  self->name = cape_str_cp (module);
  
  self->engine_tcp_inc = NULL;
  self->engine_tcp_out = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_del (QBus* p_self)
{
  QBus self = *p_self;
  
  cape_aio_context_del (&(self->aio));
  
  cape_str_del (&(self->name));
  
  qbus_engine_tcp_inc_del (&(self->engine_tcp_inc));
  qbus_engine_tcp_out_del (&(self->engine_tcp_out));
  
  qbus_route_del (&(self->route));
  
  CAPE_DEL (p_self, struct QBus_s);
}

//-----------------------------------------------------------------------------

void qbus_add_income_port (QBus self, CapeUdc bind)
{
  const CapeString type = cape_udc_get_s (bind, "type", NULL);
  
  if (type == NULL)
  {
    return;
  }
  
  if (strcmp (type, "pipe") == 0)
  {
    // check if we have name and path
    const CapeString name = cape_udc_get_s (bind, "name", NULL);
    const CapeString path = cape_udc_get_s (bind, "path", NULL);
    
    if (name && path)
    {
      
    }
    
    return;
  }
  
  if (strcmp (type, "socket") == 0)
  {
    // check if we have host and port
    const CapeString host = cape_udc_get_s (bind, "host", NULL);
    number_t port = cape_udc_get_n (bind, "port", 0);
    
    if (host && port)
    {
      self->engine_tcp_inc = qbus_engine_tcp_inc_new (self->aio, self->route, host, port);

      // power up engine
      {
        CapeErr err = cape_err_new ();
        
        int res = qbus_engine_tcp_inc_listen (self->engine_tcp_inc, err);
        if (res)
        {          
          printf ("ERROR %i: %s\n", res, cape_err_text (err));
        }
        
        cape_err_del (&err);
      }
    }
    
    return;
  }
}

//-----------------------------------------------------------------------------

void qbus_add_remote_port (QBus self, CapeUdc remote)
{
  const CapeString type = cape_udc_get_s (remote, "type", NULL);
  
  if (type == NULL)
  {
    return;
  }
  
  if (strcmp (type, "pipe") == 0)
  {
    // check if we have name and path
    const CapeString name = cape_udc_get_s (remote, "name", NULL);
    const CapeString path = cape_udc_get_s (remote, "path", NULL);
    
    if (name && path)
    {
      
    }
    
    return;
  }
  
  if (strcmp (type, "socket") == 0)
  {
    // check if we have host and port
    const CapeString host = cape_udc_get_s (remote, "host", NULL);
    number_t port = cape_udc_get_n (remote, "port", 0);
    
    if (host && port)
    {
      self->engine_tcp_out = qbus_engine_tcp_out_new (self->aio, self->route, host, port);
      
      // power up engine
      {
        CapeErr err = cape_err_new ();
        
        int res = qbus_engine_tcp_out_reconnect (self->engine_tcp_out, err);
        if (res)
        {          
          printf ("ERROR %i: %s\n", res, cape_err_text (err));
        }
        
        cape_err_del (&err);
      }
    }
    
    return;
  }  
}

//-----------------------------------------------------------------------------

void qbus_add_income_ports (QBus self, CapeUdc binds)
{
  switch (cape_udc_type (binds))
  {
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (binds, CAPE_DIRECTION_FORW);
      
      while (cape_udc_cursor_next (cursor))
      {
        qbus_add_income_port (self, cursor->item);
      }
      
      cape_udc_cursor_del (&cursor);
      
      break;
    }
    case CAPE_UDC_NODE:
    {
      qbus_add_income_port (self, binds);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_add_remote_ports (QBus self, CapeUdc remotes)
{
  switch (cape_udc_type (remotes))
  {
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (remotes, CAPE_DIRECTION_FORW);
      
      while (cape_udc_cursor_next (cursor))
      {
        qbus_add_remote_port (self, cursor->item);
      }
      
      cape_udc_cursor_del (&cursor);
      
      break;
    }
    case CAPE_UDC_NODE:
    {
      qbus_add_remote_port (self, remotes);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_wait (QBus self, CapeUdc binds, CapeUdc remotes, CapeErr err)
{
  int res;
  
  // open the operating system AIO/event subsystem
  res = cape_aio_context_open (self->aio, err);
  if (res)
  {
    return res;
  }
  
  // activate signal handling strategy
  res = cape_aio_context_set_interupts (self->aio, TRUE, TRUE, err);
  if (res)
  {
    return res;
  }
  
  if (binds)
  {
    qbus_add_income_ports (self, binds);
  }
  
  if (remotes)
  {
    qbus_add_remote_ports (self, remotes);
  }
  
  // TODO: run in several threads
  
  // wait infinite and let the AIO subsystem handle all events
  res = cape_aio_context_wait (self->aio, err);
  
  printf ("Goodbye, have a nice day!\n");
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_register (QBus self, const char* method, void* ptr, fct_qbus_onMessage onMsg, fct_qbus_onRemoved onRm, CapeErr err)
{
  qbus_route_meth_reg (self->route, method, ptr, onMsg, onRm);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_send (QBus self, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg, CapeErr err)
{  
  qbus_route_request (self->route, module, method, msg, ptr, onMsg);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_response (QBus self, const char* module, QBusM msg, CapeErr err)
{
  qbus_route_response (self->route, module, msg);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

const CapeString qbus_name (QBus self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

QBusM qbus_message_new (const CapeString key, const CapeString sender)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);
  
  if (key)
  {
    // clone the key
    self->chain_key = cape_str_cp (key);
  }
  else
  {
    // create a new key
    self->chain_key = cape_str_uuid ();    
  }
  
  self->sender = cape_str_cp (sender);
  
  // init the objects
  self->cdata = NULL;
  self->clist = NULL;
  self->uinfo = NULL;
  
  self->err = NULL;
  
  self->mtype = QBUS_MTYPE_NONE;
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_message_del (QBusM* p_self)
{
  QBusM self = *p_self;
  
  cape_str_del (&(self->chain_key));
  cape_str_del (&(self->sender));
  
  cape_udc_del (&(self->cdata));
  cape_udc_del (&(self->clist));
  cape_udc_del (&(self->uinfo));
  
  cape_err_del (&(self->err));
  
  CAPE_DEL (p_self, struct QBusMessage_s);
}

//-----------------------------------------------------------------------------

void qbus_instance (const char* name, fct_qbus_on_init on_init, fct_qbus_on_done on_done, int argc, char *argv[])
{
  int res = CAPE_ERR_NONE;
  CapeErr err = cape_err_new ();

  QBus qbus = qbus_new (name);

  CapeUdc bind = NULL;
  CapeUdc remotes = NULL;
  
  void* user_ptr= NULL;
  
  printf ("    ooooooo  oooooooooo ooooo  oooo oooooooo8  \n");
  printf ("  o888   888o 888    888 888    88 888         \n");
  printf ("  888     888 888oooo88  888    88  888oooooo  \n");
  printf ("  888o  8o888 888    888 888    88         888 \n");
  printf ("    88ooo88  o888ooo888   888oo88  o88oooo888  \n");
  printf ("         88o8                                  \n");
  printf ("\n");
  
  cape_log_msg (CAPE_LL_INFO, name, "qbus_instance", "starting up");
  
  // convert program arguments into a node with parameters 
  CapeUdc params = cape_args_from_args (argc, argv, NULL);
  if (params)
  {
    
    
    
  }
  
  cape_udc_del (&params);
  
  
  if (argc == 3)
  {      
    remotes = cape_udc_new (CAPE_UDC_LIST, NULL);
    
    CapeUdc client = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_cp (client, "type", "socket");
    cape_udc_add_s_cp (client, "host", argv[1]);
    cape_udc_add_n    (client, "port", strtol(argv[2], NULL, 10));
    
    cape_udc_add (remotes, &client);
  }
  else
  {
    bind = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_cp (bind, "type", "socket");
    cape_udc_add_s_cp (bind, "host", "127.0.0.1");
    cape_udc_add_n    (bind, "port", 33390);
  }

  cape_log_msg (CAPE_LL_TRACE, name, "qbus_instance", "arguments parsed");
  
  if (on_init)
  {
    res = on_init (qbus, &user_ptr, err);
  }

  if (res)
  {
    goto exit_and_cleanup;
  }
  
  cape_log_msg (CAPE_LL_TRACE, name, "qbus_instance", "start main loop");

  // *** main loop ***
  qbus_wait (qbus, bind, remotes, err);

  if (on_done)
  {
    res = on_done (qbus, user_ptr, err);
  }
  
exit_and_cleanup:
  
  cape_udc_del (&bind);
  cape_udc_del (&remotes);
  
  qbus_del (&qbus);
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------
