module gdc;

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


void main()
{
    test1('n');
    test2('n');
    test3();
}
