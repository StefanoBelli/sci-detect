#!/bin/bash

# you may override --disable-hexdump and --disable-disasm
# e.g. ./run.sh --enable-hexdump --hexdump-length 16

clear
sudo LD_LIBRARY_PATH=../libscid ./scid.out --disable-hexdump --disable-disasm $@ --sub-bcast --poll-forever
