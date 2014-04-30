GDC should build for MINGW and MINGW64 targets, however not much
testing has been done.

## Building ##

The following patches are required for binutils / gcc to fix TLS support:
 * https://github.com/venix1/MinGW-GDC/blob/master/patches/mingw-tls-binutils-2.23.1.patch
 * https://github.com/venix1/MinGW-GDC/blob/master/patches/mingw-tls-gcc-4.8.patch

This patch is required to fix a bug in GCC buildscripts:
 * https://github.com/venix1/MinGW-GDC/blob/master/patches/gcc/0001-Remove-fPIC-for-MinGW.patch

## SEH ##
GCC supports SEH for Windows 64 bit targets, but GDC has not been tested with SEH
enabled. It also does not compile right now because no SEH personality routine has
been written yet (__gdc_personality_seh0).

Configure gcc with --enable-sjlj-exceptions to use SJLJ execptions for win64 targets.
DWARF exceptions are untested.
