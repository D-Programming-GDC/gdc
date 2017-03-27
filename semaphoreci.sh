#!/bin/bash
# This script is intended to be ran on SemaphoreCI platform.
# Following environmental variables are assumed to be exported.
# - SEMAPHORE_PROJECT_DIR
# - SEMAPHORE_CACHE_DIR
# See https://semaphoreci.com/docs/available-environment-variables.html

## Find out which branch we are building.
GCC_VERSION=$(cat gcc.version)
CACHE_BUILD="no"

if [ "${GCC_VERSION:0:5}" = "gcc-7" ]; then
    GCC_TARBALL="snapshots/${GCC_VERSION:4}/${GCC_VERSION}.tar.bz2"
    PATCH_VERSION="7"
    HOST_PACKAGE="5"
elif [ "${GCC_VERSION:0:5}" = "gcc-6" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    PATCH_VERSION="6"
    HOST_PACKAGE="5"
elif [ "${GCC_VERSION:0:5}" = "gcc-5" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    PATCH_VERSION="5"
    HOST_PACKAGE="5"
elif [ "${GCC_VERSION:0:7}" = "gcc-4.9" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    PATCH_VERSION="4.9"
    HOST_PACKAGE="4.9"
elif [ "${GCC_VERSION:0:7}" = "gcc-4.8" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    PATCH_VERSION="4.8"
    HOST_PACKAGE="4.8"
else
    echo "This version of GCC (${GCC_VERSION}) is not supported."
    exit 1
fi

## Install build dependencies.
# Would save 1 minute if these were preinstalled in some docker image.
# But the network speed is nothing to complain about so far...
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install -qq gcc-${HOST_PACKAGE} g++-${HOST_PACKAGE} \
    autogen autoconf2.64 automake1.11 bison dejagnu flex patch pxz || exit 1

## Download and extract GCC sources.
# Makes use of local cache to save downloading on every build run.
export CC="gcc-${HOST_PACKAGE}"
export CXX="g++-${HOST_PACKAGE}"

if [ ! -e ${SEMAPHORE_CACHE_DIR}/${GCC_TARBALL} ]; then
    curl "ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/${GCC_TARBALL}" \
        --create-dirs -o ${SEMAPHORE_CACHE_DIR}/${GCC_TARBALL} || exit 1
fi

tar --strip-components=1 -jxf ${SEMAPHORE_CACHE_DIR}/${GCC_TARBALL}

## Apply GDC patches to GCC.
for PATCH in toplev gcc versym-cpu versym-os; do
    patch -p1 -i ./gcc/d/patches/patch-${PATCH}-${PATCH_VERSION}.x || exit 1
done

## And download GCC prerequisites.
# Would save 30 seconds if these were also cached.
./contrib/download_prerequisites || exit 1

## Create the build directory.
# Build typically takes around 10 minutes, could this be cached across CI runs?
if [ ! -e ${SEMAPHORE_CACHE_DIR}/builds/${GCC_VERSION}.tar.xz ]; then
    mkdir ${SEMAPHORE_PROJECT_DIR}/build
    cd ${SEMAPHORE_PROJECT_DIR}/build

    ## Configure GCC to build a D compiler.
    ${SEMAPHORE_PROJECT_DIR}/configure --enable-languages=c++,d,lto --enable-checking \
        --enable-link-mutex --disable-bootstrap --disable-libgomp --disable-libmudflap \
        --disable-libquadmath --disable-multilib --with-bugurl="http://bugzilla.gdcproject.org"
    CACHE_BUILD="yes"
else
    tar -Jxf ${SEMAPHORE_CACHE_DIR}/builds/${GCC_VERSION}.tar.xz
fi

## Build the bare-minimum in order to run tests.
# Note: libstdc++ and libphobos are built separately so that build errors don't mix.
make -j4 all-gcc all-target-libstdc++-v3 || exit 1
make -j4 all-target-libphobos || exit 1

## Finally, run the testsuite.
# This takes around 25 minutes to run, should we add more parallel jobs?
make -j2 check-d

## Stash build for re-use across CI runs.
# GDC-specific objects are removed before creating the archive.
if [ "${CACHE_BUILD}" = "yes" ]; then
    mkdir -p ${SEMAPHORE_CACHE_DIR}/builds
    rm -rvf ${SEMAPHORE_PROJECT_DIR}/build/gcc/cc1d
    rm -rvf ${SEMAPHORE_PROJECT_DIR}/build/gcc/d
    rm -rvf ${SEMAPHORE_PROJECT_DIR}/build/gcc/gdc
    rm -rvf ${SEMAPHORE_PROJECT_DIR}/build/x86_64-pc-linux-gnu/libphobos
    tar -cvPf - ${SEMAPHORE_PROJECT_DIR}/build | pxz -c > ${SEMAPHORE_CACHE_DIR}/builds/${GCC_VERSION}.tar.xz
fi
