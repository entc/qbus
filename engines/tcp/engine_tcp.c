#include "engine_tcp.h"

// cape includes
#include "sys/cape_socket.h"
#include "aio/cape_aio_timer.h"

// qbus core
#include "qbus_core.h"

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_tcp_send (void* ptr1, void* ptr2, const char* bufdat, ssize_t buflen, void* userdata)
{
  cape_aio_socket_send (ptr2, ptr1, bufdat, buflen, userdata);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_tcp_mark (void* ptr1, void* ptr2)
{
  cape_aio_socket_markSent (ptr2, ptr1);
}

//-----------------------------------------------------------------------------

struct EngineTcpInc_s
{
  CapeString host;
  
  number_t port;
  
  CapeAioContext aio;   // reference
  
  QBusRoute route;      // reference
  
};

//-----------------------------------------------------------------------------

EngineTcpInc qbus_engine_tcp_inc_new (CapeAioContext aio, QBusRoute route, const CapeString host, number_t port)
{
  EngineTcpInc self = CAPE_NEW (struct EngineTcpInc_s);
  
  self->host = cape_str_cp (host);
  self->port = port;

  self->aio = aio;
  self->route = route;
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_tcp_inc_del (EngineTcpInc* p_self)
{
  if (*p_self)
  {
    EngineTcpInc self = *p_self;
    
    cape_str_del (&(self->host));
    
    CAPE_DEL(p_self, struct EngineTcpInc_s);
  }  
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_engine_tcp_inc_onSent (void* ptr, CapeAioSocket socket, void* userdata)
{
  qbus_connection_onSent (ptr, userdata);
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_engine_tcp_inc_onRecv (void* ptr, CapeAioSocket socket, const char* bufdat, ssize_t buflen)
{
  qbus_connection_onRecv (ptr, bufdat, buflen);
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_engine_tcp_inc_onDone (void* ptr, void* userdata)
{
  QBusConnection qbus_connection = ptr;
  
  qbus_connection_del (&qbus_connection);
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_engine_tcp_inc_onConnect (void* ptr, void* handle, const char* remote_addr)
{
  EngineTcpInc self = ptr;
  
  printf ("connection from %s\n", remote_addr);
  
  if (handle == NULL)
  {
    return;
  }

  // handle new connection
  {
    // create a new core connection for routing
    QBusConnection qbus_connection = qbus_connection_new (self->route, 0);

    // create a new handler for the created socket
    CapeAioSocket sock = cape_aio_socket_new (handle);
    
    // set qbus connection callbacks
    qbus_connection_cb (qbus_connection, self->aio, sock, qbus_engine_tcp_send, qbus_engine_tcp_mark);
    
    // set callback
    cape_aio_socket_callback (sock, qbus_connection, qbus_engine_tcp_inc_onSent, qbus_engine_tcp_inc_onRecv, qbus_engine_tcp_inc_onDone);
        
    // listen on the connection  
    cape_aio_socket_listen (&sock, self->aio);

    // activate routing
    qbus_connection_reg (qbus_connection);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_engine_tcp_inc_onAcceptDone (void* ptr)
{
  printf ("stopped listen\n");
}

//-----------------------------------------------------------------------------

int qbus_engine_tcp_inc_listen (EngineTcpInc self, CapeErr err)
{
  void* socket_handle = cape_sock_acceptor_new (self->host, self->port, err);
  
  if (socket_handle == NULL)
  {
    return cape_err_code (err);
  }
  
  {
    // create a new acceptor context for the AIO subsystem
    CapeAioAccept accept_event_handler = cape_aio_accept_new (socket_handle);
    
    // set callbacks
    cape_aio_accept_callback (accept_event_handler, self, qbus_engine_tcp_inc_onConnect, qbus_engine_tcp_inc_onAcceptDone);
    
    // register at AIO
    cape_aio_accept_add (&accept_event_handler, self->aio);
  }
  
  return CAPE_ERR_NONE;
}

//=============================================================================

struct EngineTcpOut_s
{
  CapeString host;
  
  number_t port;
  
  CapeAioContext aio;   // reference
  
  QBusRoute route;      // reference
};

//-----------------------------------------------------------------------------

EngineTcpOut qbus_engine_tcp_out_new (CapeAioContext aio, QBusRoute route, const CapeString host, number_t port)
{
  EngineTcpOut self = CAPE_NEW (struct EngineTcpOut_s);
  
  self->host = cape_str_cp (host);
  self->port = port;
  
  self->aio = aio;
  self->route = route;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_tcp_out_del (EngineTcpOut* p_self)
{
  if (*p_self)
  {
    EngineTcpOut self = *p_self;
    
    cape_str_del (&(self->host));
    
    CAPE_DEL(p_self, struct EngineTcpOut_s);
  }  
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_engine_tcp_out_timer__onTimer (void* ptr)
{
  CapeErr err = cape_err_new ();

  qbus_engine_tcp_out_reconnect (ptr, err);

  cape_err_del (&err);

  // remove the timer
  return FALSE;
}

//-----------------------------------------------------------------------------

void qbus_engine_tcp_out_timer_enable (EngineTcpOut self)
{
  int res;
  CapeErr err = cape_err_new ();
  
  CapeAioTimer timer = cape_aio_timer_new ();
  
  res = cape_aio_timer_set (timer, 10000, self, qbus_engine_tcp_out_timer__onTimer, err);   // create timer with 10 seconds
  if (res)
  {
    // throw std::runtime_error (cape_err_text (err));
  }
  
  res = cape_aio_timer_add (&timer, self->aio);
  if (res)
  {
    // throw std::runtime_error (cape_err_text (err));
  }
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

typedef struct
{
  QBusConnection conn;
  
  EngineTcpOut eout;
  
} QBusEngineTcpOutCtx;

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_tcp_out_onSent (void* ptr, CapeAioSocket socket, void* userdata)
{
  QBusEngineTcpOutCtx* ctx = ptr;
  
  qbus_connection_onSent (ctx->conn, userdata);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_tcp_out_onRecv (void* ptr, CapeAioSocket socket, const char* bufdat, ssize_t buflen)
{
  QBusEngineTcpOutCtx* ctx = ptr;

  qbus_connection_onRecv (ctx->conn, bufdat, buflen);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engine_tcp_out_onDone (void* ptr, void* userdata)
{
  QBusEngineTcpOutCtx* ctx = ptr;

  qbus_connection_del (&(ctx->conn));

  // add timer for reconnection
  qbus_engine_tcp_out_timer_enable (ctx->eout);
  
  // we can cleanup the context
  CAPE_DEL (&ctx, QBusEngineTcpOutCtx);
}

//-----------------------------------------------------------------------------

int qbus_engine_tcp_out_reconnect (EngineTcpOut self, CapeErr err)
{
  void* sock = cape_sock_reader_new (self->host, self->port, err);
  if (sock == NULL)
  {
    return cape_err_code (err);
  }
  
  // handle new connection
  {
    // create a new core connection for routing
    QBusConnection qbus_connection = qbus_connection_new (self->route, 0);
    
    CapeAioSocket s = cape_aio_socket_new (sock);
    // set callbacks
    // we can use the event handler directly
    
    {
      QBusEngineTcpOutCtx* ctx = CAPE_NEW(QBusEngineTcpOutCtx);
      
      ctx->conn = qbus_connection;
      ctx->eout = self;
      
      cape_aio_socket_callback (s, ctx, qbus_engine_tcp_out_onSent, qbus_engine_tcp_out_onRecv, qbus_engine_tcp_out_onDone);
    }    
    
    // set qbus connection callbacks
    qbus_connection_cb (qbus_connection, self->aio, s, qbus_engine_tcp_send, qbus_engine_tcp_mark);
    
    cape_aio_socket_listen (&s, self->aio);

    // activate routing
    qbus_connection_reg (qbus_connection);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
