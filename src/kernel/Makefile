SHELL=/bin/bash
modname=sci-detect
obj-m := $(modname).o
$(modname)-objs += \
	main.o \
	vmfs.o \
	\
		testing/testing.o \
		testing/default-kvops.o \
	\
	hooks.o \
	\
		hooks/setuputils.o \
		\
			hooks/add/hpf.o \
			hooks/add/dap.o \
			hooks/add/wpc.o \
			hooks/add/dwp.o \
			hooks/add/spr.o \
			\
				hooks/add/utils/addpages.o \
				hooks/add/utils/setup.o \
			\
		\
			hooks/chg/cpr.o \
			\
				hooks/chg/utils/setup.o \
			\
		\
			\
				hooks/del/utils/setup.o \
			\
		\

ccflags-y += -Wall -W -Wextra -Wshadow -I$(src)/include -Wno-shadow -O2 -DSCID_CONFIG_TESTING

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
