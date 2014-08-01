import gcc.attribute;

/* Test all gdc supported attributes.  */

@attribute("forceinline")
void forceinline()
{
}

@attribute("noinline")
void noinline()
{
}

@attribute("flatten")
void flatten()
{
}

/* Tests for __builtin_volatile_load/store. ASM needs to be verified manually  */

import gcc.builtins;

enum xptr = cast(int*)0xDEADBEEF;
enum xptr2 = cast(int*)0x42;

// Verify that every load is emitted
void testVolatile1()
{
    while(true)
    {
        auto x = __builtin_volatile_load(xptr);
    }
}

// Verify that every load is emitted
void testVolatile2()
{
    auto x = __builtin_volatile_load(xptr);
    x = __builtin_volatile_load(xptr);
    x = __builtin_volatile_load(xptr);
}

// Verify that every load is emitted
void testVolatile3()
{
    __builtin_volatile_load(xptr);
    __builtin_volatile_load(xptr);
    __builtin_volatile_load(xptr);
}

// ensure ordering of volatile accesses
void testVolatile4()
{
    auto x = __builtin_volatile_load(xptr);
    x = __builtin_volatile_load(xptr2);
    x = __builtin_volatile_load(xptr);
    x = __builtin_volatile_load(xptr2);
    x = __builtin_volatile_load(xptr);
}

// Verify that every store is emitted
void testVolatile5()
{
    while(true)
    {
        __builtin_volatile_store(xptr, 0);
    }
}

// Verify that every store is emitted
void testVolatile6()
{
    __builtin_volatile_store(xptr, 0);
    __builtin_volatile_store(xptr, 0);
    __builtin_volatile_store(xptr, 0);
}

// ensure ordering of volatile accesses
void testVolatile7()
{
    __builtin_volatile_store(xptr, 0);
    __builtin_volatile_store(xptr2, 0);
    __builtin_volatile_store(xptr, 0);
    __builtin_volatile_store(xptr2, 0);
    __builtin_volatile_store(xptr, 0);
}
