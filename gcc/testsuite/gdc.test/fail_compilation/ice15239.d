/*
TEST_OUTPUT:
---
fail_compilation/ice15239.d(21): Error: cannot interpret `opDispatch!"foo"` at compile time
fail_compilation/ice15239.d(21): Error: bad type/size of operands `__error`
---
*/

struct T
{
    template opDispatch(string Name, P...)
    {
        static void opDispatch(P) {}
    }
}

void main()
{
    version(GNU)
    {
        asm
        {
            "" : : "r" T.foo;
        }
    }
    else
    {
        asm
        {
            call T.foo;
        }
    }
}
