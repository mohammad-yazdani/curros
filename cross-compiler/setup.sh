#!/bin/bash

curl https://ftp.gnu.org/gnu/binutils/binutils-2.31.1.tar.gz --output binutils.tar.gz
curl https://ftp.gnu.org/gnu/gcc/gcc-8.2.0/gcc-8.2.0.tar.gz --output gcc.tar.gz

mkdir src-binutils src-gcc

tar -xvzf binutils.tar.gz -C src-binutils --strip-components=1
tar -xvzf gcc.tar.gz -C src-gcc --strip-components=1

cd src-gcc
./contrib/download_prerequisites
cd ..

rm -r -f *.tar.*

mkdir -p build-binutils build-gcc

