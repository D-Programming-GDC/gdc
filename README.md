[![Build Status](https://semaphoreci.com/api/v1/projects/3938c9ef-a0bf-497f-876c-3e722c051ace/464583/badge.svg)](https://semaphoreci.com/d-programming-gdc/gdc)

### What is GDC?

GDC is a GPL implementation of the D compiler which integrates the open source D front end with GCC.

The GNU D Compiler (GDC) project was originally started by David Friedman in 2004 until early 2007 when he disappeared from the D scene, and was no longer able to maintain GDC. Following a revival attempt in 2008, GDC is now under the lead of Iain Buclaw who has been steering the project since 2009 with the assistance of its contributors, without them the project would not have been nearly as successful as it has been.

Documentation on GDC is available from [the wiki][wiki]. Any bugs or issues found with using GDC should be reported at [our bugzilla site][bugs]. For help with GDC, the [D.gnu][maillist] mailing list is the place to go with questions or problems.

Work is currently under way to merge GDC into a future release of GCC. Any assitance of any sort during this time would be invaluably appreciated. Feel free to contact via [email][email] or join in at #d.gdc on FreeNode.

### Building

The master branch of this project closely follows [GCC development branch][gcc-devel], which if you intend to use GDC for production applications, is likely not the version of GCC you want to build against.  For versions of GDC compatible with GCC releases, checkout one of the following branches:

* [GCC 4.7.x](https://github.com/D-Programming-GDC/GDC/tree/gdc-4.7)
* [GCC 4.8.x](https://github.com/D-Programming-GDC/GDC/tree/gdc-4.8)
* [GCC 4.9.x](https://github.com/D-Programming-GDC/GDC/tree/gdc-4.9)
* [GCC 5.x](https://github.com/D-Programming-GDC/GDC/tree/gdc-5)

[home]: http://gdcproject.org
[wiki]: http://wiki.dlang.org/GDC
[bugs]: http://bugzilla.gdcproject.org
[maillist]: http://forum.dlang.org/group/D.gnu
[email]: mailto:ibuclaw@gdcproject.org
[gcc-devel]: http://gcc.gnu.org/git/?p=gcc.git;a=shortlog
