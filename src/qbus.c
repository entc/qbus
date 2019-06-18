#include "qbus.h" 
#include "qbus_route.h"

// c includes
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_types.h"
#include "sys/cape_file.h"
#include "stc/cape_str.h"
#include "aio/cape_aio_sock.h"
#include "fmt/cape_args.h"
#include "fmt/cape_json.h"
#include "fmt/cape_tokenizer.h"

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
  
  // config
  CapeUdc config;
  
  CapeString config_file;
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
  
  self->config = NULL;
  self->config_file = NULL;
  
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
  
  cape_udc_del (&(self->config));
  cape_str_del (&(self->config_file));
  
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
          cape_log_fmt (CAPE_LL_ERROR, "QBUS", "add income", "error in listen: %s", cape_err_text (err));
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
          cape_log_fmt (CAPE_LL_ERROR, "QBUS", "add remote", "error in connect: %s", cape_err_text (err));
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

int qbus_wait__intern (QBus self, CapeUdc binds, CapeUdc remotes, CapeErr err)
{
  int res;
  
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
  
  return res;
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
  
  return qbus_wait__intern (self, binds, remotes, err);
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
  qbus_route_request (self->route, module, method, msg, ptr, onMsg, FALSE);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_continue (QBus self, const char* module, const char* method, QBusM qin, void** p_ptr, fct_qbus_onMessage on_msg, CapeErr err)
{
  qbus_route_request (self->route, module, method, qin, *p_ptr, on_msg, TRUE);
  
  *p_ptr = NULL;
  
  return CAPE_ERR_CONTINUE;
}

//-----------------------------------------------------------------------------

int qbus_response (QBus self, const char* module, QBusM msg, CapeErr err)
{
  qbus_route_response (self->route, module, msg, NULL);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

const CapeString qbus_name (QBus self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

CapeAioContext qbus_aio (QBus self)
{
  return self->aio;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_modules (QBus self)
{
  return qbus_route_modules (self->route);
}

//-----------------------------------------------------------------------------

QBusConnection const qbus_find_conn (QBus self, const char* module)
{
  return qbus_route_module_find (self->route, module);
}

//-----------------------------------------------------------------------------

void qbus_conn_request (QBus self, QBusConnection const conn, const char* module, const char* method, QBusM msg, void* ptr, fct_qbus_onMessage onMsg)
{
  qbus_route_conn_request (self->route, conn, module, method, msg, ptr, onMsg, FALSE);
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
  self->rinfo = NULL;
  self->files = NULL;
  
  self->err = NULL;
  
  self->mtype = QBUS_MTYPE_NONE;
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_message_clr (QBusM self)
{
  cape_udc_del (&(self->cdata));
  cape_udc_del (&(self->clist));
  
  cape_err_del (&(self->err));
}

//-----------------------------------------------------------------------------

void qbus_message_del (QBusM* p_self)
{
  QBusM self = *p_self;
  
  qbus_message_clr (self);

  // only clear it here
  cape_udc_del (&(self->rinfo));
  cape_udc_del (&(self->files));

  cape_str_del (&(self->chain_key));
  cape_str_del (&(self->sender));
  
  CAPE_DEL (p_self, struct QBusMessage_s);
}

//-----------------------------------------------------------------------------

void qbus_check_param (CapeUdc data, const CapeUdc param)
{
  const CapeString h = cape_udc_s (param, NULL);
  if (h)
  {
    CapeList options = cape_tokenizer_buf (h, strlen(h), ':');
    
    if (cape_list_size(options) == 3)
    {
      CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      CapeListCursor* cursor = cape_list_cursor_create (options, CAPE_DIRECTION_FORW);
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_s_cp (n, "type", cape_list_node_data (cursor->node));
      }
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_s_cp (n, "host", cape_list_node_data (cursor->node));
      }
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_n (n, "port", strtol (cape_list_node_data (cursor->node), NULL, 10));
      }
      
      cape_list_cursor_destroy (&cursor);
      
      cape_udc_add (data, &n);
    }
    
    cape_list_del (&options);
  }
}

//-----------------------------------------------------------------------------

void qbus_config_load__find_file (QBus self, CapeErr err)
{
  CapeString current_folder = cape_fs_path_current (NULL);
  CapeString filename = cape_str_catenate_2 (self->name, ".json");
  
  CapeDirCursor dc = cape_dc_new (current_folder, err);
  
  if (dc)
  {
    while (cape_dc_next (dc))
    {
      if (cape_str_compare (filename, cape_dc_name (dc)))
      {
        self->config_file = cape_fs_path_current (cape_dc_name (dc));

        cape_log_fmt (CAPE_LL_TRACE, self->name, "config file", "use config: %s", self->config_file);
      }
    }
    
    cape_dc_del (&dc);
  }
  
  /*
  if (self->config_file == NULL)
  {
    self->config_file = cape_fs_path_current (filename);

    cape_log_fmt (CAPE_LL_TRACE, self->name, "config file", "create config: %s", self->config_file);      
  }
  */

  cape_str_del (&filename);
  cape_str_del (&current_folder);
  
}

//-----------------------------------------------------------------------------

void qbus_config_load (QBus self)
{
  CapeErr err = cape_err_new ();

  // create or find the config file
  qbus_config_load__find_file (self, err);
  
  if (self->config_file)
  {
    // try to load
    self->config = cape_json_from_file (self->config_file, err);
  }
  
  if (self->config == NULL)
  {
    self->config = cape_udc_new (CAPE_UDC_NODE, NULL);
  }

  if (cape_err_code (err))
  {
    // dump error text
    cape_log_msg (CAPE_LL_ERROR, self->name, "config load", cape_err_text (err));
  }
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qbus_config_save (QBus self)
{
  // try to load
  if (self->config && cape_udc_size (self->config))
  {
    int res;
    CapeErr err = cape_err_new ();
    
    res = cape_json_to_file (self->config_file, self->config, err);
    if (res)
    {
      // dump error text
      cape_log_msg (CAPE_LL_ERROR, self->name, "config save", cape_err_text (err));      
    }
    
    cape_err_del (&err);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_config_s (QBus self, const char* name, const CapeString default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_s (config_node, default_val);
  }
  else
  {
    cape_udc_add_s_cp (self->config, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

number_t qbus_config_n (QBus self, const char* name, number_t default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_n (config_node, default_val);
  }
  else
  {    
    cape_udc_add_n (self->config, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

double qbus_config_f (QBus self, const char* name, double default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_f (config_node, default_val);
  }
  else
  {    
    cape_udc_add_f (self->config, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------

int qbus_config_b (QBus self, const char* name, int default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_b (config_node, default_val);
  }
  else
  {    
    cape_udc_add_b (self->config, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------

void qbus_instance (const char* name, void* ptr, fct_qbus_on_init on_init, fct_qbus_on_done on_done, int argc, char *argv[])
{
  int res = CAPE_ERR_NONE;
  CapeErr err = cape_err_new ();
  CapeFileLog log = NULL;

  QBus qbus = qbus_new (name);

  CapeUdc bind = NULL;
  CapeUdc remotes = NULL;
  CapeUdc params = NULL;
  
  void* user_ptr= NULL;
  
  printf ("    ooooooo  oooooooooo ooooo  oooo oooooooo8  \n");
  printf ("  o888   888o 888    888 888    88 888         \n");
  printf ("  888     888 888oooo88  888    88  888oooooo  \n");
  printf ("  888o  8o888 888    888 888    88         888 \n");
  printf ("    88ooo88  o888ooo888   888oo88  o88oooo888  \n");
  printf ("         88o8                                  \n");
  printf ("\n");

  // load config
  qbus_config_load (qbus);
  
  // create params
  params = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // convert program arguments into a node with parameters 
  CapeUdc args = cape_args_from_args (argc, argv, NULL);
  if (args)
  {
    cape_udc_merge_mv (params, &args);  
  }
  
  if (qbus->config)
  {
    cape_udc_merge_cp (params, qbus->config);  
  }
  
  // filelogging
  {
    CapeUdc arg_l = cape_udc_get (params, "l");
    if (arg_l)
    {
      log = cape_log_new (cape_udc_s (arg_l, NULL));
    }
  }
  
  // debug
  {
    CapeString h = cape_json_to_s (params);
    
    cape_log_fmt (CAPE_LL_INFO, name, "qbus_instance", "params: %s", h);
    
    cape_str_del (&h);
  }
  
  // check for remotes
  {
    CapeUdc arg_r = cape_udc_get (params, "d");
    if (arg_r)
    {
      remotes = cape_udc_new (CAPE_UDC_LIST, NULL);
      
      qbus_check_param (remotes, arg_r);
    }
  }
      
  // check for binds
  {
    CapeUdc arg_b = cape_udc_get (params, "b");
    if (arg_b)
    {
      bind = cape_udc_new (CAPE_UDC_LIST, NULL);
      
      qbus_check_param (bind, arg_b);
    }
  }
      
  cape_log_msg (CAPE_LL_TRACE, name, "qbus_instance", "arguments parsed");
  
  // open the operating system AIO/event subsystem
  res = cape_aio_context_open (qbus->aio, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "instance", "error in initialization: %s", cape_err_text(err));    
    goto exit_and_cleanup;
  }
  
  if (on_init)
  {
    res = on_init (qbus, ptr, &user_ptr, err);
  }

  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "QBUS", "instance", "error in initialization: %s", cape_err_text(err));    
    goto exit_and_cleanup;
  }
  
  // save the config back
  qbus_config_save (qbus);
  
  cape_log_msg (CAPE_LL_TRACE, name, "qbus_instance", "start main loop");

  // disable console echoing
  {
    struct termios saved;
    struct termios attributes;
    
    tcgetattr(STDIN_FILENO, &saved);
    tcgetattr(STDIN_FILENO, &attributes);
    
    attributes.c_lflag &= ~ ECHO;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attributes);    

    // *** main loop ***
    qbus_wait__intern (qbus, bind, remotes, err);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
  }
  
  if (on_done)
  {
    res = on_done (qbus, user_ptr, err);
  }
  
exit_and_cleanup:
  
  cape_udc_del (&bind);
  cape_udc_del (&remotes);
  cape_udc_del (&params);
  
  qbus_del (&qbus);
  
  cape_err_del (&err);
  cape_log_del (&log);
}

//-----------------------------------------------------------------------------

void* qbus_add_on_change (QBus self, void* ptr, fct_qbus_on_route_change on_change)
{
  return qbus_route_add_on_change (self->route, ptr, on_change);
}

//-----------------------------------------------------------------------------

void qbus_rm_on_change (QBus self, void* obj)
{
  qbus_route_rm_on_change (self->route, obj);
}

//-----------------------------------------------------------------------------

void qbus_methods (QBus self, const char* module, void* ptr, fct_qbus_on_methods on_methods)
{
  qbus_route_methods (self->route, module, ptr, on_methods);
}

//-----------------------------------------------------------------------------
