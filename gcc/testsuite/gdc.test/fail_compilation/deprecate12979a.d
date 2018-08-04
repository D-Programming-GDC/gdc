// REQUIRED_ARGS: -de
// PERMUTE_ARGS:

/*
TEST_OUTPUT:
---
fail_compilation/deprecate12979a.d(14): Deprecation: `asm` statement is assumed to throw - mark it with `nothrow` if it does not
---
*/


void foo() nothrow
{
    version(GNU)
    {
        asm { ""; }
    }
    else
    {
        asm
        {
            ret;
        }
    }
}
