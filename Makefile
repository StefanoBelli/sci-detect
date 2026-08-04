# default all for everyone

PAP_STRICTNESS ?=
export PAP_STRICTNESS

WARNINGS ?=
export WARNINGS

KPS_MAXACTIVE ?=
export KPS_MAXACTIVE

all: kernel user

clean: kernel-clean user-clean

kernel:
	make -C src/kernel

kernel-no-ptealtprot:
	make -C src/kernel DISABLE_PTEALTPROT=yes

kernel-no-snapshot:
	make -C src/kernel DISABLE_SNAPSHOT=yes

kernel-no-snapshot-no-ptealtprot:
	make -C src/kernel DISABLE_SNAPSHOT=yes DISABLE_PTEALTPROT=yes

kernel-testing:
	make -C src/kernel BUILD_TYPE=testing

kernel-testing-no-snapshot:
	make -C src/kernel BUILD_TYPE=testing DISABLE_SNAPSHOT=yes

kernel-testing-no-ptealtprot:
	make -C src/kernel BUILD_TYPE=testing DISABLE_PTEALTPROT=yes

kernel-testing-no-snapshot-no-ptealtprot:
	make -C src/kernel BUILD_TYPE=testing DISABLE_SNAPSHOT=yes DISABLE_PTEALTPROT=yes

user:
	make -C src/user

kernel-clean:
	make -C src/kernel clean

user-clean:
	make -C src/user -i clean

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

run-snapshot-examples:
	make -C examples run-snapshot-examples

run-all-examples:
	make -C examples run-all-examples

run-mlocked-examples:
	make -C examples MLOCK_ALL=yes run-examples

run-mlocked-snapshot-examples:
	make -C examples MLOCK_ALL=yes run-snapshot-examples

run-mlocked-all-examples:
	make -C examples MLOCK_ALL=yes run-all-examples

examples-clean:
	make -C examples clean
