cmake_minimum_required(VERSION 2.4)

find_package(Java REQUIRED)

# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../cape/src" "../../src" ${JAVA_INCLUDES})

set(ADAPTER_JAVA_SOURCES
  java_adapter.c
)

set(ADAPTER_JAVA_HEADERS
  java_adapter.h
)

add_library             (qbus_adapter_java SHARED ${ADAPTER_JAVA_SOURCES} ${ADAPTER_JAVA_HEADERS})
target_link_libraries   (qbus_adapter_java cape qbus)
set_target_properties   (qbus_adapter_java PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

