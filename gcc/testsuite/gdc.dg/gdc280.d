// Bug 280

void main()
{
    import std.container;
    static immutable s = new RedBlackTree!int('h');
}
