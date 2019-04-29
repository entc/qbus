#include "py_tools.h"
#include "py_qbus.h"

// c includes
#include <stdio.h>

// cape includes
#include "stc/cape_udc.h"
#include "fmt/cape_json.h"
#include "sys/cape_log.h"

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

  // create a new object
  PyObject_QBus* qbus_pyobj = (PyObject_QBus*)PyObject_CallObject((PyObject*)&PyTypeObject_QBusIntern, NULL);

  // assign object
  qbus_pyobj->qbus = qbus;
  
  {
    PyObject* arglist = Py_BuildValue ("(O)", qbus_pyobj);
    
    ctx->obj = PyEval_CallObject (ctx->on_init, arglist);
  
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "py adapter", "return object: %p", ctx->obj);

    Py_DECREF(arglist);
  }

  Py_DECREF(qbus_pyobj);
  
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

  // create a new object
  PyObject_QBus* qbus_pyobj = (PyObject_QBus*)PyObject_CallObject((PyObject*)&PyTypeObject_QBusIntern, NULL);
  
  // assign object
  qbus_pyobj->qbus = qbus;
  
  PyObject* result;
  
  {
    PyObject* arglist = Py_BuildValue ("(OO)", qbus_pyobj, ctx->obj);
  
    result = PyEval_CallObject (ctx->on_done, arglist);
    
    Py_DECREF (arglist);
  }
  
  if (result)
  {
    Py_DECREF (result);
  }
  else
  {
    // some error happened, tell python
    PyErr_Print();
    
    // we need to clean, otherwise it will crash at some point
    PyErr_Clear();    
  }

  Py_DECREF(qbus_pyobj);
  
  Py_DECREF (ctx->on_init);
  Py_DECREF (ctx->on_done);
  Py_DECREF (ctx->obj);
  
  CAPE_DEL(&ctx, PyInstanceContext);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static PyObject* py_qbus_instance (PyObject* self, PyObject* args, PyObject* kwds)
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

static PyObject* py_qbus_log (PyObject* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;
  
  
  
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
  {"log",       (PyCFunction)py_qbus_log, METH_VARARGS, "log something"},
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
  
  if (PyType_Ready (&PyTypeObject_QBus) < 0)
  {
    return NULL;
  }
    
  // this is important, otherwise it will crash
  if (PyType_Ready (&PyTypeObject_QBusIntern) < 0)
  {
    return NULL;
  }
    
  m = PyModule_Create (&moduledef);
  if (m == NULL)
  {
    return NULL;
  }
    
  Py_INCREF(&PyTypeObject_QBus);
  PyModule_AddObject(m, "QBus", (PyObject *) &PyTypeObject_QBus);

  return m;
}

//-----------------------------------------------------------------------------
