SHELL=/bin/sh
CC=gcc
CFLAGS=-O2 -Wall -W -Wextra -Wshadow 
SCID_CLI_OUT=scid.out
SCID_CLI_OBJS=scid-cli.o
LIBSCID_HDRS=libscid/include
LIBSCID_DIR=libscid
LIBSCID_LNK=scid
SCID_SHARED_HEADER=../kernel/include/user

all: lib cli

cli: $(SCID_CLI_OUT)

$(SCID_CLI_OUT): $(SCID_CLI_OBJS) | lib
	@echo -e "\tLINK $@"
	@$(CC) $(CFLAGS) -L$(LIBSCID_DIR) -l$(LIBSCID_LNK) $^ -o $@ 

$(SCID_CLI_OBJS): %.o: %.c
	@echo -e "\tCC $<"
	@$(CC) $(CFLAGS) -c -I$(SCID_SHARED_HEADER) -I$(LIBSCID_HDRS) -o $@ $<

clean:
	rm $(SCID_CLI_OBJS)

lib:
	make -C libscid

lib-clean:
	make -C libscid clean

run:
	@LD_LIBRARY_PATH=libscid ./$(SCID_CLI_OUT)
