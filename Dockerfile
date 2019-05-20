FROM base/sys:1.0.0

# specify the port number the container should expose
EXPOSE 33340

ADD docker/qore/lib lib
ADD docker/qore/bin bin
