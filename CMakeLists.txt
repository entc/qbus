cmake_minimum_required(VERSION 2.4)

SUBDIRS(core)
SUBDIRS(engines/tcp)
SUBDIRS(src)
SUBDIRS(app)
SUBDIRS(adapter/java)
SUBDIRS(adapter/python)

# copy into tmp folder all files needed


# remove none used ones (needs to fix)
INSTALL(CODE "execute_process(COMMAND docker image rm qore:1.0.0)")

# build the docker image
INSTALL(CODE "execute_process(COMMAND docker build -t qore:1.0.0 -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_INSTALL_PREFIX})")
