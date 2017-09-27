module ltotests_0;

import core.stdc.stdio;


/******************************************/

/+ XBUG: lto1: internal compiler error: in get_odr_type
interface I284
{
   void m284();
}

class C284 : I284
{
   void m284() { }
}
+/

/******************************************/

/+ XBUG: lto1: internal compiler error: in get_odr_type
class C304
{
}

C304 c304;
+/

/******************************************/

// Bug 61

struct S61a
{
    void a() { }
    void b() { }
}

struct S61b
{
    S61a other;

    void foo()
    {
        bar();
    }

    void bar()
    {
        try
            other.a();
        catch
            other.b();
    }
}

/******************************************/

// Bug 88

extern(C) int test88a();

void test88()
{
    test88a();
}

/******************************************/

void main(string[])
{
    test88();

    printf("Success!\n");
}
