cmake_minimum_required(VERSION 2.4)

# abstract operation-system layer
INCLUDE_DIRECTORIES("../../../cape/src" "../../core" "../../src")

set(ENGINE_TCP_SOURCES
  engine_tcp.c
)

set(ENGINE_TCP_HEADERS
  engine_tcp.h
)

add_library             (qbus_engine_tcp STATIC ${ENGINE_TCP_SOURCES} ${ENGINE_TCP_HEADERS})
target_link_libraries   (qbus_engine_tcp qbus_core cape)
set_target_properties   (qbus_engine_tcp PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)
