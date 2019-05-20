FROM base/sys:1.0.0

# specify the port number the container should expose
EXPOSE 44430

ADD docker/qore/lib lib
ADD docker/qore/bin bin
