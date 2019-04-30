#include "py_qbus.h"
#include "py_tools.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_new (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  PyObject_QBus* self = (PyObject_QBus*)type->tp_alloc(type, 0);

  self->qbus = NULL;
  
  return (PyObject*) self;
}

//-----------------------------------------------------------------------------

void py_object_qbus_del (PyObject_QBus* self)
{
  if (self->qbus)
  {
    qbus_del (&(self->qbus));
  }
  
  Py_TYPE(self)->tp_free((PyObject *) self);
}

//-----------------------------------------------------------------------------

int py_object_qbus_init (PyObject_QBus* self, PyObject *args, PyObject *kwds)
{
  PyObject* name;
  
  if (!PyArg_ParseTuple (args, "O", &name))
  {
    return -1;
  }  

  if (!PyUnicode_Check (name))
  {
    return -1;
  }

  // create a new qbus object
  self->qbus = qbus_new (PyUnicode_AsUTF8(name));
  
  return 0;
}

//-----------------------------------------------------------------------------

void py_object_qbus_del_e (PyObject_QBus* self)
{
  Py_TYPE(self)->tp_free((PyObject*)self);
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_wait (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;
  
  CapeUdc cape_arg1 = NULL;
  CapeUdc cape_arg2 = NULL;
  
  PyObject* arg1;
  PyObject* arg2;
  
  CapeErr err = cape_err_new ();
  
  if (!PyArg_ParseTuple (args, "OO", &arg1, &arg2))
  {
    return NULL;
  }
  
  if (arg1)
  {
    cape_arg1 = py_transform_to_udc (arg1);
  }
  
  if (arg2)
  {
    cape_arg2 = py_transform_to_udc (arg2);
  }
  
  if ((NULL == cape_arg1) && (NULL == cape_arg2))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid input parameters");
    goto exit_and_error;
  }
  
  if (cape_arg1)
  {
    CapeString h = cape_json_to_s (cape_arg1);
    
    printf ("B: %s\n", h);
    
    cape_str_del (&h);
  }
  
  if (cape_arg2)
  {
    CapeString h = cape_json_to_s (cape_arg2);
    
    printf ("R: %s\n", h);
    
    cape_str_del (&h);
  }
  
  {
    int res = qbus_wait (self->qbus, cape_arg1, cape_arg2, err);
    if (res)
    {
      printf ("ERROR: %s\n", cape_err_text (err));
      
      goto exit_and_error;
    }
  }
  
exit_and_error:
  
  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));
    
    // tell python an error ocoured
    ret = NULL;
  }
  
  cape_udc_del (&cape_arg1);
  cape_udc_del (&cape_arg2);
  
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

typedef struct {
  
  PyObject* fct;
  
} PythonCallbackData;

//-----------------------------------------------------------------------------

int __STDCALL py_object_qbus_register__on_message (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  PythonCallbackData* pcd = ptr;
  
  PyObject* arglist;
  PyObject* result;
  
  PyObject* py_qin = Py_None;
  
  if (qin->cdata)
  {
    py_qin = py_transform_to_pyo (qin->cdata);
  }

  arglist = Py_BuildValue ("(O)", py_qin);
  
  result = PyEval_CallObject (pcd->fct, arglist);
  if (result)
  {
    qout->cdata = py_transform_to_udc (result);
    qout->mtype = QBUS_MTYPE_JSON;

    Py_XDECREF (result);
  }
  else
  {
    // some error happened, tell python
    PyErr_Print();

    // we need to clean, otherwise it will crash at some point
    PyErr_Clear();
  }

  // cleanup
  Py_XDECREF (py_qin);
  Py_DECREF (arglist);
    
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static void __STDCALL py_object_qbus_register__on_removed (void* ptr)
{
  PythonCallbackData* pcd = ptr;
  
  Py_DECREF (pcd->fct);
  
  CAPE_DEL(&pcd, PythonCallbackData);
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_register (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;
  
  CapeErr err = cape_err_new ();
  
  PyObject* name;
  PyObject* cbfct;
  
  if (!PyArg_ParseTuple (args, "OO", &name, &cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }
  
  if (!PyUnicode_Check (name))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a string");
    goto exit_and_error;
  }
  
  if (!PyCallable_Check (cbfct)) 
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter is not a callback");
    goto exit_and_error;
  }
  
  {
    PythonCallbackData* pcd = CAPE_NEW (PythonCallbackData);
    
    pcd->fct = cbfct;
    
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "py adapter", "register callback %s", PyUnicode_AsUTF8 (name));
    
    {
      int res = qbus_register (self->qbus, PyUnicode_AsUTF8 (name), pcd, py_object_qbus_register__on_message, py_object_qbus_register__on_removed, err);
      if (res)
      {
        goto exit_and_error;
      }
    }
  }
  
exit_and_error:
  
  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));
    
    // tell python an error occoured
    ret = NULL;
  }
  
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_config (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;
  
  CapeErr err = cape_err_new ();
  
  PyObject* name;
  PyObject* default_val;
  
  if (!PyArg_ParseTuple (args, "OO", &name, &default_val))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }

  if (!PyUnicode_Check (name))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a string");
    goto exit_and_error;
  }
  
  if (PyUnicode_Check (default_val))
  {
    const CapeString h = qbus_config_s (self->qbus, PyUnicode_AsUTF8 (name), PyUnicode_AsUTF8 (default_val));

    ret = PyUnicode_FromString (h);
  }
  else if (PyLong_Check (default_val))
  {
    number_t h = qbus_config_n (self->qbus, PyUnicode_AsUTF8 (name), PyLong_AsLong (default_val));
    
    ret = PyLong_FromLong (h);
  }
  else if (PyFloat_Check (default_val))
  {
    double h = qbus_config_f (self->qbus, PyUnicode_AsUTF8 (name), PyFloat_AsDouble (default_val));
    
    ret = PyFloat_FromDouble (h);
  }
  else if (PyBool_Check (default_val))
  {
    int h = qbus_config_b (self->qbus, PyUnicode_AsUTF8 (name), default_val == Py_True);
    
    ret = PyBool_FromLong (h);
  }
  else
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter has not supported type");
    goto exit_and_error;    
  }
  
exit_and_error:
  
  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));
    
    // tell python an error occoured
    ret = NULL;
  }
  
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------
