#!/bin/bash

set -ex

export CC=${CC:-clang}
export CXX=${CXX:-clang++}
export WORK=${WORK:-$(pwd)}
export OUT=${OUT:-$(pwd)/out}

mkdir -p $OUT

build=$WORK/build
rm -rf $build
mkdir -p $build

export LIBTPMS=$(pwd)
autoreconf -vfi

cd $build
$LIBTPMS/configure --disable-shared --enable-static --with-openssl --with-tpm2
make -j$(nproc) && make -C tests fuzz

zip -jqr $OUT/fuzz_seed_corpus.zip "$LIBTPMS/tests/corpus-execute-command"

find $build -type f -executable -name "fuzz*" -exec mv {} $OUT \;
find $build -type f -name "*.options" -exec mv {} $OUT \;
