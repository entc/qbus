// python includes
#include <Python.h>
#include "structmember.h"

// c includes
#include <stdio.h>

// qbus
#include "qbus.h"

// cape includes
#include "stc/cape_udc.h"
#include "fmt/cape_json.h"

//-----------------------------------------------------------------------------

typedef struct 
{
  PyObject_HEAD;
  
  QBus qbus;
  
} QBusObject;

//-----------------------------------------------------------------------------

static PyObject* py_qbus_new (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  QBusObject* self;
  
  self = (QBusObject*)type->tp_alloc(type, 0);
  
  if (self != NULL)
  {
    self->qbus = qbus_new ("PYMOD");
  }
  
  return (PyObject*) self;
}

//-----------------------------------------------------------------------------

static void py_qbus_del (QBusObject* self)
{
  qbus_del (&(self->qbus));
  
  Py_TYPE(self)->tp_free((PyObject *) self);
}

//-----------------------------------------------------------------------------

// forward declaration
CapeUdc py_transform_to_udc (PyObject* o);

//-----------------------------------------------------------------------------

CapeUdc py_transform_to_udc_node (PyObject* o)
{
  CapeUdc ret = NULL;
  
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  
  ret = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // step through the dictonary
  while (PyDict_Next (o, &pos, &key, &value)) 
  {
    CapeUdc h;
    const char* name = NULL;
    
    if (PyUnicode_Check (key))
    {
      name = PyUnicode_AsUTF8(key);
    }
    else
    {
      name = "unknown";
    }
    
    h = py_transform_to_udc (value);
    if (h)
    {
      cape_udc_add_name (ret, &h, name);
    }
    else
    {
      printf ("can't add '%s' python dict\n", name);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc py_transform_to_udc_list (PyObject* o)
{
  CapeUdc ret = NULL;

  Py_ssize_t i;
  Py_ssize_t n = PyList_Size (o);
  
  ret = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  for (i = 0; i < n; i++)
  {
    CapeUdc h;
    PyObject* val = PyList_GetItem (o, i);
    
    h = py_transform_to_udc (val);
    if (h)
    {
      cape_udc_add (ret, &h);
    }
    else
    {
      
    }
  }
    
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc py_transform_to_udc_tuple (PyObject* o)
{
  CapeUdc ret = NULL;
  
  Py_ssize_t i;
  Py_ssize_t n = PyTuple_Size (o);
  
  ret = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  for (i = 0; i < n; i++)
  {
    CapeUdc h;
    PyObject* val = PyTuple_GetItem (o, i);
    
    h = py_transform_to_udc (val);
    if (h)
    {
      cape_udc_add (ret, &h);
    }
    else
    {
      
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc py_transform_to_udc (PyObject* o)
{
  CapeUdc ret = NULL;
  
  if (Py_None == o)
  {
    // nothing to do here  
  }
  else if (PyDict_Check (o))
  {
    ret = py_transform_to_udc_node (o);
  }
  else if (PyList_Check (o))
  {
    ret = py_transform_to_udc_list (o);
  }
  else if (PyTuple_Check (o))
  {
    ret = py_transform_to_udc_tuple (o);
  }
  else if (PySlice_Check (o))
  {
    printf ("TODO PYTHON OBJECT SLICE\n");
    // TODO
    
  }
  else if (PySet_Check (o))
  {
    printf ("TODO PYTHON OBJECT SET\n");
    // TODO
    
  }
  else if (PyUnicode_Check (o))
  {
    ret = cape_udc_new (CAPE_UDC_STRING, NULL);
    
    cape_udc_set_s_cp (ret, PyUnicode_AsUTF8 (o));
  }
  else if (PyLong_Check (o))
  {
    ret = cape_udc_new (CAPE_UDC_NUMBER, NULL);
    
    cape_udc_set_n (ret, PyLong_AsLong (o));
  }
  else if (PyFloat_Check (o))
  {
    ret = cape_udc_new (CAPE_UDC_FLOAT, NULL);
    
    cape_udc_set_f (ret, PyFloat_AsDouble (o));
  }
  else if (PyBool_Check (o))
  {
    ret = cape_udc_new (CAPE_UDC_BOOL, NULL);
 
    // there is no dedicated function to get the value
    // Python provides a special check for true / false
    cape_udc_set_b (ret, o == Py_True);
  }
  else
  {
    printf ("UNKNOWN PYTHON OBJECT\n");
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

PyObject* py_transform_to_pyo (CapeUdc o)
{
  PyObject* ret = Py_None;
  
  switch (cape_udc_type(o))
  {
    case CAPE_UDC_NODE:
    {
      ret = PyDict_New ();
     
      CapeUdcCursor* cursor = cape_udc_cursor_new (o, CAPE_DIRECTION_FORW);
      
      while (cape_udc_cursor_next (cursor))
      {
        CapeUdc item = cursor->item;
        
        PyObject* h = py_transform_to_pyo (item);
        PyObject* n = PyUnicode_FromString (cape_udc_name(item));
        
        PyDict_SetItem (ret, n, h);   
        
        Py_DECREF (h);
        Py_DECREF (n);
      }
      
      cape_udc_cursor_del (&cursor);
      
      break;
    }
    case CAPE_UDC_STRING:
    {
      ret = PyUnicode_FromString (cape_udc_s (o, ""));
      break;      
    }
    case CAPE_UDC_NUMBER:
    {
      ret = PyLong_FromLong (cape_udc_n (o, 0));
      break;
    }
  }

  return ret;
}  

//-----------------------------------------------------------------------------

static PyObject* py_qbus_instance (QBusObject* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;

  
  
  return ret;
}

//-----------------------------------------------------------------------------

static PyObject* py_qbus_wait (QBusObject* self, PyObject* args, PyObject* kwds)
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

int __STDCALL onMessage (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  PythonCallbackData* pcd = ptr;
  
  PyObject* py_qin = Py_None;
  
  if (qin->cdata)
  {
    py_qin = py_transform_to_pyo (qin->cdata);
  }
  
  PyObject* arglist = Py_BuildValue ("(O)", py_qin);
  
  PyObject* result = PyEval_CallObject (pcd->fct, arglist);

  if (result)
  {
    qout->cdata = py_transform_to_udc (result);
    qout->mtype = QBUS_MTYPE_JSON;
  }
  else
  {
    
  }
    
  Py_XDECREF (result);
  Py_DECREF (arglist);
  
  Py_XDECREF (py_qin);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static void __STDCALL onRemoved (void* ptr)
{
  PythonCallbackData* pcd = ptr;

  Py_DECREF (pcd->fct);
  
  CAPE_DEL(&pcd, PythonCallbackData);
}

//-----------------------------------------------------------------------------

static PyObject* py_qbus_register (QBusObject* self, PyObject* args, PyObject* kwds)
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
  
  PythonCallbackData* pcd = CAPE_NEW (PythonCallbackData);
  
  pcd->fct = cbfct;
  
  {
    int res = qbus_register (self->qbus, PyUnicode_AsUTF8 (name), pcd, onMessage, onRemoved, err);
    if (res)
    {
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
  
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

static PyMethodDef py_qbus_methods[] = 
{
  {"instance",    (PyCFunction)py_qbus_instance,    METH_VARARGS, "instance"},
  {"wait",        (PyCFunction)py_qbus_wait,        METH_VARARGS, "wait"},
  {"register",    (PyCFunction)py_qbus_register,    METH_VARARGS, "register a callback method"},
  {NULL}
};

//-----------------------------------------------------------------------------

static PyMemberDef py_qbus_members[] = 
{
  {NULL}
};

//-----------------------------------------------------------------------------

static PyTypeObject QBusType = 
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "qbus.QBus",
  .tp_doc = "QBus objects",
  .tp_basicsize = sizeof(QBusObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = py_qbus_new,
  .tp_init = (initproc) NULL,
  .tp_dealloc = (destructor) py_qbus_del,
  .tp_members = py_qbus_members,
  .tp_methods = py_qbus_methods,
};

//-----------------------------------------------------------------------------

struct module_state
{
  PyObject *error;
};

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

//-----------------------------------------------------------------------------

static PyMethodDef module_methods[] = 
{
  {NULL, NULL}
};

//-----------------------------------------------------------------------------

static int module_traverse (PyObject* m, visitproc visit, void* arg)
{ 
  struct module_state *st = GETSTATE(m);

  Py_VISIT(st->error);
  return 0;
}

//-----------------------------------------------------------------------------

static int module_clear (PyObject* m)
{
  struct module_state *st = GETSTATE(m);

  Py_CLEAR(st->error);
  return 0;
}

//-----------------------------------------------------------------------------

static struct PyModuleDef moduledef = 
{
  PyModuleDef_HEAD_INIT,
  "qbus",
  NULL,
  sizeof(struct module_state),
  module_methods,
  NULL,
  module_traverse,
  module_clear,
  NULL
};

//-----------------------------------------------------------------------------

PyMODINIT_FUNC PyInit_qbus (void)
{
  PyObject *m;
  
  if (PyType_Ready(&QBusType) < 0)
  {
    return NULL;
  }
    
  m = PyModule_Create(&moduledef);
  if (m == NULL)
  {
    return NULL;
  }
    
  Py_INCREF(&QBusType);
  PyModule_AddObject(m, "QBus", (PyObject *) &QBusType);
  
  return m;
}

//-----------------------------------------------------------------------------
