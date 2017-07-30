// { dg-options "-I $srcdir/gdc.dg" }
import imports.gdc170a;

void main()
{
    foo!void.foo1!void();
    foo!void.foo2!void();
    foo!void.foo3();
    foo!void.foo3!void();
    foo!void.foo4();
    foo!void.foo4!void();
    foo!void.foo5!void(null);
    foo!void.foo6!void(null);
    foo!void.foo7(null);
    foo!void.foo7!void(null);
    foo!void.foo8(null);
    foo!void.foo8!void(null);
}
