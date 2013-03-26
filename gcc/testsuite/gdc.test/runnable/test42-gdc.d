// { dg-additional-options "-fno-inline -fno-omit-frame-pointer" }
// GCC optimizations break this test (only the test, not the feature it tests)

module test42;

/***************************************************/
// 7290

version (D_InlineAsm_X86)
{
    enum GP_BP = "EBP";
    version = ASM_X86;
}
else version (D_InlineAsm_X86_64)
{
    enum GP_BP = "RBP";
    version = ASM_X86;
}

int foo7290a(alias dg)()
{
    assert(dg(5) == 7);

    void* p;
    version (ASM_X86)
    {
        mixin(`asm { mov p, ` ~ GP_BP ~ `; }`);
    }
    else version(GNU)
    {
        version(X86) asm
        {
            "mov %%EBP, %0" : "=r" p : :;
        }
        else version(X86_64) asm
        {
            "mov %%RBP, %0" : "=r" p : :;
        }
        else static assert(false, "ASM code not implemented for this architecture");
    }
    assert(p < dg.ptr);
    return 0;
}

int foo7290b(scope int delegate(int a) dg)
{
    assert(dg(5) == 7);
    
    void* p;
    version (ASM_X86)
    {
        mixin(`asm { mov p, ` ~ GP_BP ~ `; }`);
    }
    else version(GNU)
    {
        version(X86) asm
        {
            "mov %%EBP, %0" : "=r" p : :;
        }
        else version(X86_64) asm
        {
            "mov %%RBP, %0" : "=r" p : :;
        }
        else static assert(false, "ASM code not implemented for this architecture");
    }
    assert(p < dg.ptr);
    return 0;
}

int foo7290c(int delegate(int a) dg)
{
    assert(dg(5) == 7);

    void* p;
    version (ASM_X86)
    {
        mixin(`asm { mov p, ` ~ GP_BP ~ `; }`);
    }
    else version(GNU)
    {
        version(X86) asm
        {
            "mov %%EBP, %0" : "=r" p : :;
        }
        else version(X86_64) asm
        {
            "mov %%RBP, %0" : "=r" p : :;
        }
        else static assert(false, "ASM code not implemented for this architecture");
    }
    assert(p < dg.ptr);
    return 0;
}

void test7290()
{
    int add = 2;
    scope dg = (int a) => a + add;

    void* p;
    version (ASM_X86)
    {
        mixin(`asm { mov p, ` ~ GP_BP ~ `; }`);
    }
    else version(GNU)
    {
        version(X86) asm
        {
            "mov %%EBP, %0" : "=r" p : :;
        }
        else version(X86_64) asm
        {
            "mov %%RBP, %0" : "=r" p : :;
        }
        else static assert(false, "ASM code not implemented for this architecture");
    }
    assert(dg.ptr <= p);

    foo7290a!dg();
    foo7290b(dg);
    foo7290c(dg);
}
/***************************************************/

int main()
{
    test7290();
    return 0;
}
