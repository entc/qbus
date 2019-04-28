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
#include "sys/cape_log.h"

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

static PyObject* py_qbus_new_static (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  QBusObject* self;
  
  printf ("INIT STATIC\n");
  
  self = (QBusObject*)type->tp_alloc(type, 0);
  
  self->qbus = NULL;
  
  return (PyObject*) self;
}

//-----------------------------------------------------------------------------

static void py_qbus_del (QBusObject* self)
{
  qbus_del (&(self->qbus));
  
  Py_TYPE(self)->tp_free((PyObject *) self);
}

//-----------------------------------------------------------------------------

static void py_qbus_del_static (QBusObject* self)
{
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
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "py adapter", "register callback %s", PyUnicode_AsUTF8 (name));

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

static PyTypeObject QBusType_Dynamic = 
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

static PyTypeObject QBusType_Static = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "qbus.QBus_s",                              /*tp_name*/
  sizeof(QBusObject),                         /*tp_basicsize*/
  0,                                          /*tp_itemsize*/
  (destructor)py_qbus_del_static,                 /*tp_dealloc*/
  0,                                          /*tp_print*/
  0,                                          /*tp_getattr*/
  0,                                          /*tp_setattr*/
  0,                                          /*tp_compare*/
  0,                                          /*tp_repr*/
  0,                                          /*tp_as_number*/
  0,                                          /*tp_as_sequence*/
  0,                                          /*tp_as_mapping*/
  0,                                          /*tp_hash */
  0,                                          /*tp_call*/
  0,                                          /*tp_str*/
  0,                                          /*tp_getattro*/
  0,                                          /*tp_setattro*/
  0,                                          /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,                         /*tp_flags*/
  0,                                          /*tp_doc*/
  0,              /*tp_traverse*/
  0,                      /*tp_clear*/
  0,                                          /*tp_richcompare*/
  0,                                          /*tp_weaklistoffset*/
  0,                                          /*tp_iter*/
  0,                                          /*tp_iternext*/
  py_qbus_methods,                                          /*tp_methods*/
  0,                                /*tp_members*/
  0,                                          /*tp_getsets*/
  0,                                          /*tp_base*/
  0,                                          /*tp_dict*/
  0,                                          /*tp_descr_get*/
  0,                                          /*tp_descr_set*/
  0,                                          /*tp_dictoffset*/
  0,                      /*tp_init*/
  0,                                          /*tp_alloc*/
  py_qbus_new_static,                                 /*tp_new*/
};

//-----------------------------------------------------------------------------

typedef struct {
  
  PyObject* on_init;

  PyObject* on_done;
  
  PyObject* obj;
  
} PyInstanceContext;

//-----------------------------------------------------------------------------

static int __STDCALL py_qbus_instance__on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  PyInstanceContext* ctx = ptr;
  
  // use the same ptr
  *p_ptr = ctx;
  
  QBusObject* obj = PyObject_New (QBusObject, &QBusType_Static);

  // assign object
  obj->qbus = qbus;
  
  {
    PyObject* arglist = Py_BuildValue ("(O)", obj);
    
    ctx->obj = PyEval_CallObject (ctx->on_init, arglist);
  
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "py adapter", "return object: %p", ctx->obj);

    Py_DECREF(arglist);
  }

  if (ctx->obj == NULL)
  {
    return cape_err_set (err, CAPE_ERR_RUNTIME, "runtime error");
  }
  else
  {
    return CAPE_ERR_NONE;
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL py_qbus_instance__on_done (QBus qbus, void* ptr, CapeErr err)
{
  PyInstanceContext* ctx = ptr;

  PyObject* arglist = Py_BuildValue ("(O)", ctx->obj);
  
  PyObject* result = PyEval_CallObject (ctx->on_done, arglist);

  Py_DECREF (ctx->on_init);
  Py_DECREF (ctx->on_done);
  Py_DECREF (ctx->obj);
  
  Py_DECREF (result);

  CAPE_DEL(&ctx, PyInstanceContext);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static PyObject* py_qbus_instance (QBusObject* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;
  
  CapeErr err = cape_err_new ();
  
  PyObject* name;
  PyObject* on_init;
  PyObject* on_done;
  
  Py_ssize_t argc;
  const char** argv = NULL;
  
  if (!PyArg_ParseTuple (args, "OOO", &name, &on_init, &on_done))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }
  
  if (!PyUnicode_Check (name))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a string");
    goto exit_and_error;
  }
  
  if (!PyCallable_Check (on_init)) 
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter is not a callback");
    goto exit_and_error;
  }
  
  if (!PyCallable_Check (on_done)) 
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "3. parameter is not a callback");
    goto exit_and_error;
  }
  
  PyObject* sys_module = PyImport_AddModule("sys");
  if (sys_module == NULL)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't load 'sys' module");
    goto exit_and_error;
  }
  
  PyObject* py_argv = PyObject_GetAttrString (sys_module, "argv");
  if (py_argv == NULL)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't get 'argv' from sys module");
    goto exit_and_error;
  }
  
  if (!PyList_Check (py_argv))
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "'argv' is not a list");
    goto exit_and_error;
  }
  
  argc = PyList_Size (py_argv);
  argv = CAPE_ALLOC (argc * sizeof(char*));

  // convert all arguments into argv
  {
    Py_ssize_t i;
    
    for (i = 0; i < argc; i++)
    {
      PyObject* arg = PyList_GetItem (py_argv, i);
      
      if (PyUnicode_Check (arg))
      {
        // this is not a copy
        argv[i] = PyUnicode_AsUTF8 (arg);
      }
    }
  }
  
  {
    PyInstanceContext* ctx = CAPE_NEW (PyInstanceContext);
    
    ctx->on_init = on_init;
    ctx->on_done = on_done;
    ctx->obj = NULL;
    
    qbus_instance (PyUnicode_AsUTF8 (name), ctx, py_qbus_instance__on_init, py_qbus_instance__on_done, argc, (char**)argv);
  }
  
  CAPE_FREE(argv);

  Py_DECREF (name);
  
  Py_DECREF (py_argv);
  Py_DECREF (sys_module);
  
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

struct module_state
{
  PyObject *error;
};

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

//-----------------------------------------------------------------------------

static PyMethodDef module_methods[] = 
{
  {"instance",  (PyCFunction)py_qbus_instance, METH_VARARGS, "run a QBUS instance"},
  {NULL}
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
  
  if (PyType_Ready (&QBusType_Dynamic) < 0)
  {
    return NULL;
  }
    
  m = PyModule_Create (&moduledef);
  if (m == NULL)
  {
    return NULL;
  }
    
  Py_INCREF(&QBusType_Dynamic);
  Py_INCREF(&QBusType_Static);
  
  PyModule_AddObject(m, "QBus", (PyObject *) &QBusType_Dynamic);
  PyModule_AddObject(m, "QBus_s", (PyObject *) &QBusType_Static);

  return m;
}

//-----------------------------------------------------------------------------
