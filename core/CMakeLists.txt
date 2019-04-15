cmake_minimum_required(VERSION 2.4)

# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../src")

set(CORE_SOURCES
  qbus_core.c
  qbus_frame.c
  qbus_route.c
  qbus_route_items.c
)

set(CORE_HEADERS
  qbus_core.h
  qbus_frame.h
  qbus_route.h
  qbus_route_items.h
)

add_library             (qbus_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_link_libraries   (qbus_core cape)
set_target_properties   (qbus_core PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

