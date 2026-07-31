#!/bin/bash

clear
sudo LD_LIBRARY_PATH=../libscid ./scid.out --disable-hexdump --disable-disasm --sub-bcast --poll-forever
