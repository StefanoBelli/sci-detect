SHELL=/bin/bash
CC=gcc
CFLAGS=-O0 -Wall -W -Wextra -Wshadow -std=c11
TESTS= \
	   test-dap-first-access-read.test \
	   test-dap-first-access-write.test \
	   test-dap-prefault.test \
	   test-dwp-wpc-cow-anon.test \
	   test-spr-first-access-read-shm-anon.test \
	   test-spr-first-access-write-shm-anon.test \
	   test-spr-childsharing-shm-anon-nonpresent.test \
	   test-spr-childsharing-shm-anon-present.test \
	   test-spr-first-access-read-shm-posix.test \
	   test-spr-first-access-write-shm-posix.test \
	   test-spr-childsharing-shm-posix-nonpresent.test \
	   test-spr-childsharing-shm-posix-present.test \
	   test-spr-first-access-read-shm-sysv.test \
	   test-spr-first-access-write-shm-sysv.test \
	   test-spr-childsharing-shm-sysv-present.test \
	   test-spr-childsharing-shm-sysv-nonpresent.test \
	   test-spr-read-file-private.test \
	   test-spr-dwp-wpc-cow-write-file-private.test \
	   #test-spr-read-file-shared.test \
	   test-spr-write-file-shared.test \
	   test-spr-childsharing-file-shared.test \
	   test-file-prefault.test
 
all: $(TESTS)

clean:
	rm $(TESTS)

test-%.test: test-%.c
	@echo -ne "\tCC [test] $@\n"
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
