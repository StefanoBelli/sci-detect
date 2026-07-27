SHELL=/bin/bash
modname=sci-detect
obj-m := $(modname).o
$(modname)-objs += \
    main.o \
    kcps.o \
    resolve_syms.o \
    pgtrack.o \
    pgsnap.o \
    kpsleepable.o \
    ptealtprot.o \
    \
    	testing/testing.o \
    	testing/default-kvops.o \
    \
    netlink.o \
    \
    	netlink/pgtrack-genl-setup.o \
    	netlink/pgtrack-genl-gle.o \
    	netlink/pgtrack-genl-itp.o \
    	netlink/pgtrack-genl-events.o \
    	netlink/pgtrack-genl-gatp.o \
    	netlink/pgtrack-genl-gcps.o \
    	netlink/pgtrack-genl-gole.o \
    \
    hooks.o \
    \
    	hooks/setuputils.o \
    \
    	hooks/pte-page-track/hpf.o \
    	hooks/pte-page-track/dap.o \
    	hooks/pte-page-track/wpc.o \
    	hooks/pte-page-track/dwp.o \
    	hooks/pte-page-track/spr.o \
    	hooks/pte-page-track/cpr.o \
    	hooks/pte-page-track/fuf.o \
    	hooks/pte-page-track/fsf.o \
    \
    	hooks/pte-page-track/utils/addpages.o \
    	hooks/pte-page-track/utils/setup.o \
    	hooks/pte-page-track/utils/check_orig_pte.o \
    	hooks/pte-page-track/utils/user_page_walk.o 

ccflags-y += -Wall -W -Wextra -I$(src)/include -Wno-shadow

# utility stuff

comma := ,
empty :=
space := $(empty) $(empty)

# ------ build type ------

valid_build_types = release testing
BUILD_TYPE ?= release

ifneq ($(filter $(BUILD_TYPE), $(valid_build_types)),)
    $(info BUILD_TYPE=$(BUILD_TYPE) ok)
else
    $(error BUILD_TYPE=$(BUILD_TYPE) is invalid)
endif

ifeq ($(BUILD_TYPE), testing)
    ccflags-y += -DSCID_CONFIG_TESTING
endif

# ------ snapshot ------

valid_disable_snapshot = yes no
DISABLE_SNAPSHOT ?= no

ifneq ($(filter $(DISABLE_SNAPSHOT), $(valid_disable_snapshot)),)
    $(info DISABLE_SNAPSHOT=$(DISABLE_SNAPSHOT) ok)
else
    $(error DISABLE_SNAPSHOT=$(DISABLE_SNAPSHOT) is invalid)
endif

ifeq ($(DISABLE_SNAPSHOT), yes)
    ccflags-y += -DDISABLE_PAGE_SNAPSHOT
endif

# ------ ptealtprot -----

valid_disable_ptealtprot = yes no
DISABLE_PTEALTPROT ?= no

ifneq ($(filter $(DISABLE_PTEALTPROT), $(valid_disable_ptealtprot)),)
    $(info DISABLE_PTEALTPROT=$(DISABLE_PTEALTPROT) ok)
else
    $(error DISABLE_PTEALTPROT=$(DISABLE_PTEALTPROT) is invalid)
endif

ifeq ($(DISABLE_PTEALTPROT), yes)
    ccflags-y += -DDISABLE_PTE_ALT_PROT
endif

# ------- other options -------

PAP_STRICTNESS ?=
STRICTNESS_LIST := $(subst $(comma),$(space),$(PAP_STRICTNESS))

ifneq ($(filter init-lock-order, $(STRICTNESS_LIST)),)
    $(info PAP_STRICTNESS=init-lock-order ok)
    ccflags-y += -DPAP_INIT_LOCK_ORDER_STRICT
endif

ifneq ($(filter no-cpr-trylock, $(STRICTNESS_LIST)),)
    $(info PAP_STRICTNESS=no-cpr-trylock ok)
    ccflags-y += -DPAP_IN_CPR_MMSLK_DONT_TRYLOCK
endif

# -----------------------------

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules

clean:
	@if [ $(shell id -u) -eq 0 ]; then \
		echo "**WARNING** cleaning as root may break your kernel source code"; \
		echo "**WARNING** probably, you don't want to do this"; \
		echo "**WARNING** giving you 15 secs to --- press CTRL+C to abort operation ---"; \
		sleep 8; \
		echo "**WARNING** DO IT NOW --- press CTRL+C to abort operation --- WHAT ARE YOU WAITING FOR?"; \
		sleep 6; \
		echo "!! you have been warned... I hope you know what you're doing"; \
		sleep 1; \
		echo "proceeding..."; \
		fi
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean

mount:
	insmod $(modname).ko

umount:
	rmmod $(modname)

remount: umount mount
