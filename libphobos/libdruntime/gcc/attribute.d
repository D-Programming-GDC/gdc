module gcc.attribute;

struct Attribute(A...)
{
    A args;
}

auto attribute(A...)(A args)
{
    return Attribute!A(args);
}
