#include "qbus_cli_methods.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_thread.h>

//-----------------------------------------------------------------------------

struct QBusCliMethods_s
{
  QBus qbus;          // reference
  
  void* obj;
  
  WINDOW* methods_window;

  int menu_position;
  int menu_max;
  
  CapeUdc methods;
};

//-----------------------------------------------------------------------------

QBusCliMethods qbus_cli_methods_new (QBus qbus, SCREEN* screen)
{
  QBusCliMethods self = CAPE_NEW (struct QBusCliMethods_s);
  
  self->qbus = qbus;
  
  self->methods_window = newwin (20, 50, 0, 35);
  
  //self->curses_thread = cape_thread_new ();
  
  self->menu_position = 0;
  self->menu_max = 0;
  
  self->methods = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_cli_methods_del (QBusCliMethods* p_self)
{
  QBusCliMethods self = *p_self;
  
  cape_udc_del (&(self->methods));
  
  CAPE_DEL(p_self, struct QBusCliMethods_s);
}

//-----------------------------------------------------------------------------

void qbus_cli_methods_clear (QBusCliMethods self)
{
  wclear(self->methods_window);
  
  wborder(self->methods_window, '|', '|', '_', '_', ' ', ' ', '|', '|');
  mvwprintw (self->methods_window, 0, 0, "[ methods ]");
  
  wrefresh (self->methods_window);
}

//-----------------------------------------------------------------------------

int qbus_cli_methods_init (QBusCliMethods self, CapeErr err)
{
  noecho();
  
  curs_set(0);
  
  keypad(self->methods_window, TRUE);
  
  qbus_cli_methods_clear (self); 
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_cli_methods_stdin (QBusCliMethods self, const char* bufdat, number_t buflen)
{
  
}

//-----------------------------------------------------------------------------

void qbus_cli_methods_show (QBusCliMethods self)
{
  if (self->methods)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (self->methods, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      
      
      mvwprintw (self->methods_window, cursor->position + 2, 5, cape_udc_s (cursor->item, "none"));
    }
    
    cape_udc_cursor_del (&cursor);
  }
  else
  {
    
    
  }
  
  wrefresh (self->methods_window);
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_cli_methods_on_methods (QBus qbus, void* ptr, const CapeUdc methods, CapeErr err)
{
  QBusCliMethods self = ptr;
  
  qbus_cli_methods_clear (self);

  cape_udc_del (&(self->methods));
  
  if (cape_err_code (err))
  {
    
  }
  else if (methods)
  {
    self->methods = cape_udc_cp (methods);
  }

  qbus_cli_methods_show (self);    
}

//-----------------------------------------------------------------------------

void qbus_cli_methods_set (QBusCliMethods self, const CapeString module)
{
  CapeString h = cape_str_fmt ("[ %s ]", module);
  
  mvwprintw (self->methods_window, 0, 20, h);
  
  cape_str_del(&h);
  
  wrefresh (self->methods_window); 
  
  qbus_methods (self->qbus, module, self, qbus_cli_methods_on_methods);
}

//-----------------------------------------------------------------------------
