SHELL=/bin/sh
CC=gcc
CFLAGS=-O2 -Wall -W -Wextra -Wshadow 
SCID_CLI_OUT=scid.out
SCID_CLI_OBJS=scid-cli.o
LIBSCID_DIR=../libscid
LIBSCID_HDRS=$(LIBSCID_DIR)/include
LIBSCID_LNK=scid
SCID_SHARED_HEADER=../../kernel/include/user

all: lib cli

cli: $(SCID_CLI_OUT)

$(SCID_CLI_OUT): $(SCID_CLI_OBJS) | lib
	@echo -e "\tLINK $@"
	@$(CC) \
		$(CFLAGS) \
		-L$(LIBSCID_DIR) -l$(LIBSCID_LNK) \
		$^ -o $@ 

$(SCID_CLI_OBJS): %.o: %.c
	@echo -e "\tCC $<"
	@$(CC) \
		$(CFLAGS) -c \
		-L$(LIBSCID_DIR) -l$(LIBSCID_LNK) \
		-I$(SCID_SHARED_HEADER) -I$(LIBSCID_HDRS) \
		-o $@ $<

clean:
	rm $(SCID_CLI_OBJS)

lib:
	make -C $(LIBSCID_DIR)

lib-clean:
	make -C $(LIBSCID_DIR) clean

run:
	@LD_LIBRARY_PATH=$(LIBSCID_DIR) ./$(SCID_CLI_OUT)
