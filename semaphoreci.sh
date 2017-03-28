#!/bin/bash
# This script is intended to be ran on SemaphoreCI platform.
# Following environmental variables are assumed to be exported.
# - SEMAPHORE_PROJECT_DIR
# - SEMAPHORE_CACHE_DIR
# See https://semaphoreci.com/docs/available-environment-variables.html

## Find out which branch we are building.
GCC_VERSION=$(cat gcc.version)

if [ "${GCC_VERSION:0:5}" = "gcc-7" ]; then
    GCC_TARBALL="snapshots/${GCC_VERSION:4}/${GCC_VERSION}.tar.bz2"
    GCC_PREREQS="gmp-6.1.0.tar.bz2 mpfr-3.1.4.tar.bz2 mpc-1.0.3.tar.gz isl-0.16.1.tar.bz2"
    PATCH_VERSION="7"
    HOST_PACKAGE="5"
elif [ "${GCC_VERSION:0:5}" = "gcc-6" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    GCC_PREREQS="gmp-4.3.2.tar.bz2 mpfr-2.4.2.tar.bz2 mpc-0.8.1.tar.gz isl-0.15.tar.bz2"
    PATCH_VERSION="6"
    HOST_PACKAGE="5"
elif [ "${GCC_VERSION:0:5}" = "gcc-5" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    GCC_PREREQS="gmp-4.3.2.tar.bz2 mpfr-2.4.2.tar.bz2 mpc-0.8.1.tar.gz isl-0.14.tar.bz2"
    PATCH_VERSION="5"
    HOST_PACKAGE="5"
elif [ "${GCC_VERSION:0:7}" = "gcc-4.9" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    GCC_PREREQS="gmp-4.3.2.tar.bz2 mpfr-2.4.2.tar.bz2 mpc-0.8.1.tar.gz isl-0.12.2.tar.bz2 cloog-0.18.1.tar.gz"
    PATCH_VERSION="4.9"
    HOST_PACKAGE="4.9"
elif [ "${GCC_VERSION:0:7}" = "gcc-4.8" ]; then
    GCC_TARBALL="releases/${GCC_VERSION}/${GCC_VERSION}.tar.bz2"
    GCC_PREREQS="gmp-4.3.2.tar.bz2 mpfr-2.4.2.tar.bz2 mpc-0.8.1.tar.gz"
    PATCH_VERSION="4.8"
    HOST_PACKAGE="4.8"
else
    echo "This version of GCC ($GCC_VERSION) is not supported."
    exit 1
fi

export CC="gcc-${HOST_PACKAGE}"
export CXX="g++-${HOST_PACKAGE}"

setup() {
    ## Install build dependencies.
    # Would save 1 minute if these were preinstalled in some docker image.
    # But the network speed is nothing to complain about so far...
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update -qq
    sudo apt-get install -qq gcc-${HOST_PACKAGE} g++-${HOST_PACKAGE} \
        autogen autoconf2.64 automake1.11 bison dejagnu flex patch || exit 1

    ## Download and extract GCC sources.
    # Makes use of local cache to save downloading on every build run.
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
    # Makes use of local cache to save downloading on every build run.
    for PREREQ in ${GCC_PREREQS}; do
        if [ ! -e ${SEMAPHORE_CACHE_DIR}/infrastructure/${PREREQ} ]; then
            curl "ftp://gcc.gnu.org/pub/gcc/infrastructure/${PREREQ}" \
                --create-dirs -o ${SEMAPHORE_CACHE_DIR}/infrastructure/${PREREQ} || exit 1
        fi
        tar -xf ${SEMAPHORE_CACHE_DIR}/infrastructure/${PREREQ}
        ln -s "${PREREQ%.tar*}" "${PREREQ%-*}"
    done

    ## Create the build directory.
    # Build typically takes around 10 minutes with -j4, could this be cached across CI runs?
    mkdir ${SEMAPHORE_PROJECT_DIR}/build
    cd ${SEMAPHORE_PROJECT_DIR}/build

    ## Configure GCC to build a D compiler.
    ${SEMAPHORE_PROJECT_DIR}/configure --enable-languages=c++,d,lto --enable-checking \
        --enable-link-mutex --disable-bootstrap --disable-libgomp --disable-libmudflap \
        --disable-libquadmath --disable-multilib --with-bugurl="http://bugzilla.gdcproject.org"
}

build() {
    ## Build the bare-minimum in order to run tests.
    # Note: libstdc++ and libphobos are built separately so that build errors don't mix.
    cd ${SEMAPHORE_PROJECT_DIR}/build
    make -j$(nproc) all-gcc all-target-libstdc++-v3 || exit 1
    make -j$(nproc) all-target-libphobos || exit 1
}

alltests() {
    ## Run both the unittests and testsuite in parallel.
    # This takes around 25 minutes to run with -j2, should we add more parallel jobs?
    cd ${SEMAPHORE_PROJECT_DIR}/build
    make -j$(nproc) check-d

    ## Print out summaries of testsuite run after finishing.
    # Just omit testsuite PASSes from file.
    grep -v "^PASS" ${SEMAPHORE_PROJECT_DIR}/build/gcc/testsuite/gdc*/gdc.sum ||:
}

testsuite() {
    ## Run just the compiler testsuite.
    cd ${SEMAPHORE_PROJECT_DIR}/build
    make -j$(nproc) check-gcc-d

    ## Print out summaries of testsuite run after finishing.
    # Just omit testsuite PASSes from the summary file.
    grep -v "^PASS" ${SEMAPHORE_PROJECT_DIR}/build/gcc/testsuite/gdc*/gdc.sum ||:

    # Test for any failures and return false if any.
    if grep -q "^\(FAIL\|UNRESOLVED\)" ${SEMAPHORE_PROJECT_DIR}/build/gcc/testsuite/gdc*/gdc.sum; then
       echo "== Testsuite has failures =="
       exit 1
    fi
}

unittests() {
    ## Run just the library unittests.
    cd ${SEMAPHORE_PROJECT_DIR}/build
    if ! make -j$(nproc) check-target-libphobos; then
        echo "== Unittest has failures =="
        exit 1
    fi
}


## Run a single build task or all at once.
if [ "$1" != "" ]; then
    $1
else
    setup
    build
    alltests
fi
