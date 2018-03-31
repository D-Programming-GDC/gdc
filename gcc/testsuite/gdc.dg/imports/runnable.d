module imports.runnable;

private import runnable;

/******************************************/

// Bug 36

void test36d_1()
{
    auto parser = Parser!(char[])();
}

/******************************************/

// Bug 253

class B253 : A253
{
    void test253(int[int] a)
    {
        if (a.get(0, 1))
            return;
    }
}
