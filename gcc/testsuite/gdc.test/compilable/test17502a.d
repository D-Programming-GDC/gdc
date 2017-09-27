// https://issues.dlang.org/show_bug.cgi?id=17502
// Order of declaration: (Foo), (Baz : Bar), (Bar : Foo)
class Foo
{
    int method(int p)
    in
    {
        assert(p > 5);
    }
    out(res)
    {
        assert(res > 5);
    }
    body
    {
        return p;
    }
}

class Baz : Bar
{
    override int method(int p)
    in
    {
        assert(p > 3);
    }
    body
    {
        return p * 2;
    }
}

class Bar : Foo
{
    override int method(int p)
    in
    {
        assert(p > 2);
    }
    body
    {
        return p * 3;
    }
}
