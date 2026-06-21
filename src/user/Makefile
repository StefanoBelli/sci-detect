SHELL=/bin/sh
CC=gcc
CFLAGS=-O2 -Wall -W -Wextra -Wshadow 
SCID_NL_SHARED_HEADER=../kernel/include/user
LIBNL = libnl-3.0 libnl-genl-3.0
LIBSCID_SRC = scid.c
LIBSCID_HDRS = include
LIBSCID_OUT = libscid.so
SCID_EXEC_OUT = scid.out
SCID_EXEC_SRC = main.c

all: $(LIBSCID_OUT) $(SCID_EXEC_OUT)

$(LIBSCID_OUT): $(LIBSCID_SRC)
	$(CC) $(CFLAGS) -I$(SCID_NL_SHARED_HEADER) -I$(LIBSCID_HDRS) \
		-shared -fPIC -fpic -o $@ \
		$(shell pkg-config --libs --cflags $(LIBNL)) \
		$<

$(SCID_EXEC_OUT): $(SCID_EXEC_SRC) | $(LIBSCID_OUT)
	$(CC) $(CFLAGS) -I$(LIBSCID_HDRS) -L. -lscid -o $@ $<

run:
	LD_LIBRARY_PATH=. ./$(SCID_EXEC_OUT)

clean:
	rm $(LIBSCID_OUT) $(SCID_EXEC_OUT)
