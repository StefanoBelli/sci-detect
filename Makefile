# default all for everyone

all: kernel user

clean: kernel-clean user-clean

kernel:
	make -C src/kernel

kernel-testing:
	make -C src/kernel BUILD_TYPE=testing

user:
	make -C src/user

kernel-clean:
	make -C src/kernel clean

user-clean:
	make -C src/user clean

# module mounting

module-mount:
	make -C src/kernel mount

module-umount:
	make -C src/kernel umount

module-remount:
	make -C src/kernel remount

# tests

run-tests:
	make -C test run-tests

tests-clean:
	make -C test clean

# examples

run-examples:
	make -C examples run-examples

examples-clean:
	make -C examples clean
