module imports.gdc27a;

interface I_A
{
    bool a();
}

class C_A : I_A
{
    bool a()
    {
        return false;
    }
}
