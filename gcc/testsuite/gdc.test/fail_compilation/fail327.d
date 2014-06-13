/*
TEST_OUTPUT:
---
fail_compilation/fail327.d(10): Error: inline assembler not allowed in @safe function foo
---
*/

@safe void foo()
{
    version(GNU)
    {
        version(X86) asm {"xor %%EAX,%%EAX" : : : ;}
        else version(X86_64) asm {"xor %%EAX,%%EAX" : : : ;}
        else static assert(false, "ASM code not implemented for this architecture");
    }
    else asm { xor EAX,EAX; }
}
