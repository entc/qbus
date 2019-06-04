#include "qbus_cli_modules.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_thread.h>

//-----------------------------------------------------------------------------

struct QBusCliModules_s
{
  SCREEN* screen;     // reference
  
  QBus qbus;          // reference
  
  void* obj;
  
  WINDOW* modules_window;
  
  CapeThread curses_thread;
  
  int menu_position;
  int menu_max;
};

//-----------------------------------------------------------------------------

QBusCliModules qbus_cli_modules_new (QBus qbus, SCREEN* screen)
{
  QBusCliModules self = CAPE_NEW (struct QBusCliModules_s);

  // assign curses reference
  self->screen = screen;
  
  self->qbus = qbus;
  
  self->modules_window = newwin (20, 34, 0, 0);
  
  //self->curses_thread = cape_thread_new ();
  
  self->menu_position = 0;
  self->menu_max = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_del (QBusCliModules* p_self)
{
  QBusCliModules self = *p_self;
  
  qbus_rm_on_change (self->qbus, self->obj);
  
  //cape_thread_join (self->curses_thread);
  
  //cape_thread_del (&(self->curses_thread));
  
  CAPE_DEL(p_self, struct QBusCliModules_s);
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_menu_show (QBusCliModules self)
{
  mvwprintw (self->modules_window, self->menu_position + 2, 2, "[");
  mvwprintw (self->modules_window, self->menu_position + 2, 15, "]");
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_menu_clear (QBusCliModules self)
{
  mvwprintw (self->modules_window, self->menu_position + 2, 2, " ");
  mvwprintw (self->modules_window, self->menu_position + 2, 15, " ");
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_cli_modules_on_change (QBus qbus, void* ptr, const CapeUdc modules)
{
  QBusCliModules self = ptr;
  
  clear();
  
  qbus_cli_modules_menu_clear (self);
  
  
  self->menu_max = 0;
  
  wborder(self->modules_window, '|', '|', '_', '_', ' ', ' ', '|', '|');
  mvwprintw (self->modules_window, 0, 0, "[ modules ]");
  
  //debug
  if (modules)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (modules, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
      mvwprintw (self->modules_window, cursor->position + 2, 5, cape_udc_s (cursor->item, "[****]"));
      
      self->menu_max++;
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  qbus_cli_modules_menu_show (self);

  wrefresh (self->modules_window);  
}

//-----------------------------------------------------------------------------

/*
static int __STDCALL qbus_cli_modules_worker (void* ptr)
{
  QBusCliModules self = ptr;
  
  while( true )
  {
    int c = wgetch (self->modules_window);
    
    CapeString h = cape_str_fmt ("[%i]", c);
    
    mvwprintw (self->modules_window, 0, 20, h);
    
    cape_str_del(&h);

    if (c == KEY_DOWN)
    {
      
      
      cape_aio_context_close (qbus_aio(self->qbus), NULL);
      
      return 0;
    }
  }
  
  printf ("*** WORKER DONE ***\n");
  
  
  return 0;
}
*/

//-----------------------------------------------------------------------------

int qbus_cli_modules_init (QBusCliModules self, CapeErr err)
{  
  noecho();
  
  curs_set(0);
  
  keypad(self->modules_window, TRUE);
  
  self->obj = qbus_add_on_change (self->qbus, self, qbus_cli_modules_on_change);
  
  //cape_thread_start (self->curses_thread, qbus_cli_modules_worker, self);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_menu_up (QBusCliModules self)
{
  if (self->menu_position > 0)
  {
    qbus_cli_modules_menu_clear (self);
    
    self->menu_position--;
    
    qbus_cli_modules_menu_show (self);
  }
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_menu_down (QBusCliModules self)
{
  if (self->menu_position < (self->menu_max - 1))
  {
    qbus_cli_modules_menu_clear (self);

    self->menu_position++;

    qbus_cli_modules_menu_show (self);
  }
}

//-----------------------------------------------------------------------------

void qbus_cli_modules_stdin  (QBusCliModules self, const char* bufdat, number_t buflen)
{
  int state = 0;
  int i;
  
  for (i = 0; i < buflen; i++)
  {
    switch (bufdat[i])
    {
      case 27:
      {
        state++;
        
        break;
      }
      case 79:
      {
        if (state == 1)
        {
          state++;
        }
        
        break;
      }
      case 65:   // up
      {
        if (state == 2)
        {
          CapeString h = cape_str_fmt ("[  UP  ]");
          
          mvwprintw (self->modules_window, 0, 20, h);
          
          cape_str_del(&h);
          
          qbus_cli_modules_menu_up (self);
        }
        
        break;
      }
      case 66:   // down
      {
        if (state == 2)
        {
          CapeString h = cape_str_fmt ("[ DOWN ]");
          
          mvwprintw (self->modules_window, 0, 20, h);
          
          cape_str_del(&h);

          qbus_cli_modules_menu_down (self);
        }
       
        break;
      }
      case 67:   // right
      {
        if (state == 2)
        {
          
        }
        
        break;
      }
      case 68:   // right
      {
        if (state == 2)
        {
          
        }
        
        break;
      }
      default:
      {
        CapeString h = cape_str_fmt ("[ %i | %i ]", i, bufdat[i]);
        
        mvwprintw (self->modules_window, 0, 20, h);
        
        cape_str_del(&h);
      }
    }
  }

  wrefresh (self->modules_window);  
}

//-----------------------------------------------------------------------------
