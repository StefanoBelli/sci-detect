SHELL=/bin/bash
modname=sci-detect
obj-m := $(modname).o
$(modname)-objs += \
	main.o \
	hooks/hooks.o \
	hooks/hpf.o \
	hooks/dap.o \
	hooks/wpc.o \
	hooks/wpr.o \
	hooks/pageutils.o \
	vmfs.o
ccflags-y += -Wall -W -Wextra -Wshadow -I$(src)/include -Wno-shadow -O2 #careful with opt

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
	@insmod $(modname).ko

umount:
	rmmod $(modname)
