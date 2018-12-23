#!/bin/bash
# This script is intended to be ran on SemaphoreCI or Buildkite platform.
# Following environmental variables are assumed to be exported on SemaphoreCI.
#
# - SEMAPHORE_PROJECT_DIR
# - SEMAPHORE_CACHE_DIR
#
# See https://semaphoreci.com/docs/available-environment-variables.html
#
# Following environmental variables are assumed to be exported on Buildkite.
#
# - BUILDKITE_CACHE_DIR
# - BUILDKITE_TARGET
# - BUILDKITE_BOOTSTRAP
#
# See https://buildkite.com/docs/builds/environment-variables
#
## Top-level build system configration.
gcc_prereqs="gmp-6.1.0.tar.bz2 mpfr-3.1.4.tar.bz2 mpc-1.0.3.tar.gz isl-0.18.tar.bz2"
host_package="7"

export CC="gcc-${host_package}"
export CXX="g++-${host_package}"
export GDC="gdc-${host_package}"

environment() {
    ## Determine what flags to use for configure, build and testing the compiler.
    ## Commonize CI environment variables.
    #
    # project_dir:              directory of checked out sources.
    # cache_dir:                tarballs of downloaded dependencies cached
    #                           between builds.
    # build_host:               host triplet that build is ran from.
    # build_host_canonical:     canonical version of host triplet.
    # build_target:             target triplet of the compiler to build.
    # build_target_canonical:   canonical version of target triplet.
    # make_flags:               flags to pass to make.
    # build_bootstrap:          whether to enable bootstrap build.
    #
    if [ "${SEMAPHORE}" = "true" ]; then
        project_dir=${SEMAPHORE_PROJECT_DIR}
        cache_dir=${SEMAPHORE_CACHE_DIR}
        build_host=$($CC -dumpmachine)
        build_host_canonical=$(/usr/share/misc/config.sub ${build_host})
        build_target=${build_host}
        build_target_canonical=${build_host_canonical}
        make_flags="-j$(nproc)"
        build_bootstrap="disable"
    elif [ "${BUILDKITE}" = "true" ]; then
        project_dir=${PWD}
        cache_dir=${BUILDKITE_CACHE_DIR}
        build_host=$($CC -dumpmachine)
        build_host_canonical=$(/usr/share/misc/config.sub ${build_host})
        build_target=${BUILDKITE_TARGET}
        build_target_canonical=$(/usr/share/misc/config.sub ${build_target})
        make_flags="-j$(nproc) -sw LIBTOOLFLAGS=--silent"
        build_bootstrap=${BUILDKITE_BOOTSTRAP}
    else
        echo "Unhandled CI environment"
        exit 1
    fi

    ## Options determined by target, what steps to skip, or extra flags to add.
    ## Also, should the testsuite be ran under a simulator?
    #
    # build_supports_phobos:    whether to build phobos and run unittests.
    # build_target_phobos:      where to run the phobos testsuite from.
    # build_enable_languages:   which languages to build, this affects whether C++
    #                           or LTO tests are ran in the testsuite.
    # build_prebuild_script:    script to run after sources have been extracted.
    # build_configure_flags:    extra configure flags for the target.
    # build_test_flags:         options to pass to RUNTESTFLAGS.
    #
    build_supports_phobos='yes'
    build_target_phobos=''
    build_enable_languages='c++,d,lto'
    build_prebuild_script=''
    build_configure_flags=''
    build_test_flags=''

    # Check whether this is a cross or multiarch compiler.
    if [ "${build_host_canonical}" != "${build_target_canonical}" ]; then
        multilib_targets=( $(${CC} -print-multi-lib | cut -f2 -d\;) )
        is_cross_compiler=1

        for multilib in ${multilib_targets[@]}; do
            build_multiarch=$(${CC} -print-multiarch ${multilib/@/-})
            build_multiarch_canonical=$(/usr/share/misc/config.sub ${build_multiarch})

            # This is a multiarch compiler, update target to the host compiler.
            if [ "${build_multiarch_canonical}" = "${build_target_canonical}" ]; then
                build_target=$build_host
                build_target_canonical=$build_host_canonical
                build_target_phobos="${build_target}/$(${CC} ${multilib/@/-} -print-multi-directory)/libphobos"
                build_test_flags="--target_board=unix{${multilib/@/-}}"
                build_configure_flags='--enable-multilib --enable-multiarch'
                is_cross_compiler=0
                break
            fi
        done

        # Building a cross compiler, need to explicitly say where to find native headers.
        if [ ${is_cross_compiler} -eq 1 ]; then
            build_configure_flags="--with-native-system-header-dir=/usr/${build_target}/include"

            # Note: setting target board to something other than "generic" only makes
            # sense if phobos is being built. Without phobos, all runnable tests will
            # all fail as being 'UNRESOLVED', and so are never ran anyway.
            case ${build_target_canonical} in
                arm*-*-*)
                    build_test_flags='--target_board=buildci-arm-sim'
                    ;;
                *)
                    build_test_flags='--target_board=buildci-generic-sim'
                    ;;
            esac
        fi
    fi

    if [ "${build_target_phobos}" = "" ]; then
        build_target_phobos="${build_target}/libphobos"
    fi

    # Unless requested, don't build with multilib.
    if [ `expr "${build_configure_flags}" : '.*enable-multilib'` -eq 0 ]; then
        build_configure_flags="--disable-multilib ${build_configure_flags}"
    fi

    # If bootstrapping, be sure to turn off slow tree checking.
    if [ "${build_bootstrap}" = "enable" ]; then
        build_configure_flags="${build_configure_flags} \
            --enable-bootstrap --enable-checking=release"
    else
        build_configure_flags="${build_configure_flags} \
            --disable-bootstrap --enable-checking"
    fi

    # Determine correct flags for configuring a compiler for target.
    case ${build_target_canonical} in
      arm-*-*eabihf)
            build_configure_flags="${build_configure_flags} \
                --with-arch=armv7-a --with-fpu=vfpv3-d16 --with-float=hard --with-mode=thumb"
            build_prebuild_script="${cache_dir}/patches/arm-multilib.sh"
            ;;
      arm*-*-*eabi)
            build_configure_flags="${build_configure_flags} \
                --with-arch=armv5t --with-float=soft"
            ;;
      mips-*-*|mipsel-*-*)
            build_configure_flags="${build_configure_flags} \
                --with-arch=mips32r2"
            ;;
      mips64*-*-*)
            build_configure_flags="${build_configure_flags} \
                --with-arch-64=mips64r2 --with-abi=64"
            ;;
      powerpc64le-*-*)
            build_configure_flags="${build_configure_flags} \
                --with-cpu=power8 --with-long-double-format=ieee"
            ;;
      powerpc64-*-*)
            build_configure_flags="${build_configure_flags} \
                --with-cpu=power7"
            ;;
      x86_64-*-*)
            ;;
      *)
            build_supports_phobos='no'
            build_enable_languages='c++,d --disable-lto'
            ;;
    esac
}

installdeps() {
    ## Install build dependencies.
    # Would save 1 minute if these were preinstalled in some docker image.
    # But the network speed is nothing to complain about so far...
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update -qq
    sudo apt-get install -qq gcc-${host_package} g++-${host_package} gdc-${host_package} \
        autogen autoconf automake bison dejagnu flex rsync patch || exit 1
}

configure() {
    ## Download the GCC sources - last 350 commits should be plenty if kept in sync regularly.
    git submodule update --init --depth 350 gcc

    ## Sync local GDC sources with upstream GCC.
    rsync -avc --delete ${project_dir}/gdc/gcc/d/ ${project_dir}/gcc/gcc/d
    rsync -avc          ${project_dir}/gdc/gcc/config/ ${project_dir}/gcc/gcc/config
    rsync -avc --delete ${project_dir}/gdc/libphobos/ ${project_dir}/gcc/libphobos
    rsync -avc --delete ${project_dir}/gdc/gcc/testsuite/gdc.dg/ ${project_dir}/gcc/gcc/testsuite/gdc.dg
    rsync -avc --delete ${project_dir}/gdc/gcc/testsuite/gdc.test/ ${project_dir}/gcc/gcc/testsuite/gdc.test
    cp -av ${project_dir}/gdc/gcc/testsuite/lib/gdc.exp ${project_dir}/gcc/gcc/testsuite/lib/gdc.exp
    cp -av ${project_dir}/gdc/gcc/testsuite/lib/gdc-dg.exp ${project_dir}/gcc/gcc/testsuite/lib/gdc-dg.exp

    ## Apply GDC patches to GCC.
    if test -f ${project_dir}/patches/gcc.patch; then
        patch -d ${project_dir}/gcc -p1 -i ${project_dir}/patches/gcc.patch || exit 1
    fi

    ## And download GCC prerequisites.
    # Makes use of local cache to save downloading on every build run.
    for prereq in ${gcc_prereqs}; do
        if [ ! -e ${cache_dir}/infrastructure/${prereq} ]; then
            curl "ftp://gcc.gnu.org/pub/gcc/infrastructure/${prereq}" \
                --create-dirs -o ${cache_dir}/infrastructure/${prereq} || exit 1
        fi
        tar -C ${project_dir}/gcc -xf ${cache_dir}/infrastructure/${prereq}
        ln -s "${project_dir}/gcc/${prereq%.tar*}" "${project_dir}/gcc/${prereq%-*}"
    done

    ## Apply any ad-hoc fixes to the sources.
    if [ "${build_prebuild_script}" != "" ]; then
       source ${build_prebuild_script}
    fi

    ## Create the build directory.
    # Build typically takes around 10 minutes with -j4, could this be cached across CI runs?
    mkdir ${project_dir}/build
    cd ${project_dir}/build

    ## Configure GCC to build a D compiler.
    ${project_dir}/gcc/configure --prefix=/usr --libdir=/usr/lib --libexecdir=/usr/lib --with-sysroot=/ \
        --enable-languages=${build_enable_languages} --enable-link-mutex \
        --disable-werror --disable-libgomp --disable-libmudflap \
        --disable-libquadmath --disable-libitm --disable-libsanitizer \
        --build=${build_host} --host=${build_host} --target=${build_target} \
        ${build_configure_flags} --with-bugurl="http://bugzilla.gdcproject.org"
}

setup() {
    installdeps
    environment
    configure
}

build() {
    if [ "${build_bootstrap}" = "enable" ]; then
        ## Build the entire project to completion.
        cd ${project_dir}/build
        make ${make_flags}
    else
        ## Build the bare-minimum in order to run tests.
        cd ${project_dir}/build
        make ${make_flags} all-gcc || exit 1

        # Note: libstdc++ and libphobos are built separately so that build errors don't mix.
        if [ "${build_supports_phobos}" = "yes" ]; then
            make ${make_flags} all-target-libstdc++-v3 || exit 1
            make ${make_flags} all-target-libphobos || exit 1
        fi
    fi
}

testsuite() {
    ## Run just the compiler testsuite.
    cd ${project_dir}/build

    make check-gcc RUNTESTFLAGS="help.exp"
    make ${make_flags} check-gcc-d RUNTESTFLAGS="${build_test_flags}"

    # For now, be lenient towards targets with no phobos support,
    # and ignore unresolved test failures.
    if [ "${build_supports_phobos}" = "yes" ]; then
        print_filter="^PASS"
        fail_filter="^\(FAIL\|UNRESOLVED\)"
    else
        print_filter="^\(PASS\|UNRESOLVED\)"
        fail_filter="^FAIL"
    fi

    ## Print out summaries of testsuite run after finishing.
    # Just omit testsuite PASSes from the summary file.
    grep -v ${print_filter} ${project_dir}/build/gcc/testsuite/gcc/gcc.sum ||:
    grep -v ${print_filter} ${project_dir}/build/gcc/testsuite/gdc*/gdc.sum ||:

    # Test for any failures and return false if any.
    if grep -q ${fail_filter} ${project_dir}/build/gcc/testsuite/gdc*/gdc.sum; then
       echo "== Testsuite has failures =="
       exit 1
    fi
}

unittests() {
    ## Run just the library unittests.
    if [ "${build_supports_phobos}" = "yes" ]; then
        cd ${project_dir}/build
        if ! make ${make_flags} -C ${build_target_phobos} check RUNTESTFLAGS="${build_test_flags}"; then
            echo "== Unittest has failures =="
            exit 1
        fi
    fi
}


## Run a single build task or all at once.
if [ "$1" != "" ]; then
    # Skip calling environment if running setup, as dependencies might not be installed yet.
    if [ "$1" != "setup" ]; then
        environment
    fi
    $1
else
    setup
    build
    unittests
fi
