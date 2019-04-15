cmake_minimum_required(VERSION 2.4)

# abstract operation-system layer
INCLUDE_DIRECTORIES("../../cape/src" "../src")

set(APP_SOURCES
  qbus_app.c
)

set(APP_HEADERS

)

add_executable          (qbus_app ${APP_SOURCES})
target_link_libraries   (qbus_app qbus)
set_target_properties   (qbus_app PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)
