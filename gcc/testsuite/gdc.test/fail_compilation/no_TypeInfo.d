/* 
DFLAGS:
REQUIRED_ARGS: -c
EXTRA_SOURCES: extra-files/no_TypeInfo/object.d
TEST_OUTPUT:
---
fail_compilation/no_TypeInfo.d(13): Error: `object.TypeInfo` could not be found, but is implicitly used
---
*/

void test()
{
    int i;
    auto ti = typeid(i);
}

