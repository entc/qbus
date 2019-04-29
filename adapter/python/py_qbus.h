#ifndef __QBUS_PY_QBUS__H
#define __QBUS_PY_QBUS__H 1

#include "sys/cape_export.h"

// python includes
#include <Python.h>
#include "structmember.h"

// qbus
#include "qbus.h"

//-----------------------------------------------------------------------------

typedef struct 
{
  PyObject_HEAD;
  
  QBus qbus;
  
} PyObject_QBus;

//-----------------------------------------------------------------------------

PyObject*   py_object_qbus_new        (PyTypeObject* type, PyObject* args, PyObject* kwds);

void        py_object_qbus_del        (PyObject_QBus*);

int         py_object_qbus_init       (PyObject_QBus*, PyObject *args, PyObject *kwds);

void        py_object_qbus_del_e      (PyObject_QBus*);

//-----------------------------------------------------------------------------

PyObject*   py_object_qbus_wait       (PyObject_QBus*, PyObject* args, PyObject* kwds);

PyObject*   py_object_qbus_register   (PyObject_QBus*, PyObject* args, PyObject* kwds);

//-----------------------------------------------------------------------------

static PyMethodDef PyMethodDef_QBus [] = 
{
  {"wait",        (PyCFunction)py_object_qbus_wait,        METH_VARARGS, "wait"},
  {"register",    (PyCFunction)py_object_qbus_register,    METH_VARARGS, "register a callback method"},
  {NULL}
};

//-----------------------------------------------------------------------------

static PyTypeObject PyTypeObject_QBus = 
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "qbus.QBus",
  .tp_doc = "QBus objects",
  .tp_basicsize = sizeof(PyObject_QBus),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = py_object_qbus_new,
  .tp_init = (initproc) py_object_qbus_init,
  .tp_dealloc = (destructor) py_object_qbus_del,
  .tp_members = NULL,
  .tp_methods = PyMethodDef_QBus,
};

//-----------------------------------------------------------------------------

static PyTypeObject PyTypeObject_QBusIntern = 
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "qbus2.QBus",
  .tp_doc = "QBus objects",
  .tp_basicsize = sizeof(PyObject_QBus),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_new = py_object_qbus_new,
  .tp_init = (initproc) NULL,
  .tp_dealloc = (destructor) py_object_qbus_del_e,
  .tp_members = NULL,
  .tp_methods = PyMethodDef_QBus,
};

//-----------------------------------------------------------------------------

#endif
