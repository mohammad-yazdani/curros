#!/bin/bash

PREFIX=/usr/local/cross
TARGET=x86_64-elf



cd ./build-binutils
../src-binutils/configure --prefix=$PREFIX --target=$TARGET --disable-nls --enable-languages=c,c++ --with-headers=/usr/cross/x86_64-elf/include --with-gmp=/usr/local --with-mpfr=/usr/local
make all
make install
cd ..

cd ./build-gcc
../src-gcc/configure --prefix=$PREFIX --target=$TARGET --disable-nls --enable-languages=c,c++ --with-headers=/usr/cross/x86_64-elf/include --with-gmp=/usr/local --with-mpfr=/usr/local
make all-gcc
make install-gcc
cd ..


