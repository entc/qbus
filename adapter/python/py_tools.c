#include "py_tools.h"

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
