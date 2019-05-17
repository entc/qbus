cmake_minimum_required(VERSION 2.4)

SUBDIRS(core)
SUBDIRS(engines/tcp)
SUBDIRS(src)
SUBDIRS(app)
SUBDIRS(adapter/java)
SUBDIRS(adapter/python)

# remove none used docker base
INSTALL (CODE "execute_process(COMMAND docker image rm scribele/base:1.0.0)")

# build the docker image
INSTALL (CODE "execute_process(COMMAND docker build --no-cache -t scribele/base:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
