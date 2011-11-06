/* In C, the stdio/stdlib function to use are determined by a test in cdefs.h.
   There is another test for math functions in architecture/ppc/math.h which
   is reproduced, in spirit, here.  This one test controls both stdio/stdlib and
   math functions for D. */

module gcc.ldblcompat;

version (darwin)
{
    version (PPC)
        version = GNU_CheckLongDoubleFormat;
}
else
    version = GNU_CheckLongDoubleFormat;


version (GNU_CheckLongDoubleFormat)
{
    version (GNU_WantLongDoubleFormat128)
        version = GNU_UseLongDoubleFormat128;
    else version (GNU_WantLongDoubleFormat64)
        { }
    else
    {
        version (GNU_LongDouble128)
            version = GNU_UseLongDoubleFormat128;
    }
}

version (GNU_UseLongDoubleFormat128)
{
    version (darwin)
    {
        // Currently, the following test from cdefs.h is not supported:
        //# if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__-0 < 1040
        version (all)
            invariant __DARWIN_LDBL_COMPAT  = "$LDBL128";
        else
            invariant __DARWIN_LDBL_COMPAT  = "$LDBLStub";
        invariant __DARWIN_LDBL_COMPAT2 = "$LDBL128";

        invariant __LIBMLDBL_COMPAT = "$LDBL128";
    }
    else
    {
        static const bool __No_Long_Double_Math = false;
        const char[] __LDBL_COMPAT_PFX = "";
    }
}
else
{
    version (darwin)
    {
        invariant __DARWIN_LDBL_COMPAT  = "";
        invariant __DARWIN_LDBL_COMPAT2 = "";
        invariant __LIBMLDBL_COMPAT = "";
    }
    else
    {
        static const bool __No_Long_Double_Math = true;
        const char[] __LDBL_COMPAT_PFX = "__nldbl_";
    }
}
