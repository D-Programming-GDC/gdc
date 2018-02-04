/**
 * D header file for C99.
 *
 * $(C_HEADER_DESCRIPTION pubs.opengroup.org/onlinepubs/009695399/basedefs/_stdarg.h.html, _stdarg.h)
 *
 * Copyright: Copyright Digital Mars 2000 - 2009.
 * License:   $(WEB www.boost.org/LICENSE_1_0.txt, Boost License 1.0).
 * Authors:   Walter Bright, Hauke Duden
 * Standards: ISO/IEC 9899:1999 (E)
 * Source: $(DRUNTIMESRC core/stdc/_stdarg.d)
 */

/* NOTE: This file has been patched from the original DMD distribution to
 * work with the GDC compiler.
 */
module core.stdc.stdarg;

@system:
//@nogc:    // Not yet, need to make TypeInfo's member functions @nogc first
nothrow:

version( GNU )
{
    import gcc.builtins;
    alias __builtin_va_list __gnuc_va_list;


    /*********************
     * The argument pointer type.
     */
    alias __gnuc_va_list va_list;


    /**********
     * Initialize ap.
     * parmn should be the last named parameter.
     */
    void va_start(T)(out va_list ap, ref T parmn);


    /************
     * Retrieve and return the next value that is type T.
     */
    T va_arg(T)(ref va_list ap);


    /*************
     * Retrieve and store through parmn the next value that is of type T.
     */
    void va_arg(T)(ref va_list ap, ref T parmn);


    /*************
     * Retrieve and store through parmn the next value that is of TypeInfo ti.
     * Used when the static type is not known.
     */
    version( X86 )
    {
        ///
        void va_arg()(ref va_list ap, TypeInfo ti, void* parmn)
        {
            auto p = ap;
            auto tsize = ti.tsize;
            ap = cast(va_list)(cast(size_t)p + ((tsize + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
            parmn[0..tsize] = p[0..tsize];
        }
    }
    else version( X86_64 )
    {
        /// Layout of this struct must match __builtin_va_list for C ABI compatibility
        struct __va_list
        {
            uint offset_regs = 6 * 8;            // no regs
            uint offset_fpregs = 6 * 8 + 8 * 16; // no fp regs
            void* stack_args;
            void* reg_args;
        }

        ///
        void va_arg()(ref va_list apx, TypeInfo ti, void* parmn)
        {
            __va_list* ap = cast(__va_list*)apx;
            TypeInfo arg1, arg2;
            if (!ti.argTypes(arg1, arg2))
            {
                bool inXMMregister(TypeInfo arg) pure nothrow @safe
                {
                    return (arg.flags & 2) != 0;
                }

                TypeInfo_Vector v1 = arg1 ? cast(TypeInfo_Vector)arg1 : null;
                if (arg1 && (arg1.tsize() <= 8 || v1))
                {   // Arg is passed in one register
                    auto tsize = arg1.tsize();
                    void* p;
                    bool stack = false;
                    auto offset_fpregs_save = ap.offset_fpregs;
                    auto offset_regs_save = ap.offset_regs;
                L1:
                    if (inXMMregister(arg1) || v1)
                    {   // Passed in XMM register
                        if (ap.offset_fpregs < (6 * 8 + 16 * 8) && !stack)
                        {
                            p = ap.reg_args + ap.offset_fpregs;
                            ap.offset_fpregs += 16;
                        }
                        else
                        {
                            p = ap.stack_args;
                            ap.stack_args += (tsize + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                            stack = true;
                        }
                    }
                    else
                    {   // Passed in regular register
                        if (ap.offset_regs < 6 * 8 && !stack)
                        {
                            p = ap.reg_args + ap.offset_regs;
                            ap.offset_regs += 8;
                        }
                        else
                        {
                            p = ap.stack_args;
                            ap.stack_args += 8;
                            stack = true;
                        }
                    }
                    parmn[0..tsize] = p[0..tsize];

                    if (arg2)
                    {
                        if (inXMMregister(arg2))
                        {   // Passed in XMM register
                            if (ap.offset_fpregs < (6 * 8 + 16 * 8) && !stack)
                            {
                                p = ap.reg_args + ap.offset_fpregs;
                                ap.offset_fpregs += 16;
                            }
                            else
                            {
                                if (!stack)
                                {   // arg1 is really on the stack, so rewind and redo
                                    ap.offset_fpregs = offset_fpregs_save;
                                    ap.offset_regs = offset_regs_save;
                                    stack = true;
                                    goto L1;
                                }
                                p = ap.stack_args;
                                ap.stack_args += (arg2.tsize() + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                            }
                        }
                        else
                        {   // Passed in regular register
                            if (ap.offset_regs < 6 * 8 && !stack)
                            {
                                p = ap.reg_args + ap.offset_regs;
                                ap.offset_regs += 8;
                            }
                            else
                            {
                                if (!stack)
                                {   // arg1 is really on the stack, so rewind and redo
                                    ap.offset_fpregs = offset_fpregs_save;
                                    ap.offset_regs = offset_regs_save;
                                    stack = true;
                                    goto L1;
                                }
                                p = ap.stack_args;
                                ap.stack_args += 8;
                            }
                        }
                        auto sz = ti.tsize() - 8;
                        (parmn + 8)[0..sz] = p[0..sz];
                    }
                }
                else
                {   // Always passed in memory
                    // The arg may have more strict alignment than the stack
                    auto talign = ti.talign();
                    auto tsize = ti.tsize();
                    auto p = cast(void*)((cast(size_t)ap.stack_args + talign - 1) & ~(talign - 1));
                    ap.stack_args = cast(void*)(cast(size_t)p + ((tsize + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
                    parmn[0..tsize] = p[0..tsize];
                }
            }
            else
            {
                assert(false, "not a valid argument type for va_arg");
            }
        }
    }
    else version( ARM )
    {
        ///
        void va_arg()(ref va_list ap, TypeInfo ti, void* parmn)
        {
            auto p = *cast(void**) &ap;
            auto tsize = ti.tsize();
            *cast(void**) &ap += ( tsize + size_t.sizeof - 1 ) & ~( size_t.sizeof - 1 );
            parmn[0..tsize] = p[0..tsize];
        }
    }
    else version( AArch64 )
    {
        // Note: SoftFloat support is not in the ARM documents but apparently
        // supported in some GCC versions. Not tested!

        private pure @property bool inGeneralRegister(const TypeInfo ti)
        {
            // Float types and HAs are always in SIMD register (or stack in SoftFloat)
            // Integers, pointers and composite types up to 64 byte are
            // passed in registers
            if (ti.argTypes.isFloat)
                return false;
            else if (ti.argTypes.isHA)
                return false;
            else
                return ti.tsize <= 64;
        }

        private pure @property bool inFloatRegister(const TypeInfo ti)
        {
            version (D_SoftFloat)
                return false;
            else
                return ti.argTypes.isFloat;
        }

        private pure @property bool isHA(const TypeInfo ti)
        {
            version (D_SoftFloat)
                return false;
            else
                return ti.argTypes.isHA;
        }

        private pure @property bool passByReference(const TypeInfo ti)
        {
            // See aarch64_pass_by_reference
            if (ti.inFloatRegister || ti.isHA)
                return false;
            else
                return ti.tsize > 16;
        }

        /// Layout of this struct must match __builtin_va_list for C ABI compatibility
        struct __va_list
        {
            void* __stack;
            void* __gr_top;
            void* __vr_top;
            int __gr_offs;
            int __vr_offs;
        }

        ///
        // This code is based on the pseudo code in 'Procedure call standard
        // for the ARM 64-bit architecture' release 1.0, 22.05.13
        void va_arg()(ref va_list apx, TypeInfo ti, void* parmn)
        {
            import core.stdc.stdint : intptr_t;

            __va_list* ap = cast(__va_list*)&apx;
            auto uparmn = cast(ubyte*) parmn;

            void assignValue(void* pFrom)
            {
                auto len = ti.tsize;
                uparmn[0 .. len] = (cast(ubyte*) pFrom)[0 .. len];
            }

            void readFromStack()
            {
                intptr_t arg = cast(intptr_t) ap.__stack;
                // round up
                if (ti.talign > 8)
                    arg = (arg + 15) & -16;

                ap.__stack = cast(void*)((arg + ti.tsize + 7) & -8);
                version (BigEndian)
                {
                    if (!ti.argTypes.isAggregate && ti.tsize < 8)
                        arg += 8 - ti.tsize;
                }
                return assignValue(cast(void*) arg);
            }

            void readReferenceFromStack()
            {
                intptr_t arg = cast(intptr_t) ap.__stack;

                ap.__stack = cast(void*)((arg + 8 + 7) & -8);
                auto len = ti.tsize;
                uparmn[0 .. len] = (*cast(ubyte**)(arg))[0 .. len];
            }

            // Note: This code is missing in the AArch64 PCS pseudo code
            if (ti.passByReference)
            {
                // Passed as a reference => always a 8 byte pointer
                auto offs = ap.__gr_offs;
                // reg save area empty
                if (offs >= 0)
                    return readReferenceFromStack();

                ap.__gr_offs = offs + 8;
                // overflowed reg save area
                if (ap.__gr_offs > 0)
                    return readReferenceFromStack();

                auto len = ti.tsize;
                uparmn[0 .. len] = (*cast(ubyte**)(ap.__gr_top + offs))[0 .. len];
            }
            else if (ti.inGeneralRegister)
            {
                auto offs = ap.__gr_offs;
                // reg save area empty
                if (offs >= 0)
                    return readFromStack();
                // round up
                if (ti.talign > 8)
                    offs = (offs + 15) & -16;

                auto nreg = cast(int)((ti.tsize + 7) / 8);
                ap.__gr_offs = offs + (nreg * 8);
                // overflowed reg save area
                if (ap.__gr_offs > 0)
                    return readFromStack();

                version (BigEndian)
                {
                    if (!ti.argTypes.isAggregate && ti.tsize < 8)
                        offs += 8 - ti.tsize;
                }
                return assignValue(ap.__gr_top + offs);
            }
            else if (ti.isHA)
            {
                auto offs = ap.__vr_offs;
                // reg save area empty
                if (offs >= 0)
                    return readFromStack();

                auto nreg = ti.argTypes.haFieldNum;
                ap.__vr_offs = offs + (nreg * 16);
                // overflowed reg save area
                if (ap.__vr_offs > 0)
                    return readFromStack();

                version (BigEndian)
                {
                    if (ti.argTypes.haFieldSize < 16)
                        offs += 16 - ti.argTypes.haFieldSize;
                }

                size_t pos = 0;
                for (size_t i = 0; i < nreg; i++)
                {
                    uparmn[pos .. pos + ti.argTypes.haFieldSize] = (
                            cast(ubyte*)(ap.__vr_top + offs))[0 .. ti.argTypes.haFieldSize];

                    pos += ti.argTypes.haFieldSize;
                    offs += 16;
                }
            }
            else if (ti.inFloatRegister)
            {
                auto offs = ap.__vr_offs;
                // reg save area empty
                if (offs >= 0)
                    return readFromStack();

                auto nreg = cast(int)((ti.tsize + 15) / 16);
                ap.__vr_offs = offs + (nreg * 16);
                // overflowed reg save area
                if (ap.__vr_offs > 0)
                    return readFromStack();

                version (BigEndian)
                {
                    if (!ti.argTypes.isAggregate && ti.tsize < 16)
                        offs += 16 - ti.tsize;
                }
                return assignValue(ap.__vr_top + offs);
            }
            else
            {
                return readFromStack();
            }
        }
    }
    else
    {
        ///
        void va_arg()(ref va_list ap, TypeInfo ti, void* parmn)
        {
            static assert(false, "Unsupported platform");
        }
    }

    /***********************
     * End use of ap.
     */
    alias __builtin_va_end va_end;

    /***********************
     * Make a copy of ap.
     */
    alias __builtin_va_copy va_copy;

}
else version( X86 )
{
    /*********************
     * The argument pointer type.
     */
    alias char* va_list;

    /**********
     * Initialize ap.
     * For 32 bit code, parmn should be the last named parameter.
     * For 64 bit code, parmn should be __va_argsave.
     */
    void va_start(T)(out va_list ap, ref T parmn)
    {
        ap = cast(va_list)( cast(void*) &parmn + ( ( T.sizeof + int.sizeof - 1 ) & ~( int.sizeof - 1 ) ) );
    }

    /************
     * Retrieve and return the next value that is type T.
     * Should use the other va_arg instead, as this won't work for 64 bit code.
     */
    T va_arg(T)(ref va_list ap)
    {
        T arg = *cast(T*) ap;
        ap = cast(va_list)( cast(void*) ap + ( ( T.sizeof + int.sizeof - 1 ) & ~( int.sizeof - 1 ) ) );
        return arg;
    }

    /************
     * Retrieve and return the next value that is type T.
     * This is the preferred version.
     */
    void va_arg(T)(ref va_list ap, ref T parmn)
    {
        parmn = *cast(T*)ap;
        ap = cast(va_list)(cast(void*)ap + ((T.sizeof + int.sizeof - 1) & ~(int.sizeof - 1)));
    }

    /*************
     * Retrieve and store through parmn the next value that is of TypeInfo ti.
     * Used when the static type is not known.
     */
    void va_arg()(ref va_list ap, TypeInfo ti, void* parmn)
    {
        // Wait until everyone updates to get TypeInfo.talign
        //auto talign = ti.talign;
        //auto p = cast(void*)(cast(size_t)ap + talign - 1) & ~(talign - 1);
        auto p = ap;
        auto tsize = ti.tsize;
        ap = cast(va_list)(cast(size_t)p + ((tsize + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
        parmn[0..tsize] = p[0..tsize];
    }

    /***********************
     * End use of ap.
     */
    void va_end(va_list ap)
    {
    }

    ///
    void va_copy(out va_list dest, va_list src)
    {
        dest = src;
    }
}
else version (Windows) // Win64
{   /* Win64 is characterized by all arguments fitting into a register size.
     * Smaller ones are padded out to register size, and larger ones are passed by
     * reference.
     */

    /*********************
     * The argument pointer type.
     */
    alias char* va_list;

    /**********
     * Initialize ap.
     * parmn should be the last named parameter.
     */
    void va_start(T)(out va_list ap, ref T parmn); // Compiler intrinsic

    /************
     * Retrieve and return the next value that is type T.
     */
    T va_arg(T)(ref va_list ap)
    {
        static if (T.sizeof > size_t.sizeof)
            T arg = **cast(T**)ap;
        else
            T arg = *cast(T*)ap;
        ap = cast(va_list)(cast(void*)ap + ((size_t.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
        return arg;
    }

    /************
     * Retrieve and return the next value that is type T.
     * This is the preferred version.
     */
    void va_arg(T)(ref va_list ap, ref T parmn)
    {
        static if (T.sizeof > size_t.sizeof)
            parmn = **cast(T**)ap;
        else
            parmn = *cast(T*)ap;
        ap = cast(va_list)(cast(void*)ap + ((size_t.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
    }

    /*************
     * Retrieve and store through parmn the next value that is of TypeInfo ti.
     * Used when the static type is not known.
     */
    void va_arg()(ref va_list ap, TypeInfo ti, void* parmn)
    {
        // Wait until everyone updates to get TypeInfo.talign
        //auto talign = ti.talign;
        //auto p = cast(void*)(cast(size_t)ap + talign - 1) & ~(talign - 1);
        auto p = ap;
        auto tsize = ti.tsize;
        ap = cast(va_list)(cast(size_t)p + ((size_t.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
        void* q = (tsize > size_t.sizeof) ? *cast(void**)p : p;
        parmn[0..tsize] = q[0..tsize];
    }

    /***********************
     * End use of ap.
     */
    void va_end(va_list ap)
    {
    }

    ///
    void va_copy(out va_list dest, va_list src)
    {
        dest = src;
    }
}
else version (X86_64)
{
    // Determine if type is a vector type
    template isVectorType(T)
    {
        enum isVectorType = false;
    }

    template isVectorType(T : __vector(T[N]), size_t N)
    {
        enum isVectorType = true;
    }

    // Layout of this struct must match __gnuc_va_list for C ABI compatibility
    struct __va_list_tag
    {
        uint offset_regs = 6 * 8;            // no regs
        uint offset_fpregs = 6 * 8 + 8 * 16; // no fp regs
        void* stack_args;
        void* reg_args;
    }
    alias __va_list = __va_list_tag;

    align(16) struct __va_argsave_t
    {
        size_t[6] regs;   // RDI,RSI,RDX,RCX,R8,R9
        real[8] fpregs;   // XMM0..XMM7
        __va_list va;
    }

    /*
     * Making it an array of 1 causes va_list to be passed as a pointer in
     * function argument lists
     */
    alias va_list = __va_list*;

    ///
    void va_start(T)(out va_list ap, ref T parmn); // Compiler intrinsic

    ///
    T va_arg(T)(va_list ap)
    {   T a;
        va_arg(ap, a);
        return a;
    }

    ///
    void va_arg(T)(va_list apx, ref T parmn)
    {
        __va_list* ap = cast(__va_list*)apx;
        static if (is(T U == __argTypes))
        {
            static if (U.length == 0 || T.sizeof > 16 || (U[0].sizeof > 8 && !isVectorType!(U[0])))
            {   // Always passed in memory
                // The arg may have more strict alignment than the stack
                auto p = (cast(size_t)ap.stack_args + T.alignof - 1) & ~(T.alignof - 1);
                ap.stack_args = cast(void*)(p + ((T.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
                parmn = *cast(T*)p;
            }
            else static if (U.length == 1)
            {   // Arg is passed in one register
                alias U[0] T1;
                static if (is(T1 == double) || is(T1 == float) || isVectorType!(T1))
                {   // Passed in XMM register
                    if (ap.offset_fpregs < (6 * 8 + 16 * 8))
                    {
                        parmn = *cast(T*)(ap.reg_args + ap.offset_fpregs);
                        ap.offset_fpregs += 16;
                    }
                    else
                    {
                        parmn = *cast(T*)ap.stack_args;
                        ap.stack_args += (T1.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                    }
                }
                else
                {   // Passed in regular register
                    if (ap.offset_regs < 6 * 8 && T.sizeof <= 8)
                    {
                        parmn = *cast(T*)(ap.reg_args + ap.offset_regs);
                        ap.offset_regs += 8;
                    }
                    else
                    {
                        auto p = (cast(size_t)ap.stack_args + T.alignof - 1) & ~(T.alignof - 1);
                        ap.stack_args = cast(void*)(p + ((T.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
                        parmn = *cast(T*)p;
                    }
                }
            }
            else static if (U.length == 2)
            {   // Arg is passed in two registers
                alias U[0] T1;
                alias U[1] T2;
                auto p = cast(void*)&parmn + 8;

                // Both must be in registers, or both on stack, hence 4 cases

                static if ((is(T1 == double) || is(T1 == float)) &&
                           (is(T2 == double) || is(T2 == float)))
                {
                    if (ap.offset_fpregs < (6 * 8 + 16 * 8) - 16)
                    {
                        *cast(T1*)&parmn = *cast(T1*)(ap.reg_args + ap.offset_fpregs);
                        *cast(T2*)p = *cast(T2*)(ap.reg_args + ap.offset_fpregs + 16);
                        ap.offset_fpregs += 32;
                    }
                    else
                    {
                        *cast(T1*)&parmn = *cast(T1*)ap.stack_args;
                        ap.stack_args += (T1.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                        *cast(T2*)p = *cast(T2*)ap.stack_args;
                        ap.stack_args += (T2.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                    }
                }
                else static if (is(T1 == double) || is(T1 == float))
                {
                    void* a = void;
                    if (ap.offset_fpregs < (6 * 8 + 16 * 8) &&
                        ap.offset_regs < 6 * 8 && T2.sizeof <= 8)
                    {
                        *cast(T1*)&parmn = *cast(T1*)(ap.reg_args + ap.offset_fpregs);
                        ap.offset_fpregs += 16;
                        a = ap.reg_args + ap.offset_regs;
                        ap.offset_regs += 8;
                    }
                    else
                    {
                        *cast(T1*)&parmn = *cast(T1*)ap.stack_args;
                        ap.stack_args += (T1.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                        a = ap.stack_args;
                        ap.stack_args += 8;
                    }
                    // Be careful not to go past the size of the actual argument
                    const sz2 = T.sizeof - 8;
                    p[0..sz2] = a[0..sz2];
                }
                else static if (is(T2 == double) || is(T2 == float))
                {
                    if (ap.offset_regs < 6 * 8 && T1.sizeof <= 8 &&
                        ap.offset_fpregs < (6 * 8 + 16 * 8))
                    {
                        *cast(T1*)&parmn = *cast(T1*)(ap.reg_args + ap.offset_regs);
                        ap.offset_regs += 8;
                        *cast(T2*)p = *cast(T2*)(ap.reg_args + ap.offset_fpregs);
                        ap.offset_fpregs += 16;
                    }
                    else
                    {
                        *cast(T1*)&parmn = *cast(T1*)ap.stack_args;
                        ap.stack_args += 8;
                        *cast(T2*)p = *cast(T2*)ap.stack_args;
                        ap.stack_args += (T2.sizeof + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                    }
                }
                else // both in regular registers
                {
                    void* a = void;
                    if (ap.offset_regs < 5 * 8 && T1.sizeof <= 8 && T2.sizeof <= 8)
                    {
                        *cast(T1*)&parmn = *cast(T1*)(ap.reg_args + ap.offset_regs);
                        ap.offset_regs += 8;
                        a = ap.reg_args + ap.offset_regs;
                        ap.offset_regs += 8;
                    }
                    else
                    {
                        *cast(T1*)&parmn = *cast(T1*)ap.stack_args;
                        ap.stack_args += 8;
                        a = ap.stack_args;
                        ap.stack_args += 8;
                    }
                    // Be careful not to go past the size of the actual argument
                    const sz2 = T.sizeof - 8;
                    p[0..sz2] = a[0..sz2];
                }
            }
            else
            {
                static assert(false);
            }
        }
        else
        {
            static assert(false, "not a valid argument type for va_arg");
        }
    }

    ///
    void va_arg()(va_list apx, TypeInfo ti, void* parmn)
    {
        __va_list* ap = cast(__va_list*)apx;
        TypeInfo arg1, arg2;
        if (!ti.argTypes(arg1, arg2))
        {
            bool inXMMregister(TypeInfo arg) pure nothrow @safe
            {
                return (arg.flags & 2) != 0;
            }

            TypeInfo_Vector v1 = arg1 ? cast(TypeInfo_Vector)arg1 : null;
            if (arg1 && (arg1.tsize <= 8 || v1))
            {   // Arg is passed in one register
                auto tsize = arg1.tsize;
                void* p;
                bool stack = false;
                auto offset_fpregs_save = ap.offset_fpregs;
                auto offset_regs_save = ap.offset_regs;
            L1:
                if (inXMMregister(arg1) || v1)
                {   // Passed in XMM register
                    if (ap.offset_fpregs < (6 * 8 + 16 * 8) && !stack)
                    {
                        p = ap.reg_args + ap.offset_fpregs;
                        ap.offset_fpregs += 16;
                    }
                    else
                    {
                        p = ap.stack_args;
                        ap.stack_args += (tsize + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                        stack = true;
                    }
                }
                else
                {   // Passed in regular register
                    if (ap.offset_regs < 6 * 8 && !stack)
                    {
                        p = ap.reg_args + ap.offset_regs;
                        ap.offset_regs += 8;
                    }
                    else
                    {
                        p = ap.stack_args;
                        ap.stack_args += 8;
                        stack = true;
                    }
                }
                parmn[0..tsize] = p[0..tsize];

                if (arg2)
                {
                    if (inXMMregister(arg2))
                    {   // Passed in XMM register
                        if (ap.offset_fpregs < (6 * 8 + 16 * 8) && !stack)
                        {
                            p = ap.reg_args + ap.offset_fpregs;
                            ap.offset_fpregs += 16;
                        }
                        else
                        {
                            if (!stack)
                            {   // arg1 is really on the stack, so rewind and redo
                                ap.offset_fpregs = offset_fpregs_save;
                                ap.offset_regs = offset_regs_save;
                                stack = true;
                                goto L1;
                            }
                            p = ap.stack_args;
                            ap.stack_args += (arg2.tsize + size_t.sizeof - 1) & ~(size_t.sizeof - 1);
                        }
                    }
                    else
                    {   // Passed in regular register
                        if (ap.offset_regs < 6 * 8 && !stack)
                        {
                            p = ap.reg_args + ap.offset_regs;
                            ap.offset_regs += 8;
                        }
                        else
                        {
                            if (!stack)
                            {   // arg1 is really on the stack, so rewind and redo
                                ap.offset_fpregs = offset_fpregs_save;
                                ap.offset_regs = offset_regs_save;
                                stack = true;
                                goto L1;
                            }
                            p = ap.stack_args;
                            ap.stack_args += 8;
                        }
                    }
                    auto sz = ti.tsize - 8;
                    (parmn + 8)[0..sz] = p[0..sz];
                }
            }
            else
            {   // Always passed in memory
                // The arg may have more strict alignment than the stack
                auto talign = ti.talign;
                auto tsize = ti.tsize;
                auto p = cast(void*)((cast(size_t)ap.stack_args + talign - 1) & ~(talign - 1));
                ap.stack_args = cast(void*)(cast(size_t)p + ((tsize + size_t.sizeof - 1) & ~(size_t.sizeof - 1)));
                parmn[0..tsize] = p[0..tsize];
            }
        }
        else
        {
            assert(false, "not a valid argument type for va_arg");
        }
    }

    ///
    void va_end(va_list ap)
    {
    }

    import core.stdc.stdlib : alloca;

    ///
    void va_copy(out va_list dest, va_list src, void* storage = alloca(__va_list_tag.sizeof))
    {
        // Instead of copying the pointers, and aliasing the source va_list,
        // the default argument alloca will allocate storage in the caller's
        // stack frame.  This is still not correct (it should be allocated in
        // the place where the va_list variable is declared) but most of the
        // time the caller's stack frame _is_ the place where the va_list is
        // allocated, so in most cases this will now work.
        dest = cast(va_list)storage;
        *dest = *src;
    }
}
else
{
    static assert(false, "Unsupported platform");
}

private version (unittest)
{
    import core.stdc.stdarg, core.simd;
    import core.internal.traits : Unqual, staticMap;

    template isVectorType(T)
    {
        enum isVectorType = false;
    }

    template isVectorType(T : __vector(T[N]), size_t N)
    {
        enum isVectorType = true;
    }

    template vectorSize(T : __vector(T[N]), size_t N)
    {
        enum vectorSize = N;
    }

    bool compare(T)(T t1, T t2)
    {
        static if (isVectorType!T)
        {
            for (size_t i = 0; i < vectorSize!T; i++)
            {
                if (t1[i] != t2[i])
                    return false;
            }
            return true;
        }
        else
        {
            return t1 == t2;
        }
    }

    enum TestMode
    {
        C,
        D,
        both
    }

    template testTypeInfoVaArg(TestMode mode, CTArgs...)
    {
        staticMap!(Unqual, CTArgs) ctArgs;

        /*
         * This function implements the C-style variadic function.
         * The first arg in CTArgs is used as named parameter, the
         * remaining args are used as variadic parameters.
         */
        extern (C) void testTypeInfoC(CTArgs[0] p1, ...)
        {
            // Make sure our buffer is properly aligned
            union Buffer
            {
                ubyte[1024] buf;
                double4 vect;
            }

            Buffer buffu;

            // Check first parameter, just to be sure
            assert(compare(p1, ctArgs[0]));

            // Setup va_list
            va_list ap;
            va_start(ap, p1);

            // Compare all variadic arguments
            foreach (i, CTArg; CTArgs[1 .. $])
            {
                // Get TypeInfo for static type
                auto ti = typeid(CTArg);

                va_arg(ap, ti, buffu.buf.ptr);
                assert(compare(ctArgs[i + 1], *(cast(CTArg*) buffu.buf.ptr)));
            }
            va_end(ap);
        }

        /*
         * This function implements the C-style, extern(D) variadic function.
         * The first arg in CTArgs is used as named parameter, the
         * remaining args are used as variadic parameters.
         */
        void testTypeInfoD(...)
        {
            // Make sure our buffer is properly aligned
            union Buffer
            {
                ubyte[1024] buf;
                double4 vect;
            }

            Buffer buffu;

            // Compare all variadic arguments
            foreach (i, CTArg; CTArgs[1 .. $])
            {
                // Get TypeInfo
                auto ti = _arguments[i];

                va_arg(_argptr, ti, buffu.buf.ptr);
                assert(compare(ctArgs[i + 1], *(cast(CTArg*) buffu.buf.ptr)));
            }
        }

        void testTypeInfoVaArg(CTArgs lctArgs)
        {
            ctArgs = lctArgs;
            static if (mode == TestMode.C || mode == TestMode.both)
                testTypeInfoC(lctArgs[0], lctArgs[1 .. $]);
            static if (mode == TestMode.D || mode == TestMode.both)
                testTypeInfoD(lctArgs[1 .. $]);
        }
    }

    // An AArch64 HFA, passed in SIMD registers
    struct HFA1
    {
        double a, b;
    }

    // An AArch64 HFA, passed in SIMD registers
    struct HFA2
    {
        cdouble a;
        double b;
    }

    // An AArch64 HFA, passed in SIMD registers
    struct HFA3
    {
        HFA1 a;
        double b;
    }

    // An AArch64 HFA, passed in SIMD registers
    struct HFA4
    {
        union
        {
            double a1;
            double a2;
        }

        double b, c, d;

        this(double a, double b, double c, double d)
        {
            this.a1 = a;
            this.b = b;
            this.c = c;
            this.d = d;
        }
    }

    version (AArch64)
    {
        struct HVA1
        {
            float4 a, b;
            bool opEquals()(auto ref const HVA1 rhs) const
            {
                return compare(a, rhs.a) && compare(b, rhs.b);
            }
        }

        struct HVA2
        {
            float4 a;
            double2 b;

            bool opEquals()(auto ref const HVA2 rhs) const
            {
                return compare(a, rhs.a) && compare(b, rhs.b);
            }
        }

        struct HVA3
        {
            double4 a;
            double4 b;

            bool opEquals()(auto ref const HVA3 rhs) const
            {
                return compare(a, rhs.a) && compare(b, rhs.b);
            }
        }
    }

    struct Normal1
    {
        double a;
        float b;
    }

    struct Normal2
    {
        double a;
        int b;
    }

    struct Normal3
    {
        union
        {
            double a1;
            long a2;
        }

        double b, c, d;

        this(double a, double b, double c, double d)
        {
            this.a1 = a;
            this.b = b;
            this.c = c;
            this.d = d;
        }
    }

    // Not a HFA: Too many members
    struct Normal4
    {
        double a, b, c, d, e;
    }

    // Not a HVA: Too many members
    version (AArch64) struct Normal5
    {
        double2 a, b, c, d, e;
        bool opEquals()(auto ref const Normal5 rhs) const
        {
            return compare(a, rhs.a) && compare(b, rhs.b) && compare(c, rhs.c)
                && compare(d, rhs.d) && compare(e, rhs.e);
        }
    }

    // An AArch64 composite, passed in general purpose registers
    struct Reg1
    {
        int a, b;
    }

    // An AArch64 composite, passed in general purpose registers
    struct Reg2
    {
        double a;
        float b;
    }

    // An AArch64 composite, passed in general purpose registers
    struct Reg3
    {
        ubyte[16] a;
        static opCall()
        {
            Reg3 result;
            for (size_t i = 0; i < 16; i++)
                result.a[i] = cast(ubyte) i;

            return result;
        }
    }

    // An AArch64 composite, passed in general purpose registers by reference
    // for Num > 16
    struct Mem1(size_t Num)
    {
        ubyte[Num] a;

        static opCall()
        {
            Mem1!Num result;
            for (size_t i = 0; i < Num; i++)
                result.a[i] = cast(ubyte) i;

            return result;
        }
    }

    class Class1
    {
        void foo()
        {
        }

        int a;
        double b;
    }
}

/*
 * Test TypeInfo va_arg function for basic types, both extern(C) and D funcs.
 */
unittest
{
    void* ptr = cast(void*) 0xABCDEF;
    bool boolI = false;
    byte byteI = 1;
    ubyte ubyteI = 1;
    short shortI = 2;
    ushort ushortI = 2;
    int intI = 3;
    uint uintI = 3;
    long longI = 4;
    ulong ulongI = 4;
    char charI = 'c';
    wchar wcharI = 'w';
    dchar dcharI = 'd';
    float floatI = 21;
    double doubleI = 42;
    real realI = 11;
    ifloat ifloatI = 11i;
    idouble idoubleI = 12i;
    ireal irealI = 13i;
    cfloat cfloatI = 1 + 1i;
    cdouble cdoubleI = 2 + 2i;
    creal crealI = 3 + 3i;
    immutable(creal) crealII = 3 + 3i;

    testTypeInfoVaArg!(TestMode.both)(ptr, ptr);
    testTypeInfoVaArg!(TestMode.both)(ptr, boolI);
    testTypeInfoVaArg!(TestMode.both)(ptr, byteI);
    testTypeInfoVaArg!(TestMode.both)(ptr, ubyteI);
    testTypeInfoVaArg!(TestMode.both)(ptr, shortI);
    testTypeInfoVaArg!(TestMode.both)(ptr, ushortI);
    testTypeInfoVaArg!(TestMode.both)(ptr, intI);
    testTypeInfoVaArg!(TestMode.both)(ptr, uintI);
    testTypeInfoVaArg!(TestMode.both)(ptr, longI);
    testTypeInfoVaArg!(TestMode.both)(ptr, ulongI);
    testTypeInfoVaArg!(TestMode.both)(ptr, charI);
    testTypeInfoVaArg!(TestMode.both)(ptr, wcharI);
    testTypeInfoVaArg!(TestMode.both)(ptr, dcharI);
    testTypeInfoVaArg!(TestMode.both)(ptr, ptr);
    testTypeInfoVaArg!(TestMode.both)(ptr, doubleI);
    testTypeInfoVaArg!(TestMode.both)(ptr, realI);
    testTypeInfoVaArg!(TestMode.both)(ptr, idoubleI);
    testTypeInfoVaArg!(TestMode.both)(ptr, irealI);
    testTypeInfoVaArg!(TestMode.both)(ptr, cfloatI);
    testTypeInfoVaArg!(TestMode.both)(ptr, cdoubleI);
    testTypeInfoVaArg!(TestMode.both)(ptr, crealI);
    testTypeInfoVaArg!(TestMode.both)(ptr, crealII);
}

version (AArch64)
{
    /*
     * Special cases for AArch64 float types:
     * When float is passed to an extern(C) varags func, it is promoted to double.
     * When pased to extern(D) varargs func, it does not promote.
     */
    unittest
    {
        void* ptr = cast(void*) 0xABCDEF;
        float floatI = 1;
        ifloat ifloatI = 2i;
        testTypeInfoVaArg!(TestMode.D)(ptr, floatI);
        testTypeInfoVaArg!(TestMode.D)(ptr, ifloatI);

        static extern (C) void testFloat(void* p1, ...)
        {
            va_list ap;
            va_start(ap, p1);
            double val;
            va_arg(ap, typeid(double), &val);
            assert(val == 1);
            va_end(ap);
        }

        static extern (C) testIFloat(void* p1, ...)
        {
            va_list ap;
            va_start(ap, p1);
            idouble val;
            va_arg(ap, typeid(idouble), &val);
            assert(val == 2i);
            va_end(ap);
        }

        static extern (C) testIFloatI(void* p1, ...)
        {
            va_list ap;
            va_start(ap, p1);
            idouble val;
            va_arg(ap, typeid(immutable(idouble)), &val);
            assert(val == 3i);
            va_end(ap);
        }

        float f = 1;
        testFloat(ptr, f);
        ifloat fi = 2i;
        testIFloat(ptr, fi);
        immutable(ifloat) ifi = 3i;
        testIFloatI(ptr, ifi);
    }

    /*
     * Test TypeInfo va_arg function for vector types, both extern(C) and D funcs.
     */
    unittest
    {
        void* ptr = cast(void*) 0xABCDEF;
        float2 float2I = [1.0, 2.0];
        float4 float4I = [3.0, 4.0, 5.0, 6.0];
        double2 double2I = [7.0, 8.0];
        // Note: AArch64 short vector must be 8 or 16 in size, so this
        // is not a short vector!
        double4 double4I = [9.0, 10.0, 11.0, 12.0];
        ubyte16 ubyte16I = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        uint4 uint4I = [1, 2, 3, 4];

        testTypeInfoVaArg!(TestMode.both)(ptr, float2I);
        testTypeInfoVaArg!(TestMode.both)(ptr, float4I);
        testTypeInfoVaArg!(TestMode.both)(ptr, double2I);
        testTypeInfoVaArg!(TestMode.both)(ptr, double4I);
        testTypeInfoVaArg!(TestMode.both)(ptr, uint4I);
        testTypeInfoVaArg!(TestMode.both)(ptr, ubyte16I);
    }

    /*
     * Homogeneous vector arrays are special types on AArch64.
     */
    unittest
    {
        void* ptr = cast(void*) 0xABCDEF;
        auto hva1 = HVA1([1.0, 2.0, 3.0, 4.0], [1.0, 2.0, 3.0, 4.0]);
        auto hva2 = HVA2([1.0, 2.0, 3.0, 4.0], [1.0, 2.0]);
        auto hva3 = HVA3([1.0, 2.0, 3.0, 4.0], [1.0, 2.0, 3.0, 4.0]);
        immutable(HVA3) hva3I = HVA3([1.0, 2.0, 3.0, 4.0], [1.0, 2.0, 3.0, 4.0]);
        auto normal5 = Normal5([9.0, 9.0], [10.0, 10.0], [11.0, 11.0], [12.0, 12.0], [13.0, 13.0]);
        immutable(Normal5) normal5I = Normal5([9.0, 9.0], [10.0, 10.0], [11.0,
                11.0], [12.0, 12.0], [13.0, 13.0]);

        testTypeInfoVaArg!(TestMode.both)(ptr, hva1);
        testTypeInfoVaArg!(TestMode.both)(ptr, hva2);
        testTypeInfoVaArg!(TestMode.both)(ptr, hva3);
        testTypeInfoVaArg!(TestMode.both)(ptr, hva3I);
        testTypeInfoVaArg!(TestMode.both)(ptr, normal5);
        testTypeInfoVaArg!(TestMode.both)(ptr, normal5I);
    }
}

/*
 * Homogeneous float arrays are special types on AArch64.
 */
unittest
{
    void* ptr = cast(void*) 0xABCDEF;
    auto hfa1 = HFA1(1.0, 2.0);
    auto hfa2 = HFA2(1.0 + 1.0i, 2.0);
    auto hfa3 = HFA3(HFA1(1.0, 2.0), 3.0);
    auto hfa4 = HFA4(1.0, 2.0, 3.0, 4.0);
    immutable(HFA4) hfa4I = HFA4(1.0, 2.0, 3.0, 4.0);

    testTypeInfoVaArg!(TestMode.both)(ptr, hfa1);
    testTypeInfoVaArg!(TestMode.both)(ptr, hfa2);
    testTypeInfoVaArg!(TestMode.both)(ptr, hfa3);
    testTypeInfoVaArg!(TestMode.both)(ptr, hfa4);
}

/*
 * Composite types.
 */
unittest
{
    void* ptr = cast(void*) 0xABCDEF;
    auto normal1 = Normal1(1.0, 2.0);
    auto normal2 = Normal2(3.0, 4);
    auto normal3 = Normal3(5.0, 6.0, 7.0, 8.0);
    auto normal4 = Normal4(9.0, 10.0, 11.0, 12.0, 13.0);
    auto reg1 = Reg1(1, 2);
    auto reg2 = Reg2(1, 2);
    auto reg3 = Reg3();
    auto mem1 = Mem1!512();
    // 16 byte is larged type passed in registers on Aarch64
    auto mem2 = Mem1!17();
    immutable(Mem1!32) mem3 = Mem1!32();

    auto class1 = new Class1();
    uint[] darray1 = [1, 2, 3];
    immutable(uint[]) darray1I = [1, 2, 3];
    auto del1 = &class1.foo;
    int[string] aa1 = ["tst" : 1];

    testTypeInfoVaArg!(TestMode.both)(ptr, normal1);
    testTypeInfoVaArg!(TestMode.both)(ptr, normal2);
    testTypeInfoVaArg!(TestMode.both)(ptr, normal3);
    testTypeInfoVaArg!(TestMode.both)(ptr, normal4);
    testTypeInfoVaArg!(TestMode.both)(ptr, reg1);
    testTypeInfoVaArg!(TestMode.both)(ptr, reg2);
    testTypeInfoVaArg!(TestMode.both)(ptr, reg3);
    testTypeInfoVaArg!(TestMode.both)(ptr, mem1);
    testTypeInfoVaArg!(TestMode.both)(ptr, mem2);
    testTypeInfoVaArg!(TestMode.both)(ptr, mem3);

    testTypeInfoVaArg!(TestMode.both)(ptr, class1);
    testTypeInfoVaArg!(TestMode.both)(ptr, del1);
    testTypeInfoVaArg!(TestMode.both)(ptr, aa1);

    testTypeInfoVaArg!(TestMode.D)(ptr, darray1);
    testTypeInfoVaArg!(TestMode.D)(ptr, darray1I);
}

version (unittest)
{
    void testStaticArray(T, T[] value)(void* ptr, ...)
    {
        // Make sure our buffer is properly aligned
        union Buffer
        {
            ubyte[1024] buf;
            T[] arr;
        }

        Buffer buffu;

        auto ti = _arguments[0];
        va_arg(_argptr, ti, buffu.buf.ptr);
        assert(buffu.arr == value);
    }
}

/*
 * Arrays can only be passed to D-style varargs functions and
 * static arrays get converted into dynamic arrays.
 */
unittest
{
    void* ptr = cast(void*) 0xABCDEF;
    uint[4] sarray1 = [1, 2, 3, 4];
    immutable(uint[4]) sarray1I = [1, 2, 3, 4];
    double[2] sarray2 = [1, 2];
    immutable(double[2]) sarray2I = [1, 2];
    float[3] sarray3 = [1, 2, 3];
    real[4] sarray4 = [1, 2, 3, 4];

    testStaticArray!(uint, [1, 2, 3, 4])(ptr, sarray1);
    testStaticArray!(uint, [1, 2, 3, 4])(ptr, sarray1I);
    testStaticArray!(double, [1, 2])(ptr, sarray2);
    testStaticArray!(double, [1, 2])(ptr, sarray2I);
    testStaticArray!(float, [1, 2, 3])(ptr, sarray3);
    testStaticArray!(real, [1, 2, 3, 4])(ptr, sarray4);
}

/*
 * This tests special cases for AArch64
 */
unittest
{
    static align(16) struct S2
    {
        int a;
        int b;
        int c;
        int d;
    }

    void* ptr = cast(void*) 0xABCDEF;
    auto hfa1 = HFA1(1.0, 2.0);
    double doubleI = 3.0;
    auto reg1 = Reg1(10, 20);
    auto s2 = S2(30, 40, 50, 60);

    // Fill all SIMD registers, so rest is passed on stack
    testTypeInfoVaArg!(TestMode.both)(ptr, hfa1, hfa1, hfa1, hfa1, hfa1);
    testTypeInfoVaArg!(TestMode.both)(ptr, hfa1, hfa1, hfa1, hfa1, doubleI);
    testTypeInfoVaArg!(TestMode.both)(ptr, doubleI, doubleI, hfa1, doubleI, hfa1, hfa1);
    // Passing a simple double, then hfas requiring two registers, then single double
    testTypeInfoVaArg!(TestMode.both)(ptr, doubleI, hfa1, hfa1, hfa1, hfa1, doubleI);

    // Fill all general purpose registers, so rest is passed on stack
    testTypeInfoVaArg!(TestMode.both)(ptr, reg1, reg1, reg1, reg1, reg1, reg1,
            reg1, reg1, reg1, reg1);
    testTypeInfoVaArg!(TestMode.both)(ptr, s2, s2, reg1, reg1, reg1, reg1, reg1, reg1, reg1);

    // Mixing register types
    testTypeInfoVaArg!(TestMode.both)(ptr, reg1, s2, reg1, s2, reg1, reg1,
            reg1, reg1, reg1, reg1, doubleI, doubleI, hfa1, doubleI, hfa1, hfa1);
    testTypeInfoVaArg!(TestMode.both)(ptr, doubleI, doubleI, hfa1, doubleI,
            hfa1, hfa1, reg1, reg1, reg1, reg1, reg1, s2, reg1, s2, reg1, reg1);
    testTypeInfoVaArg!(TestMode.both)(ptr, s2, doubleI, reg1, hfa1, doubleI,
            reg1, hfa1, reg1, hfa1, reg1, hfa1, reg1, s2, reg1, s2, reg1, doubleI);
}
