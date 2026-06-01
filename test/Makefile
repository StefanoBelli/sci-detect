SHELL=/bin/sh
CC=gcc
CFLAGS=-O0 -Wall -W -Wextra -Wshadow -std=c11 -pedantic
TESTS= \
	   test-dap-first-access-read

all: $(TESTS)

clean:
	rm $(TESTS)

test-%: test-%.c
	@echo -ne "\tCC $@\n"
	@$(CC) $(CFLAGS) $< -o $@.test
