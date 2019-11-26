#include "qbus.h"

#include <stdlib.h>

//-----------------------------------------------------------------------------

static int __STDCALL test_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  res = CAPE_ERR_NONE;
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  // register methods
  qbus_register (qbus, "test_method", NULL, test_method, NULL, err);
  
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
  qbus_instance ("TEST", NULL, app_on_init, app_on_done, argc, argv);
  
  return 0;
}

//-----------------------------------------------------------------------------
