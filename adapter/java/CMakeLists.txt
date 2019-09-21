cmake_minimum_required(VERSION 2.4)

find_package(Java COMPONENTS Development QUIET)
find_package(JNI)

IF(JAVA_FOUND)


message(STATUS "Java Include Dirs = ${JNI_INCLUDE_DIRS}")

# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../cape/src" "../../src" ${JNI_INCLUDE_DIRS})

set(ADAPTER_JAVA_SOURCES
  java_adapter.c
)

set(ADAPTER_JAVA_HEADERS
  java_adapter.h
)

add_library             (qbus_adapter_java SHARED ${ADAPTER_JAVA_SOURCES} ${ADAPTER_JAVA_HEADERS})
target_link_libraries   (qbus_adapter_java cape qbus)
set_target_properties   (qbus_adapter_java PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

INSTALL (TARGETS qbus_adapter_java DESTINATION lib)

ENDIF()
