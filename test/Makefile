SHELL=/bin/bash
CC=gcc
CFLAGS=-O0 -Wall -W -Wextra -Wshadow -std=c11 -pedantic
TESTS= \
	   test-dap-first-access-read.test \
	   test-dap-first-access-write.test \
	   test-dap-prefault.test \
	   test-dwp-wpc-cow-anon.test \
	   test-spr-first-access-read-shm-anon.test \
	   test-spr-first-access-write-shm-anon.test \
	   test-spr-childsharing-shm-anon.test

all: $(TESTS)

clean:
	rm $(TESTS)

test-%.test: test-%.c
	@echo -ne "\tCC $@\n"
	@$(CC) $(CFLAGS) $< -o $@

run-tests: all
	@echo -e "\033[1;37m**\033[0m Running the \033[1;37mwhole test suite\033[0m..."
	@for test in $(TESTS); do \
		echo -e "\n \033[1;33m-->\033[0m Running \033[1;33m$$test\033[0m..."; \
		./$$test || { \
			echo -e "\n\033[1;31m!!\033[0m Test suite \033[1;31mfailure\033[0m, aborting..."; \
			exit 1; }; \
	done
	@echo -e "\n\033[1;32m++\033[0m Test suite \033[1;32msuccess\033[0m!"
