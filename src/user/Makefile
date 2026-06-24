all: build-libscid build-scid-cli

clean: scid-cli-clean libscid-clean

build-scid-cli:
	make -C scid-cli

build-libscid:
	make -C libscid

scid-cli-clean:
	make -C scid-cli clean

libscid-clean:
	make -C libscid clean
