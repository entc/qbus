#include "qbus.h"

#include <stdlib.h>

//-----------------------------------------------------------------------------

static int __STDCALL app_on_init (QBus qbus, void** p_ptr, CapeErr err)
{

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_done (QBus qbus, void* ptr, CapeErr err)
{
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("TEST", app_on_init, app_on_done, argc, argv);
  
  return 0;
}

//-----------------------------------------------------------------------------
