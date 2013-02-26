module gcc.attribute;

private struct Attribute(A...)
{
    A args;
}

auto attribute(A...)(A args) if(A.length > 0 && is(A[0] == string))
{
    return Attribute!A(args);
}
