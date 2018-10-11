UNAME := $(shell uname -s)

SRC := src/server.cpp src/webapi.cpp src/protocol.cpp

ifneq ($(MTRACE),)
CFLAGS=-I/usr/local/include -Iinclude -L/usr/local/lib -Wall -g -DMTRACE
else
CFLAGS=-I/usr/local/include -Iinclude -L/usr/local/lib -Wall -g
endif

ifeq ($(UNAME),Linux)
LDFLAGS=-lrt -pthread -lmicrohttpd
endif
ifeq ($(UNAME),Darwin)
LDFLAGS=-pthread -lmicrohttpd
endif

default:: server

server:: ${SRC}
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS)
clean::
	rm -f server
	rm -rf server.dSYM
