import gcc.attribute;

/* Test all gdc supported attributes.  */

@attribute("forceinline")
void forceinline()
{
}

@attribute("noinline")
void noinline()
{
}

@attribute("flatten")
void flatten()
{
}

