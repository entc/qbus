cmake_minimum_required(VERSION 2.4)

find_package (PythonLibs COMPONENTS Interpreter Development QUIET)

IF(PYTHON_LIBRARIES)

  message (STATUS "PYTHON_INCLUDES  = ${PYTHON_INCLUDE_DIR}")
  message (STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")

  # abstract operation-system layer
  INCLUDE_DIRECTORIES("../../../cape/src" "../../src" ${PYTHON_INCLUDE_DIR})

  set(ADAPTER_PYTHON_SOURCES
    py_adapter.c
    py_tools.c
    py_qbus.c
  )

  set(ADAPTER_PYTHON_HEADERS
    py_tools.h
    py_qbus.h
  )

  add_library             (qbus_adapter_python SHARED ${ADAPTER_PYTHON_SOURCES} ${ADAPTER_PYTHON_HEADERS})
  target_link_libraries   (qbus_adapter_python cape qbus ${PYTHON_LIBRARIES})
  set_target_properties   (qbus_adapter_python PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

  install (TARGETS qbus_adapter_python DESTINATION lib)

  # for docker
  INSTALL (TARGETS qbus_adapter_python DESTINATION docker/vele/mods)

ENDIF()
