#include "qbus_core.h"
#include "qbus_frame.h"

// cape includes
#include "stc/cape_list.h"
#include "sys/cape_mutex.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

struct QBusConnection_s
{
  QBusRoute route;    // reference
  
  void* ptr1;
  
  void* ptr2;
  
  fct_qbus_connection_send fct_send;
  
  fct_qbus_connection_mark fct_mark;

  CapeString ident;
  
  // income
  
  QBusFrame frame;

  // out 
  
  CapeList cache_qeue;
  
  CapeMutex mutex;  
};

//-----------------------------------------------------------------------------

static void __STDCALL qbus_connection_cache_onDel (void* ptr)
{
  CapeStream cs = ptr; cape_stream_del (&cs);
}

//-----------------------------------------------------------------------------

QBusConnection qbus_connection_new (QBusRoute route, number_t channels)
{
  QBusConnection self = CAPE_NEW (struct QBusConnection_s);
  
  self->cache_qeue = cape_list_new (qbus_connection_cache_onDel);
  self->mutex = cape_mutex_new (); 
  
  // initial frame
  self->frame = qbus_frame_new ();
  
  self->route = route;
  self->ident = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_connection_del (QBusConnection* p_self)
{
  QBusConnection self = *p_self;
  
  qbus_route_conn_rm (self->route, self);
  
  cape_list_del (&(self->cache_qeue));
  cape_mutex_del (&(self->mutex));
  
  qbus_frame_del (&(self->frame));
  
  cape_str_del (&(self->ident));
  
  CAPE_DEL (p_self, struct QBusConnection_s);
}

//-----------------------------------------------------------------------------

void qbus_connection_cb (QBusConnection self, void* ptr1, void* ptr2, fct_qbus_connection_send send, fct_qbus_connection_mark mark)
{
  self->fct_send = send;
  self->fct_mark = mark;
  
  self->ptr1 = ptr1;
  self->ptr2 = ptr2;
}

//-----------------------------------------------------------------------------

void qbus_connection_reg (QBusConnection self)
{
  qbus_route_conn_reg (self->route, self);
}

//-----------------------------------------------------------------------------

void qbus_connection_set (QBusConnection self, const CapeString ident)
{
  cape_str_replace_cp (&(self->ident), ident);
}

//-----------------------------------------------------------------------------

const CapeString qbus_connection_get (QBusConnection self)
{
  return self->ident;  
}

//-----------------------------------------------------------------------------

void qbus_connection_onSent (QBusConnection self, void* userdata)
{
  CapeStream cs;
  
  if (userdata)
  {
    qbus_connection_cache_onDel (userdata);
  }
  
  cape_mutex_lock (self->mutex);
  
  // extract the first element from the queue cache
  cs = cape_list_pop_front (self->cache_qeue);
  
  cape_mutex_unlock (self->mutex);
  
  if (cs)
  {
    // finally send the buffer content to the unerlaying engine
    self->fct_send (self->ptr1, self->ptr2, cape_stream_data(cs), cape_stream_size(cs), cs);
  }
}

//-----------------------------------------------------------------------------

void qbus_connection_onRecv (QBusConnection self, const char* bufdat, ssize_t buflen)
{
  number_t written = 0;    // how many bytes were processed
  
  // decode the data stream into frames
  while (qbus_frame_decode (self->frame, bufdat + written, buflen - written, &written))
  {
    // call the route method to deliver the frame
    qbus_route_conn_onFrame (self->route, self, &(self->frame));

    // recreate a new frame
    self->frame = qbus_frame_new ();
  }
}

//-----------------------------------------------------------------------------

void qbus_connection_send (QBusConnection self, QBusFrame* p_frame)
{
  // create a new buffer stream
  CapeStream cs = cape_stream_new ();

  // encode (stringify) the frame
  qbus_frame_encode (*p_frame, cs);

  // cleanup the frame  
  qbus_frame_del (p_frame);
  
  // enter monitor
  cape_mutex_lock (self->mutex);
  
  // add the stream buffer to the queue
  cape_list_push_back (self->cache_qeue, (void*)cs);

  // leave monitor
  cape_mutex_unlock (self->mutex);

  // trigger the underlaying engine to process the buffer
  self->fct_mark (self->ptr1, self->ptr2);
}

//-----------------------------------------------------------------------------
