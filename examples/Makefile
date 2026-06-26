SHELL=/bin/bash
CC=gcc
CFLAGS=-O0 -Wall -W -Wextra -Wshadow -std=c11
LIBSCID_BASE=../src/user/libscid
LIBSCID_HDRS=$(LIBSCID_BASE)/include
SCID_SHARED_HDRS=../src/kernel/include/user
LIBSCID_LNK=scid

# order matters (e.g. two uncorrelated process)
EXAMPLES= \
	   example-single-one.example \
	   example-fork.example \
	   example-vfork.example \
	   example-shm-posix-fork.example \
	   example-shm-anon-fork.example \
	   example-shm-sysv-fork.example \
	   example-shm-posix-vfork.example \
	   example-shm-anon-vfork.example \
	   example-shm-sysv-vfork.example \
	   example-shm-posix-noncorr-1.example \
	   example-shm-posix-noncorr-2.example \
	   example-shm-sysv-noncorr-1.example \
	   example-shm-sysv-noncorr-2.example \
	   example-file-priv-fork.example \
	   example-file-priv-vfork.example \
	   example-file-shared-noncorr-1.example \
	   example-file-shared-noncorr-2.example \
	   example-file-shared-fork.example \
	   example-file-shared-vfork.example \
	   example-fork-mremap-child.example \
	   example-fork-mremap-parent.example \
	   example-fork-mremap-dontunmap-parent.example \
	   example-fork-mremap-dontunmap-child.example \
	   example-fork-mremap-extend-child.example \
	   example-vfork-mremap-child.example \
	   example-vfork-mremap-parent.example \
	   example-vfork-mremap-dontunmap-child.example \
	   example-vfork-mremap-dontunmap-parent.example \
	   example-vfork-mremap-extend-child.example \
	   example-shm-anon-mremap-parent-fork.example \
	   example-shm-anon-mremap-child-fork.example \
	   example-shm-anon-mremap-dontunmap-child-fork.example \
	   example-shm-posix-mremap-fork.example \
	   example-shm-posix-mremap-extend-fork.example \
	   example-shm-sysv-mremap-fork.example \
	   example-file-shared-mremap-fork.example \
	   example-file-shared-mremap-extend-vfork.example \
	   example-file-priv-mremap-fork.example \
	   example-file-priv-mremap-extend-vfork.example \
	   example-fork-prefault.example \
	   example-vfork-prefault.example \
	   example-shm-anon-fork-prefault.example \
	   example-shm-posix-vfork-prefault.example \
	   example-shm-sysv-mremap-fork-prefault.example


valid_mlock_all = yes no
MLOCK_ALL ?= no

ifneq ($(filter $(MLOCK_ALL), $(valid_mlock_all)),)
    $(info MLOCK_ALL=$(MLOCK_ALL) ok)
else
    $(error MLOCK_ALL=$(MLOCK_ALL) is invalid)
endif

valid_build_types = libscid-check without-libscid-check
BUILD_TYPE ?= libscid-check

ifneq ($(filter $(BUILD_TYPE), $(valid_build_types)),)
    $(info BUILD_TYPE=$(BUILD_TYPE) ok)
else
    $(error BUILD_TYPE=$(BUILD_TYPE) is invalid)
endif

ifeq ($(BUILD_TYPE), libscid-check)
	CFLAGS += -DEXAMPLE_CHECK_WITH_LIBSCID
endif 

ifeq ($(MLOCK_ALL), yes)
	CFLAGS += -DEXAMPLE_MLOCK_ALL
endif

all: $(EXAMPLES)

clean:
	rm $(EXAMPLES)

# Thanks: https://stackoverflow.com/questions/42113237/undefined-reference-when-linking-dynamic-library-in-ubuntu-system

example-%.example: example-%.c
	@echo -ne "\tCC [example] $@\n"
	@$(CC) $(CFLAGS) $< \
		-I$(LIBSCID_HDRS) -I$(SCID_SHARED_HDRS) \
		-L$(LIBSCID_BASE) -l$(LIBSCID_LNK) \
		-o $@

run-examples: all
	@echo -e "\033[1;37m**\033[0m Running \033[1;37mall the examples\033[0m..."
	@for example in $(EXAMPLES); do \
		echo -e "\n \033[1;33m-->\033[0m Running \033[1;33m$$example\033[0m..."; \
		LD_LIBRARY_PATH=$(LIBSCID_BASE) ./$$example || { \
			echo -e "\n\033[1;31m!!\033[0m Examples \033[1;31mfailure\033[0m, aborting..."; \
			exit 1; }; \
	done
	@echo -e "\n\033[1;32m++\033[0m Examples \033[1;32msuccess\033[0m!"
