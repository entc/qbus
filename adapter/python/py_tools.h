#ifndef __QBUS_PY_TOOLS__H
#define __QBUS_PY_TOOLS__H 1

#include "sys/cape_export.h"
#include "stc/cape_udc.h"

// python includes
#include <Python.h>
#include "structmember.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   PyObject*   py_transform_to_pyo     (CapeUdc o);

__CAPE_LIBEX   CapeUdc     py_transform_to_udc     (PyObject* o);

//-----------------------------------------------------------------------------


#endif
