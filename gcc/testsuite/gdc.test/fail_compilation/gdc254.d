/*
TEST_OUTPUT:
---
fail_compilation/gdc254.d(15): Error: class gdc254.C254 interface function 'void F()' is not implemented
---
*/

import imports.gdc254a;

interface A254
{
    void F();
}

class C254 : B254, A254
{
}

