#ifndef __QBUS_ADAPTER_JAVA_H
#define __QBUS_ADAPTER_JAVA_H 1

// JNI includes
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif
  
//-----------------------------------------------------------------------------

JNIEXPORT jlong JNICALL Java_QBus_qbnew       (JNIEnv*, jobject, jstring name);
  
JNIEXPORT void JNICALL Java_QBus_qbdel        (JNIEnv*, jobject, jlong);

JNIEXPORT void JNICALL Java_QBus_qbwait       (JNIEnv*, jobject, jlong, jstring bind, jstring remote);

JNIEXPORT void JNICALL Java_QBus_qbregister   (JNIEnv*, jobject, jlong, jstring method, jobject callback);

JNIEXPORT void JNICALL Java_qbusJNI_send      (JNIEnv*, jobject);

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

