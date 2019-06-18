#include "qbus_cli_requests.h"

#include <adbl.h>

//-----------------------------------------------------------------------------

struct QBusCliRequests_s
{
  SCREEN* screen;     // reference
  
  QBus qbus;          // reference
  
  WINDOW* requests_window;
  
  AdblCtx adbl;
  
  AdblSession session;
};

//-----------------------------------------------------------------------------

QBusCliRequests qbus_cli_requests_new (QBus qbus, SCREEN* screen)
{
  QBusCliRequests self = CAPE_NEW (struct QBusCliRequests_s);
  
  self->qbus = qbus;
  
  self->requests_window = newwin (20, 50, 0, 35);
  
  self->adbl = NULL;
  self->session = NULL;
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_cli_requests_del (QBusCliRequests* p_self)
{
  
}

//-----------------------------------------------------------------------------

int qbus_cli_requests_init (QBusCliRequests self, CapeErr err)
{
  self->adbl = adbl_ctx_new ("adbl", "adbl2_sqlite3", err);
  
  if (self->adbl == NULL)
  {
    return cape_err_code (err);
  }

  self->session = adbl_session_open_file (self->adbl, "adbl_default.json", err);
  
  if (self->session == NULL)
  {
    return cape_err_code (err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_cli_requests_refresh (QBusCliRequests self, const char* method)
{
  CapeErr err = cape_err_new ();
  CapeUdc results = NULL;
  
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // params
  cape_udc_add_s_cp     (params, "method"     , method);
  // values
  cape_udc_add_s_cp     (values, "payload"    , NULL);
  
  results = adbl_session_query (self->session, "requests", &params, &values, err);
  if (results == NULL)
  {
    goto exit_and_cleanup;
  }
  
  // display the results
  
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    
    
  }

  cape_udc_del (&results);  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------
