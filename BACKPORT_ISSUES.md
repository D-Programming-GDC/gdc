### Known issues in this backport version

These are known issues which can only be fixed in the GCC backend and are therefore only fixed in newer GCC/GDC versions:

* Thunk generation is limited for GCC <= 4.9 [(1)][1] [(2)][2]
* Certain external template functions cause an ICE when using debug info in GCC <= 4.9. Debug info generation is disabled when building libphobos because of this issue. [(3)][3]
* GCC bug 71762 can be triggered when using GDC 4.9. GDC 4.8 and GDC 5 should be affected as well, but do not trigger the bug in the test suite [(4)][4]

[1]: https://github.com/D-Programming-GDC/GDC/commit/50d87f28208341aff7e32939cea9cb4e72bce6ac##commitcomment-21212855
[2]: https://bugzilla.gdcproject.org/show_bug.cgi?id=27
[3]: https://bugzilla.gdcproject.org/show_bug.cgi?id=157
[4]: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71762
