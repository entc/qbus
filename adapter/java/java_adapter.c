#include "java_adapter.h"

#include "qbus.h"

// cape includes
#include <stc/cape_udc.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

jlong JNICALL Java_QBus_qbnew (JNIEnv* env, jobject o, jstring name)
{
  const char* name_text = (*env)->GetStringUTFChars (env, name, 0);
  
  jlong java_void_ptr = (jlong)qbus_new (name_text);
    
  //jclass cls = (*env)->GetObjectClass(env, o);
    
  //printf ("CLS: %p\n", cls);
  
 // jfieldID fid = (*env)->GetStaticFieldID (env, cls, "qbptr", "L");
  
 // printf ("FID: %p\n", fid);

  //(*env)->SetLongField (env, o, fid, java_void_ptr);
  
  return java_void_ptr;
}

//-----------------------------------------------------------------------------

void JNICALL Java_QBus_qbdel (JNIEnv* env, jobject o, jlong ptr)
{
  QBus qbus = (QBus)ptr;
  
  qbus_del (&qbus);
}

//-----------------------------------------------------------------------------

void JNICALL Java_QBus_qbwait (JNIEnv* env, jobject o, jlong ptr, jstring bind, jstring remote)
{
  QBus qbus = (QBus)ptr;

  CapeErr err = cape_err_new ();
  
  const char* bind_text = (*env)->GetStringUTFChars(env, bind, 0);
  const char* remote_text = (*env)->GetStringUTFChars(env, remote, 0);
  
  CapeUdc bind_udc = cape_json_from_s (bind_text);
  CapeUdc remote_udc = cape_json_from_s (remote_text);

  if (bind_udc)
  {
    CapeString h = cape_json_to_s (bind_udc);
    
    printf ("B: %s\n", h);
  }
  if (remote_udc)
  {
    CapeString h = cape_json_to_s (remote_udc);
    
    printf ("R: %s\n", h);
  }
  
  qbus_wait (qbus, bind_udc, remote_udc, err);
  
  cape_udc_del (&bind_udc);
  cape_udc_del (&remote_udc);
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

typedef struct {
  
  jclass clazz;
  
  jmethodID store_method;
  
  JNIEnv* env;
  
  jweak store_Wlistener;
  
} JavaCallbackData;

//-----------------------------------------------------------------------------

int __STDCALL onMessage (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  JavaCallbackData* jcd = ptr;
  
  jstring json_in;
  jstring json_out;
  
  if (qin->cdata)
  {
    CapeString h1 = cape_json_to_s (qin->cdata);
    
    json_in = (*jcd->env)->NewStringUTF (jcd->env, h1);
    
    cape_str_del (&h1);
  }
  else
  {
    json_in = (*jcd->env)->NewStringUTF (jcd->env, "");
  }
  
  // create output java string
  json_out = (*jcd->env)->NewStringUTF (jcd->env, "");
    
  int res = (*jcd->env)->CallIntMethod (jcd->env, jcd->store_Wlistener, jcd->store_method, json_in, json_out);
  
  // get the output string
  const char* json_out_cstring = (*jcd->env)->GetStringUTFChars (jcd->env, json_out, 0);

  // cleanup qout
  cape_udc_del (&(qout->cdata));

  if (!cape_str_empty (json_out_cstring))
  {
    // set the new qout object
    qout->cdata = cape_json_from_s (json_out_cstring);
  }
  
  // tell the garbage collector to free the strings
  (*jcd->env)->DeleteLocalRef (jcd->env, json_out);
  (*jcd->env)->DeleteLocalRef (jcd->env, json_in);
  
  return res;
}

//-----------------------------------------------------------------------------

void __STDCALL onRemoved (void* ptr)
{
  JavaCallbackData* jcd = ptr;

  
  
}

//-----------------------------------------------------------------------------

void JNICALL Java_QBus_qbregister (JNIEnv* env, jobject o, jlong ptr, jstring method, jobject listener)
{
  QBus qbus = (QBus)ptr;
  
  const char* method_text = (*env)->GetStringUTFChars(env, method, 0);
  
  CapeErr err = cape_err_new ();
  
  JavaCallbackData* jcd = CAPE_NEW(JavaCallbackData);
  
  jcd->env = env;
  
  jcd->store_Wlistener = (*env)->NewWeakGlobalRef (env, listener);
  
  jcd->clazz = (*env)->GetObjectClass(env, listener);
  jcd->store_method = (*env)->GetMethodID(env, jcd->clazz, "onAcceptMessage", "(Ljava/lang/String;)V");
  
  int res = qbus_register (qbus, method_text, jcd, onMessage, onRemoved, err);
  if (res)
  {
    
    
  }

  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void JNICALL Java_qbusJNI_send (JNIEnv* env, jobject instance)
{
  
}

//-----------------------------------------------------------------------------
