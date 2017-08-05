/* TEST_OUTPUT:
---
fail_compilation/fail17720.d(13): Error: cannot implicitly convert expression `i1 + i2` of type `__vector(int[4])` to `__vector(float[4])`
---
*/

// https://issues.dlang.org/show_bug.cgi?id=17720

void fail17720()
{
    import core.simd;
    int4 i1, i2;
    float4 f1 = i1 + i2;
}
