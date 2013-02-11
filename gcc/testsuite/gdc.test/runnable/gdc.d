// EXTRA_SOURCES: imports/gdca.d

module gdc;

import imports.gdca;

/*
 * Here getChar is a function in a template where template.isnested == false
 * but getChar still is a nested function and needs to get a
 * static chain containing test1.
 */
void test1()(char val)
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

/*
 * Similar as test1, but a little more complicated:
 * Here getChar is nested in a struct template which is nested
 * in a function. getChar's static chain still needs to contain test2.
 */
void test2()(char val)
{
    void error()
    {
    }
    
    struct S(T){
    void getChar()
    {
        error();
    }}


    void parseString()
    {
        S!(int)().getChar();
    }
}

/*
 * If g had accessed a, the frontend would have generated a closure.
 *
 * As we do not access it, there's no closure. We have to be careful
 * not to set a static chain for g containing test3helper though,
 * as g can be called from outside (here from test3). In the end
 * we have to treat this as if everything in test3helper was declared
 * at module scope.
 */
auto test3helper()
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

void test3()
{
    test3helper().g();
}

/*
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
            @property empty(){  return false; }
        }

        auto counterRange = FixedStack!uint();
    }
}

void test4()
{
    auto parser = Parser!(char[])();
    imports.gdca.test4a;
}


void main()
{
    test1('n');
    test2('n');
    test3();
    test4();
}
