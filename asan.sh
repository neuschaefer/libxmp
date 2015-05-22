#!/bin/sh

CC='afl-gcc -fsanitize=address -m32 -g'

make             CC="$CC" LD="$CC" clean all install
make -C examples CC="$CC" LD="$CC" clean all
