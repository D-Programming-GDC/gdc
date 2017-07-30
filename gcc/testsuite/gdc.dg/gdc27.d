// { dg-additional-sources "imports/gdc27a.d" }
module gdc27;

import imports.gdc27a;

interface I_B : I_A
{
    void b();
}

abstract class C_B : C_A, I_B
{
    abstract void b();
}

void main()
{
}
