#!/bin/bash
# This script is intended to be ran on SemaphoreCI platform.
# Following environmental variables are assumed to be exported.
# - SEMAPHORE_PROJECT_DIR
# - SEMAPHORE_CACHE_DIR
# See https://semaphoreci.com/docs/available-environment-variables.html

## Top-level build system configration.
GCC_PREREQS="gmp-6.1.0.tar.bz2 mpfr-3.1.4.tar.bz2 mpc-1.0.3.tar.gz isl-0.18.tar.bz2"
HOST_PACKAGE="7"

export CC="gcc-${HOST_PACKAGE}"
export CXX="g++-${HOST_PACKAGE}"
export GDC="gdc-${HOST_PACKAGE}"

setup() {
    ## Install build dependencies.
    # Would save 1 minute if these were preinstalled in some docker image.
    # But the network speed is nothing to complain about so far...
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update -qq
    sudo apt-get install -qq gcc-${HOST_PACKAGE} g++-${HOST_PACKAGE} gdc-${HOST_PACKAGE} \
        autogen autoconf automake bison dejagnu flex rsync patch || exit 1

    ## Download the GCC sources - last 350 commits should be plenty if kept in sync regularly.
    git submodule update --init --depth 350 gcc

    ## Sync local GDC sources with upstream GCC.
    rsync -avc --delete ${SEMAPHORE_PROJECT_DIR}/gdc/gcc/d/ ${SEMAPHORE_PROJECT_DIR}/gcc/gcc/d
    rsync -avc --delete ${SEMAPHORE_PROJECT_DIR}/gdc/libphobos/ ${SEMAPHORE_PROJECT_DIR}/gcc/libphobos
    rsync -avc --delete ${SEMAPHORE_PROJECT_DIR}/gdc/gcc/testsuite/gdc.dg/ ${SEMAPHORE_PROJECT_DIR}/gcc/gcc/testsuite/gdc.dg
    rsync -avc --delete ${SEMAPHORE_PROJECT_DIR}/gdc/gcc/testsuite/gdc.test/ ${SEMAPHORE_PROJECT_DIR}/gcc/gcc/testsuite/gdc.test
    cp -av ${SEMAPHORE_PROJECT_DIR}/gdc/gcc/testsuite/lib/gdc.exp ${SEMAPHORE_PROJECT_DIR}/gcc/gcc/testsuite/lib/gdc.exp
    cp -av ${SEMAPHORE_PROJECT_DIR}/gdc/gcc/testsuite/lib/gdc-dg.exp ${SEMAPHORE_PROJECT_DIR}/gcc/gcc/testsuite/lib/gdc-dg.exp

    ## Apply GDC patches to GCC.
    if test -f ${SEMAPHORE_PROJECT_DIR}/patches/gcc.patch; then
        patch -d ${SEMAPHORE_PROJECT_DIR}/gcc -p1 -i ${SEMAPHORE_PROJECT_DIR}/patches/gcc.patch || exit 1
    fi

    ## And download GCC prerequisites.
    # Makes use of local cache to save downloading on every build run.
    for PREREQ in ${GCC_PREREQS}; do
        if [ ! -e ${SEMAPHORE_CACHE_DIR}/infrastructure/${PREREQ} ]; then
            curl "ftp://gcc.gnu.org/pub/gcc/infrastructure/${PREREQ}" \
                --create-dirs -o ${SEMAPHORE_CACHE_DIR}/infrastructure/${PREREQ} || exit 1
        fi
        tar -C ${SEMAPHORE_PROJECT_DIR}/gcc -xf ${SEMAPHORE_CACHE_DIR}/infrastructure/${PREREQ}
        ln -s "${SEMAPHORE_PROJECT_DIR}/gcc/${PREREQ%.tar*}" "${SEMAPHORE_PROJECT_DIR}/gcc/${PREREQ%-*}"
    done

    ## Create the build directory.
    # Build typically takes around 10 minutes with -j4, could this be cached across CI runs?
    mkdir ${SEMAPHORE_PROJECT_DIR}/build
    cd ${SEMAPHORE_PROJECT_DIR}/build

    ## Configure GCC to build a D compiler.
    ${SEMAPHORE_PROJECT_DIR}/gcc/configure --enable-languages=c++,d,lto --enable-checking \
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

    make check-gcc RUNTESTFLAGS="help.exp"
    make -j$(nproc) check-gcc-d

    ## Print out summaries of testsuite run after finishing.
    # Just omit testsuite PASSes from the summary file.
    grep -v "^PASS" ${SEMAPHORE_PROJECT_DIR}/build/gcc/testsuite/gcc/gcc.sum ||:
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
