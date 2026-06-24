SHELL=/bin/bash
CC=gcc
CFLAGS=-O2 -Wall -W -Wextra -Wshadow 
SCID_NL_SHARED_HEADER=../../kernel/include/user
LIBNL = libnl-3.0 libnl-genl-3.0
LIBSCID_HDRS = include
LIBSCID_OUT = libscid.so
LIBSCID_OBJS = scid.o scid-cmds.o scid-wrappers.o

all: $(LIBSCID_OUT)

$(LIBSCID_OUT): $(LIBSCID_OBJS)
	@echo -ne "\tLINK $@\n"
	@$(CC) $(CFLAGS) -shared -o $@ $^ \
		$(shell pkg-config --libs $(LIBNL))

$(LIBSCID_OBJS): %.o: %.c
	@echo -ne "\tCC $<\n"
	@$(CC) $(CFLAGS) -fPIC -fpic -c -I$(SCID_NL_SHARED_HEADER) -I$(LIBSCID_HDRS) \
		 $(shell pkg-config --cflags $(LIBNL)) -o $@ $<

clean:
	rm $(LIBSCID_OBJS)
