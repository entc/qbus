#include "qbus.h"

#include <stdlib.h>

int main (int argc, char *argv[])
{
  CapeErr err = cape_err_new ();
  
  QBus qbus = qbus_new ("TEST");

  {
    CapeUdc bind = NULL;
    CapeUdc remotes = NULL;
    
    if (argc == 3)
    {      
      remotes = cape_udc_new (CAPE_UDC_LIST, NULL);
      
      CapeUdc client = cape_udc_new (CAPE_UDC_NODE, NULL);

      cape_udc_add_s_cp (client, "type", "socket");
      cape_udc_add_s_cp (client, "host", argv[1]);
      cape_udc_add_n    (client, "port", strtol(argv[2], NULL, 10));
      
      cape_udc_add (remotes, &client);
    }
    else
    {
      bind = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_s_cp (bind, "type", "socket");
      cape_udc_add_s_cp (bind, "host", "127.0.0.1");
      cape_udc_add_n    (bind, "port", 8090);
    }
    
    qbus_wait (qbus, bind, remotes, err);
    
    cape_udc_del (&bind);
    cape_udc_del (&remotes);
  }
  
  
  qbus_del (&qbus);
  
  cape_err_del (&err);
}
