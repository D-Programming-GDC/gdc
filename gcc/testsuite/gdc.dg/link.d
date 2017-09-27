// { dg-do link { target arm*-*-* i?86-*-* x86_64-*-* } }

class A()
{
    static struct S { A a; }
}

enum e = is(A!());

void main() {}
