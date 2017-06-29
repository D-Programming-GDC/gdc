// PERMUTE_ARGS:

void test261()
{
    class C1
    {
        void f1()
        {
            class C2
            {
                void f2()
                {
                    auto v = &f1;
                }
            }
        }
    }
}
