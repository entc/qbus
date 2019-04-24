cmake_minimum_required(VERSION 2.4)

# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../core" ".")

set(WSRV_SOURCES
  qbus.c
)

set(WSRV_HEADERS
  qbus.h
)

add_library             (qbus SHARED ${WSRV_SOURCES} ${WSRV_HEADERS})
target_link_libraries   (qbus qbus_core qbus_engine_tcp cape)
set_target_properties   (qbus PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

install (TARGETS qbus DESTINATION lib)
