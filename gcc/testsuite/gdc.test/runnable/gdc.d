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

void main()
{
    test1('n');
}
