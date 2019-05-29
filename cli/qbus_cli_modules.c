#include "qbus_cli_modules.h"

// cape includes
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusCliModules_s
{
  SCREEN* screen;     // reference
  
  QBus qbus;          // reference
  
  void* obj;
};

//-----------------------------------------------------------------------------

QBusCliModules qbus_cli_modules_new (QBus qbus, SCREEN* screen)
{
  QBusCliModules self = CAPE_NEW (struct QBusCliModules_s);

  // assign curses reference
  self->screen = screen;
  
  self->qbus = qbus;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_del (QBusCliModules* p_self)
{
  
  
  CAPE_DEL(p_self, struct QBusCliModules_s);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_cli_modules_on_change (QBus qbus, void* ptr, const CapeUdc modules)
{
  QBusCliModules self = ptr;
  
  clear();
  
  //debug
  if (modules)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (modules, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
      
      mvprintw (cursor->position, 0, cape_udc_s (cursor->item, "[****]"));
      
      
      
      
      
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  refresh();  
}

//-----------------------------------------------------------------------------

int qbus_cli_modules_init (QBusCliModules self, CapeErr err)
{
  noecho();
  
  refresh ();
  
  
  
  self->obj = qbus_add_on_change (self->qbus, self, qbus_cli_modules_on_change);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
