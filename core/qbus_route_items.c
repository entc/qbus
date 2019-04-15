#include "qbus_route_items.h"

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_mutex.h>
#include <stc/cape_map.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusRouteItems_s
{
  CapeMutex mutex;
  
  CapeMap routes_direct;

  CapeMap routes_node;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_routes_direct_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_routes_nodes_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusRouteItems qbus_route_items_new (void)
{
  QBusRouteItems self = CAPE_NEW (struct QBusRouteItems_s);
  
  self->routes_direct = cape_map_new (NULL, qbus_route_routes_direct_del, NULL);
  self->routes_node = cape_map_new (NULL, qbus_route_routes_nodes_del, NULL);
  
  self->mutex = cape_mutex_new ();

  return self;  
}

//-----------------------------------------------------------------------------

void qbus_route_items_del (QBusRouteItems* p_self)
{
  QBusRouteItems self = *p_self;
  
  cape_map_del (&(self->routes_direct));
  cape_map_del (&(self->routes_node));

  cape_mutex_del (&(self->mutex));

  CAPE_DEL (p_self, struct QBusRouteItems_s);  
}

//-----------------------------------------------------------------------------

void qbus_route_items_node_add (QBusRouteItems self, QBusConnection conn, CapeUdc item)
{
  switch (cape_udc_type (item))
  {
    case CAPE_UDC_STRING:
    {
      const CapeString remote_module = cape_udc_s (item, NULL);
      if (remote_module)
      {
        CapeString h = cape_str_cp (remote_module);
        
        cape_str_to_upper (h);
        
        printf ("ROUTE NODES: insert %s -> %p\n", h, conn);
        
        cape_map_insert (self->routes_node, (void*)h, (void*)conn);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_items_nodes_add (QBusRouteItems self, QBusConnection conn, CapeUdc nodes)
{
  switch (cape_udc_type (nodes))
  {
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (nodes, CAPE_DIRECTION_FORW);

      while (cape_udc_cursor_next (cursor))
      {
        qbus_route_items_node_add (self, conn, cursor->item);
      }
      
      cape_udc_cursor_del (&cursor);
      
      break;
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_items_nodes_remove_all (QBusRouteItems self, QBusConnection conn)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->routes_node, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    QBusConnection conn_remote = cape_map_node_value (cursor->node);
    
    if (conn_remote == conn)
    {
      printf ("ROUTE NODES: remove %s\n", (CapeString)cape_map_node_key (cursor->node));
      
      cape_map_cursor_erase (self->routes_node, cursor);      
    }
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

void qbus_route_items_add (QBusRouteItems self, const CapeString module_origin, QBusConnection conn, CapeUdc* p_nodes)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeString module = cape_str_cp (module_origin);
    
    cape_str_to_upper (module);
    
    cape_map_insert (self->routes_direct, (void*)module, (void*)conn);
    
    qbus_connection_set (conn, module);
  }
  
  if (*p_nodes)
  {
    qbus_route_items_nodes_add (self, conn, *p_nodes);
    
    cape_udc_del (p_nodes);    
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

void qbus_route_items_update (QBusRouteItems self, const CapeString module_origin, CapeUdc* p_nodes)
{
  CapeString module = cape_str_cp (module_origin);
  
  cape_str_to_upper (module);

  cape_mutex_lock (self->mutex);
  
  // direct
  {
    CapeMapNode n = cape_map_find (self->routes_direct, (void*)module);
    if (n)
    {
      QBusConnection conn = cape_map_node_value (n);
      
      qbus_route_items_nodes_remove_all (self, conn);
      
      if (*p_nodes)
      {
        qbus_route_items_nodes_add (self, conn, *p_nodes);
      }
    }
  }
  
  cape_str_del (&module);
  
  cape_mutex_unlock (self->mutex);
  
  cape_udc_del (p_nodes);
}

//-----------------------------------------------------------------------------

QBusConnection qbus_route_items_get (QBusRouteItems self, const CapeString module_origin)
{
  QBusConnection ret = NULL;
  
  CapeString module = cape_str_cp (module_origin);
  
  cape_str_to_upper (module);
  
  cape_mutex_lock (self->mutex);

  // direct
  {
    CapeMapNode n = cape_map_find (self->routes_direct, (void*)module);
    if (n)
    {
      ret = cape_map_node_value (n);
      
      goto exit_and_cleanup;
    }
  }
  // node
  {
    CapeMapNode n = cape_map_find (self->routes_node, (void*)module);    
    if (n)
    {
      ret = cape_map_node_value (n);
      
      goto exit_and_cleanup;
    }
  }
  
  
exit_and_cleanup:

  cape_str_del (&module);

  cape_mutex_unlock (self->mutex);

  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route_items_rm (QBusRouteItems self, const CapeString module)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapNode n = cape_map_find (self->routes_direct, (void*)module);
    
    if (n)
    {
      QBusConnection conn = cape_map_node_value (n);
      
      // remove all indirect nodes with this connection
      qbus_route_items_nodes_remove_all (self, conn);

      printf ("qbus connection removed: %s\n", module);
      
      // remove the connection
      cape_map_erase (self->routes_direct, n);
    }      
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_items_nodes (QBusRouteItems self)
{
  CapeUdc nodes = cape_udc_new (CAPE_UDC_LIST, NULL);
 
  cape_mutex_lock (self->mutex);

  // iterate through all direct connections
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routes_direct, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      cape_udc_add_s_cp (nodes, NULL, cape_map_node_key (cursor->node));      
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  // iterate through all node connections
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routes_node, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      cape_udc_add_s_cp (nodes, NULL, cape_map_node_key (cursor->node));      
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);

  return nodes;
}

//-----------------------------------------------------------------------------

CapeList qbus_route_items_conns (QBusRouteItems self, QBusConnection exception)
{
  CapeList conns = cape_list_new (NULL);
  
  cape_mutex_lock (self->mutex);
  
  // iterate through all direct connections
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routes_direct, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusConnection conn = cape_map_node_value (cursor->node);
      
      if (conn != exception)
      {
        cape_list_push_back (conns, (void*)conn);
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);
  
  return conns;
}

//-----------------------------------------------------------------------------

