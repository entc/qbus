#include "qbus_cli_modules.h"

// cape includes
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusCliModules_s
{
  SCREEN* screen;     // reference
  
  QBus qbus;          // reference
  
  void* obj;
  
  WINDOW* modules_window;
};

//-----------------------------------------------------------------------------

QBusCliModules qbus_cli_modules_new (QBus qbus, SCREEN* screen)
{
  QBusCliModules self = CAPE_NEW (struct QBusCliModules_s);

  // assign curses reference
  self->screen = screen;
  
  self->qbus = qbus;
  
  self->modules_window = newwin (20, 34, 0, 0);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_del (QBusCliModules* p_self)
{
  QBusCliModules self = *p_self;
  
  qbus_rm_on_change (self->qbus, self->obj);
  
  CAPE_DEL(p_self, struct QBusCliModules_s);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_cli_modules_on_change (QBus qbus, void* ptr, const CapeUdc modules)
{
  QBusCliModules self = ptr;
  
  clear();
  
  
  wborder(self->modules_window, '|', '|', '_', '_', ' ', ' ', '|', '|');
  mvwprintw (self->modules_window, 0, 0, "[ modules ]");
  
  //debug
  if (modules)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (modules, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
      mvwprintw (self->modules_window, cursor->position + 2, 2, cape_udc_s (cursor->item, "[****]"));
      
      
      
      
      
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  wrefresh (self->modules_window);  
}

//-----------------------------------------------------------------------------

int qbus_cli_modules_init (QBusCliModules self, CapeErr err)
{  
  //noecho();
  
  refresh ();
  
  
  
  self->obj = qbus_add_on_change (self->qbus, self, qbus_cli_modules_on_change);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_stdin  (QBusCliModules self, const char* bufdat, number_t buflen)
{
  if (buflen > 0)
  {
    switch (bufdat[0])
    {
      case 27:
      {
        if (buflen > 1)
        {
          switch (bufdat[1])
          {
            
            
            default:
            {
              CapeString h = cape_str_fmt ("[ %i | %i ]", buflen, bufdat[0]);
              
              mvwprintw (self->modules_window, 0, 20, h);
              
              cape_str_del(&h);
            }
          }
        }
       
        break;
      }
      default:
      {
        CapeString h = cape_str_fmt ("[ %i | %i ]", buflen, bufdat[0]);
        
        mvwprintw (self->modules_window, 0, 20, h);
        
        cape_str_del(&h);
      }
    }
  }

  wrefresh (self->modules_window);  
}

//-----------------------------------------------------------------------------
