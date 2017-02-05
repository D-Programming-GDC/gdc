// REQUIRED_ARGS: -transition=checkimports -de
/*
TEST_OUTPUT:
---
---
*/

template anySatisfy(T...)
{
    alias anySatisfy = T[$ - 1];
}

alias T = anySatisfy!(int);

// https://issues.dlang.org/show_bug.cgi?id=15857

template Mix15857(T)
{
    void foo15857(T) {}
}
mixin Mix15857!int;
mixin Mix15857!string;

void test15857()
{
    foo15857(1);
}
