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


void main()
{
    test1('n');
    test2('n');
}
