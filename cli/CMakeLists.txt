cmake_minimum_required(VERSION 2.4)

find_package(Curses QUIET)

IF (CURSES_LIBRARIES)

  # abstract operation-system layer
  INCLUDE_DIRECTORIES("../../cape/src" "../src" "../../adbl/src/core")

  set(CLI_SOURCES
    qbus_cli.c
    qbus_cli_modules.c
    qbus_cli_methods.c
    qbus_cli_requests.c
  )

  set(CLI_HEADERS
    qbus_cli_modules.h
    qbus_cli_methods.h
    qbus_cli_requests.h
  )

  add_executable          (qbus_cli ${CLI_SOURCES})
  target_link_libraries   (qbus_cli qbus adbl2 ${CURSES_LIBRARIES})
  set_target_properties   (qbus_cli PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

  INSTALL (TARGETS qbus_cli DESTINATION bin)
  
ENDIF()
