FROM scratch

# specify the port number the container should expose
EXPOSE 44430

ADD docker/qore/lib lib
