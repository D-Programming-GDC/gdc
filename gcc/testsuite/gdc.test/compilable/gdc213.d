// PERMUTE_ARGS:

import core.simd;

struct S213
{
    int4 vec;
}

void test213()
{
    S213 s, b;

    assert(s == b);
}
