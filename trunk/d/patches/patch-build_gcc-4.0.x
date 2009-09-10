--- gcc-5363-orig/build_gcc	2006-06-27 03:29:34.000000000 -0400
+++ gcc-5363/build_gcc	2008-02-25 16:03:50.000000000 -0500
@@ -106,11 +106,12 @@
 CONFIGFLAGS="--disable-checking -enable-werror \
   --prefix=$DEST_ROOT \
   --mandir=\${prefix}/share/man \
-  --enable-languages=c,objc,c++,obj-c++ \
+  --enable-languages=c,objc,c++,obj-c++,d \
   --program-transform-name=/^[cg][^.-]*$/s/$/-$MAJ_VERS/ \
   --with-gxx-include-dir=\${prefix}/include/c++/$LIBSTDCXX_VERSION \
   --with-slibdir=/usr/lib \
-  --build=$BUILD-apple-darwin$DARWIN_VERS"
+  --build=$BUILD-apple-darwin$DARWIN_VERS \
+    $EXTRA_CONFIG_FLAGS"
 
 # Figure out how many make processes to run.
 SYSCTL=`sysctl -n hw.activecpu`
@@ -140,7 +141,7 @@
    --host=$BUILD-apple-darwin$DARWIN_VERS --target=$BUILD-apple-darwin$DARWIN_VERS || exit 1
 fi
 make $MAKEFLAGS $BOOTSTRAP CFLAGS="$CFLAGS" CXXFLAGS="$CFLAGS" || exit 1
-make $MAKEFLAGS html CFLAGS="$CFLAGS" CXXFLAGS="$CFLAGS" || exit 1
+#make $MAKEFLAGS html CFLAGS="$CFLAGS" CXXFLAGS="$CFLAGS" || exit 1
 make $MAKEFLAGS DESTDIR=$DIR/dst-$BUILD-$BUILD install-gcc install-target \
   CFLAGS="$CFLAGS" CXXFLAGS="$CFLAGS" || exit 1
 
@@ -237,9 +238,9 @@
 rm -rf * || exit 1
 
 # HTML documentation
-HTMLDIR="/Developer/ADC Reference Library/documentation/DeveloperTools"
-mkdir -p ".$HTMLDIR" || exit 1
-cp -Rp $DIR/obj-$BUILD-$BUILD/gcc/HTML/* ".$HTMLDIR/" || exit 1
+#HTMLDIR="/Developer/ADC Reference Library/documentation/DeveloperTools"
+#mkdir -p ".$HTMLDIR" || exit 1
+#cp -Rp $DIR/obj-$BUILD-$BUILD/gcc/HTML/* ".$HTMLDIR/" || exit 1
 
 # Manual pages
 mkdir -p .$DEST_ROOT/share || exit 1
@@ -269,7 +270,7 @@
 # bin
 # The native drivers ('native' is different in different architectures).
 BIN_FILES=`ls $DIR/dst-$BUILD-$BUILD$DEST_ROOT/bin | grep '^[^-]*-[0-9.]*$' \
-  | grep -v gccbug | grep -v gcov || exit 1`
+  | grep -v gccbug | grep -v gcov | grep -v gdmd || exit 1`
 mkdir .$DEST_ROOT/bin
 for f in $BIN_FILES ; do
   lipo -output .$DEST_ROOT/bin/$f -create $DIR/dst-*$DEST_ROOT/bin/$f || exit 1
@@ -285,6 +286,8 @@
     $DIR/dst-*-$t$DEST_ROOT/bin/$t-apple-darwin$DARWIN_VERS-gcc-$VERS || exit 1
   lipo -output .$DEST_ROOT/bin/$t-apple-darwin$DARWIN_VERS-g++-$VERS -create \
     $DIR/dst-*-$t$DEST_ROOT/bin/$t-apple-darwin$DARWIN_VERS-g++* || exit 1
+  lipo -output .$DEST_ROOT/bin/$t-apple-darwin$DARWIN_VERS-gdc-$VERS -create \
+    $DIR/dst-*-$t$DEST_ROOT/bin/$t-apple-darwin$DARWIN_VERS-gdc* || exit 1
 done
 
 # lib
@@ -358,6 +361,42 @@
   fi
 done
 
+# GDC: Phobos and gdmd
+if test -e $DIR/dst-$BUILD-$BUILD$DEST_ROOT/include/d2; then
+    phobos_sfx=2;
+else
+    phobos_sfx='';
+fi
+cp -p $DIR/dst-$BUILD-$BUILD$DEST_ROOT/bin/gdmd-* $DEST_DIR$DEST_ROOT/bin
+cp -pR $DIR/dst-$BUILD-$BUILD$DEST_ROOT/include/d$phobos_sfx $DEST_DIR$DEST_ROOT/include \
+    || exit 1
+phobos_lib_list=
+for t in $TARGETS ; do
+    if test $t = $BUILD ; then
+	phobos_lib_dir="$DIR/dst-$BUILD-$t$DEST_ROOT/lib"
+    else
+	cp -pR $DIR/dst-$BUILD-$t$DEST_ROOT/include/d$phobos_sfx/$VERS/$t-apple-darwin$DARWIN_VERS \
+	    $DEST_DIR$DEST_ROOT/include/d$phobos_sfx/$VERS || exit 1
+	phobos_lib_dir="$DIR/dst-$BUILD-$t$DEST_ROOT/$t-apple-darwin$DARWIN_VERS/lib"
+    fi
+    phobos_lib_list="$phobos_lib_list $phobos_lib_dir/libgphobos$phobos_sfx.a"
+
+    # Get 64-bit variants
+    other_libdir=''
+    if test "$t" = i686; then
+	other_libdir="x86_64"
+    elif test "$t" = powerpc; then
+	other_libdir="ppc64"
+    fi
+    if test -n "$other_libdir"; then
+	other_phobos="$phobos_lib_dir/$other_libdir/libgphobos$phobos_sfx.a"
+	if test -r "$other_phobos"; then
+	    phobos_lib_list="$phobos_lib_list $other_phobos"
+	fi
+    fi
+done
+lipo -output $DEST_DIR$DEST_ROOT/lib/libgphobos$phobos_sfx.a -create $phobos_lib_list || exit 1
+
 # Add extra man page symlinks for 'c++' and for arch-specific names.
 MDIR=$DEST_DIR$DEST_ROOT/share/man/man1
 ln -f $MDIR/g++-$MAJ_VERS.1 $MDIR/c++-$MAJ_VERS.1 || exit 1
@@ -389,6 +428,16 @@
 	-L$DIR/dst-$BUILD-$h$DEST_ROOT/$h-apple-darwin$DARWIN_VERS/lib/                    \
         -L$DIR/obj-$h-$BUILD/libiberty/                                        \
 	-o $DEST_DIR/$DEST_ROOT/bin/tmp-$h-g++-$MAJ_VERS || exit 1
+
+    $DEST_DIR$DEST_ROOT/bin/$h-apple-darwin$DARWIN_VERS-gcc-$VERS                          \
+	$ORIG_SRC_DIR/gcc/config/darwin-driver.c                               \
+	-DPDN="\"-apple-darwin$DARWIN_VERS-gdc-$VERS\""                                    \
+	-DIL="\"$DEST_ROOT/bin/\"" -I  $ORIG_SRC_DIR/include                   \
+	-I  $ORIG_SRC_DIR/gcc -I  $ORIG_SRC_DIR/gcc/config                     \
+	-liberty -L$DIR/dst-$BUILD-$h$DEST_ROOT/lib/                           \
+	-L$DIR/dst-$BUILD-$h$DEST_ROOT/$h-apple-darwin$DARWIN_VERS/lib/                    \
+        -L$DIR/obj-$h-$BUILD/libiberty/                                        \
+	-o $DEST_DIR/$DEST_ROOT/bin/tmp-$h-gdc-$MAJ_VERS || exit 1
 done
 
 lipo -output $DEST_DIR/$DEST_ROOT/bin/gcc-$MAJ_VERS -create \
@@ -397,11 +446,24 @@
 lipo -output $DEST_DIR/$DEST_ROOT/bin/g++-$MAJ_VERS -create \
   $DEST_DIR/$DEST_ROOT/bin/tmp-*-g++-$MAJ_VERS || exit 1
 
+lipo -output $DEST_DIR/$DEST_ROOT/bin/gdc-$MAJ_VERS -create \
+  $DEST_DIR/$DEST_ROOT/bin/tmp-*-gdc-$MAJ_VERS || exit 1
+
 ln -f $DEST_DIR/$DEST_ROOT/bin/g++-$MAJ_VERS $DEST_DIR/$DEST_ROOT/bin/c++-$MAJ_VERS || exit 1
 
 rm $DEST_DIR/$DEST_ROOT/bin/tmp-*-gcc-$MAJ_VERS || exit 1
 rm $DEST_DIR/$DEST_ROOT/bin/tmp-*-g++-$MAJ_VERS || exit 1
+rm $DEST_DIR/$DEST_ROOT/bin/tmp-*-gdc-$MAJ_VERS || exit 1
 
+# Build rdmd
+for h in $HOSTS ; do
+    $DEST_DIR$DEST_ROOT/bin/$h-apple-darwin$DARWIN_VERS-gdc-$VERS                          \
+	$ORIG_SRC_DIR/gcc/d/rdmd.d	\
+	-o $DEST_DIR/$DEST_ROOT/bin/tmp-$h-rdmd || exit 1
+done
+lipo -output $DEST_DIR/$DEST_ROOT/bin/rdmd -create \
+  $DEST_DIR/$DEST_ROOT/bin/tmp-*-rdmd || exit 1
+rm $DEST_DIR/$DEST_ROOT/bin/tmp-*-rdmd || exit 1
 
 ########################################
 # Save the source files and objects needed for debugging
