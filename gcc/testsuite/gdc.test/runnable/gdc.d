// EXTRA_SOURCES: imports/gdca.d

module gdc;

import imports.gdca;
import core.stdc.stdio;
import gcc.attribute;


/******************************************/

// Bug 2

struct S
{
    string toString() { return "foo"; }
}

void test2()
{
    import std.string : format;
    assert(format("%s", S()) == "foo");
}

/******************************************/

// Bug 4

void test4()
{
    string str = "allo";
    static assert(!__traits(compiles, str.reverse));
    static assert(!__traits(compiles, str.sort));
}

/******************************************/

// Bug 15

class B
{
    class A { }
    A a;
}

class C
{
    void visit(B b)
    {
        import std.algorithm : map;
        auto as = [b.a];
        as.map!(d => d);
    }
}

/******************************************/

// Bug 16

void test16()
{
    import std.parallelism : taskPool;

    taskPool.reduce!"a+b"([0, 1, 2, 3]);
}

/******************************************/

// Bug 17

/**
 * Parameters are not copied into a frame to be accessed from
 * the method's __require function.
 */
void contractTest(string path)
{
    assert(path[0] == 't');
    assert(path.length == 9);
    assert(path[8] == 'i');
}

interface ModuleSaver
{
    void save(string str)
    in
    {
        contractTest(str);
    }
}

class ModuleWriter : ModuleSaver
{
    void save (string str)
    in {}
    body
    {
    }
}

void test17()
{
  (new ModuleWriter()).save ("test.0.mci");
}

/******************************************/

// Bug 18

class C18
{
    struct Link
    {
        int x;
        int y;
    }

    void sort_links()
    {
        import std.algorithm : sort;
        import std.array : empty;
        import std.exception : enforce;

        enforce(!_link.empty);

        bool lt(Link a, Link b)
        {
            if(a.x > b.x)
                return false;
            if(a.x < b.x)
                return true;
            if(a.y >= b.y)
                return false;
            else
                return true;
        }
        sort!(lt)(_link);
    }

    this()
    {
        _link ~= Link(8, 3);
        _link ~= Link(4, 7);
        _link ~= Link(4, 6);
        _link ~= Link(3, 7);
        _link ~= Link(2, 7);
        _link ~= Link(2, 2);
        _link ~= Link(4, 1);
    }

    Link[] _link;
}

void test18()
{
    C18 foo = new C18;
    foo.sort_links();
}

/******************************************/

// Bug 19

void test19()
{
   byte b;
   --b = b;
}

/******************************************/

// Bug 24

void test24()
{
    struct S24
    {
        char[1] b;
    }

    S24 a;

    if (*a.b.ptr)
        return;
}

/******************************************/

// Bug 29

void test29()
{
    import std.string : format, text;

    string s;
    for (auto i = 0; i < 100000; i++)
    {
        s = format("%d", i);
        s = text(i);
    }
}

/******************************************/

// Bug 31

class RedBlackTree(T, alias less)
{
    struct Range
    {
        @property empty() { }
    }

    Range opSlice()
    {
        return Range();
    }
}

auto redBlackTree(alias less, E)()
{
    return new RedBlackTree!(E, less);
}

void test31()
{
    redBlackTree!((a){}, double)();
}

/******************************************/

// Bug 35

/**
 * Here the BinaryHeap instance uses an alias parameter and therefore
 * the instance's functions (percolateDown) need to be generated in
 * topNIndex->BinaryHeap scope and not in the declaration scope
 * (module->BinaryHeap).
 */
void topNIndex()()
{
    bool indirectLess(int a, int b)
    {
        return a > b;
    }

    auto a = BinaryHeap!(indirectLess)();
}

struct BinaryHeap(alias less)
{
    void percolateDown()
    {
        less(0, 1);
    }
}

void test35a()
{
    topNIndex();
}

/*
 * Similar as test35a but with an additional indirection.
 * The nested function chain for percolateDown should look like this:
 * topNIndex2->BinaryHeap2->percolateDown.
 */
void topNIndex2()()
{
    bool indirectLess(int a, int b)
    {
        return a > b;
    }
    auto a = BinaryHeap2!(S35b!(indirectLess)())();
}

struct S35b(alias a)
{
    void foo()
    {
        a(0, 0);
    }
}

struct BinaryHeap2(alias less)
{
    void percolateDown()
    {
        less.foo();
    }
}

void test35b()
{
    topNIndex2();
}

void test35()
{
    test35a();
    test35b();
}

/******************************************/

// Bug 36

/**
 * Here getChar is a function in a template where template.isnested == false
 * but getChar still is a nested function and needs to get a static chain
 * containing test36a.
 */
void test36a()(char val)
{
    void error()
    {
    }

    void getChar()()
    {
        error();
    }

    void parseString()
    {
        getChar();
    }
}

/**
 * Similar as test36a, but a little more complicated:
 * Here getChar is nested in a struct template which is nested in a function.
 * getChar's static chain still needs to contain test36b.
 */
void test36b()(char val)
{
    void error()
    {
    }

    struct S(T)
    {
        void getChar()
        {
            error();
        }
    }


    void parseString()
    {
        S!(int)().getChar();
    }
}

/**
 * If g had accessed a, the frontend would have generated a closure.
 *
 * As we do not access it, there's no closure. We have to be careful
 * not to set a static chain for g containing test36c_1 though,
 * as g can be called from outside (here from test1c). In the end
 * we have to treat this as if everything in test36c_1 was declared
 * at module scope.
 */
auto test36c_1()
{
    int a;
    void c() {};
    class Result
    {
        int b;
        void g() { c(); /*a = 42;*/ }
    }

    return new Result();
}

void test36c()
{
    test36c_1().g();
}

/**
 * empty is a (private) function which is nested in lightPostprocess.
 * At the same time it's a template instance, so it has to be declared
 * as weak or otherwise one-only. imports/gdca.d creates another instance
 * of Regex!char to verify that.
 */
struct Parser(R)
{
    @property program()
    {
        return Regex!char();
    }
}

struct Regex(Char)
{
    @trusted lightPostprocess()
    {
        struct FixedStack(T)
        {
            @property empty() { return false; }
        }
        auto counterRange = FixedStack!uint();
    }
}

void test36d()
{
    auto parser = Parser!(char[])();
    imports.gdca.test36d_1;
}

void test36()
{
  test36a('n');
  test36b('n');
  test36c();
  test36d();
}

/******************************************/

// Bug 37

struct S37
{
    int bar(const S37 s)
    {
        return 0;
    }
}

int test37()
{
    S37 s;
    return s.bar(s);
}

/******************************************/

// Bug 43

void test43()
{
    import core.vararg;
    import core.stdc.stdio;

    void formatArray(ref va_list argptr)
    {
        auto a = va_arg!(const(float)[])(argptr);
        foreach(f; a)
        {
            printf("%f\n", f);
        }
    }

    void doFormat(TypeInfo[] arguments, va_list argptr)
    {
        formatArray(argptr);
    }

    void format(...)
    {
        doFormat(_arguments, _argptr);
    }

    format([1.0f, 2.0f, 3.0f]);
}

/******************************************/

// Bug 47

template Foo47()
{
    void test47()
    {
        asm { "nop"; }
    }
}

mixin Foo47!();

/******************************************/

// Bug 51

struct S51
{
    int x;
    int pad;

    this(this)
    {
        ++x;
    }
}

void test51()
{
    S51 s;
    auto sarr = new S51[1];
    auto sarr2 = sarr;

    // postblit all fields.
    sarr2 ~= s;

    assert (sarr2[0].x == 1);
    assert (sarr2[1].x == 1);
    assert (sarr[0].x == 0);
    assert (s.x == 0);
}

/******************************************/

// Bug 57

struct S57
{
    int a;
    long b;
    // Doesn't happen for bigger structs
}

S57 bar57()
{
    return S57(4, 42);
}

void test57()
{
    S57 s = bar57();
    assert (s is S57(4, 42));
}

/******************************************/

// Bug 66

void test66()
{
    int pos = 0;

    foreach(x; 0 .. 64)
    {
        ++pos %= 4;
        assert (pos != 4);
    }
}

/******************************************/

// Bug 67

__vector(float[4]) d[2];  // ICE


/******************************************/

// Bug 71

struct Leaf
{
    ubyte symbol;
    ubyte codeLen;
}

struct CanonicalHuffman
{
    Leaf[] table;

    void print()
    {
        import std.algorithm;
        import std.range;

        auto list = zip(iota(table.length), table.dup).array
            .sort!((a, b) => a[1].symbol < b[1].symbol)
            .uniq!((a, b) => (a[0] & (1 << a[1].codeLen) - 1) == (b[0] & (1 << b[1].codeLen) - 1));
    }
}

/******************************************/

// Bug 77

void fun (ubyte[3] buf)
{
    import std.bitmanip : bigEndianToNative;
    bigEndianToNative!ushort(buf[0..2]);
}
void test77()
{
    fun([1,2,3]);
}

/******************************************/

// Bug 108

@attribute("forceinline")
void test108()
{
    import std.stdio : writeln;
    writeln("Here");
}

/******************************************/

// Bug 115

void test115()
{
    union U
    {
        float f;
        uint i;
    }
    float a = 123.0;
    const l = U(a);

    assert(l.i == U(a).i);
}

/******************************************/

// Bug 121

immutable char C121 = void; // ICE

/******************************************/

// Bug 122

void test122()
{
    import std.algorithm : map;
    import std.parallelism : taskPool;
    import std.range : iota;

    immutable n = 10000;
    immutable delta = 1.0 / n;
    immutable pi = 4.0 * delta * taskPool.reduce!"a + b"(
        map!((int i) { immutable x = (i - 0.5) * delta; return 1.0 / (1.0 + x * x); })(iota(n)));
}

/******************************************/

// Bug 127

int[0] test127a;     // OK
int[1][0] test127b;  // OK
int[0][1] test127c;  // ICE

/******************************************/

// Bug 131

struct S131
{
    this(string ) { }
    string opAssign(string v) { return v; }
}

void test131()
{
    S131[string] s;
    s["foo"] = "bar";
}

/******************************************/

// Bug 133

void delegate()[] D133;

void test133a(void delegate() dg)
{
    D133 ~= dg;
}

void test133()
{
    void nested()
    {}
    test133a(&nested);
}

/******************************************/

// Bug 141

bool test141a(int a)
{
    return a > (a + 1);
}

void test141()
{
    assert(test141a(int.min) == false);
    assert(test141a(int.max) == true);
}

/******************************************/

// Bug 142

@attribute("noinline")
int test142a()()
{
    return 142;
}

void test142()
{
    enum E142 = test142a();
}

/******************************************/

// Tests for __builtin_volatile_load / __builtin_volatile_store
import gcc.builtins;

void testVolatile1()
{
    int local;
    __builtin_volatile_store(&local, 42);
    assert(local == 42);
}

void testVolatile2()
{
    int local = 42;
    assert(__builtin_volatile_load(&local) == 42);
    local = 0;
    assert(__builtin_volatile_load(&local) == 0);
}

struct NonPOD
{
    int x;
    ~this()
    {
        x++;
    }
}

struct POD
{
    int x, y, z;
    double x1, y1, z1;
}

void testVolatile3()
{
    assert(!__traits(compiles, __builtin_volatile_load(cast(NonPOD*)0xDEADBEEF)));
    assert(!__traits(compiles, __builtin_volatile_load(cast(POD*)0xDEADBEEF)));
    assert(__traits(compiles, __builtin_volatile_load(cast(ubyte*)0xDEADBEEF)));
    assert(__traits(compiles, __builtin_volatile_load(cast(ushort*)0xDEADBEEF)));
    assert(__traits(compiles, __builtin_volatile_load(cast(uint*)0xDEADBEEF)));
    assert(__traits(compiles, __builtin_volatile_load(cast(ulong*)0xDEADBEEF)));

    assert(!__traits(compiles, __builtin_volatile_store(cast(NonPOD*)0xDEADBEEF, NonPOD.init)));
    assert(!__traits(compiles, __builtin_volatile_store(cast(POD*)0xDEADBEEF, POD.init)));
    assert(__traits(compiles, __builtin_volatile_store(cast(ubyte*)0xDEADBEEF, ubyte.init)));
    assert(__traits(compiles, __builtin_volatile_store(cast(ushort*)0xDEADBEEF, ushort.init)));
    assert(__traits(compiles, __builtin_volatile_store(cast(uint*)0xDEADBEEF, uint.init)));
    assert(__traits(compiles, __builtin_volatile_store(cast(ulong*)0xDEADBEEF, ulong.init)));
}

/******************************************/

void main()
{
    test2();
    test4();
    test16();
    test17();
    test18();
    test35();
    test36();
    test43();
    test51();
    test57();
    test66();
    test77();
    test108();
    test115();
    test131();
    test133();
    test141();
    testVolatile1();
    testVolatile2();
    testVolatile3();

    printf("Success!\n");
}
