module imports.gdc231a;

interface ImplementorIF
{
    void* getImplementorStruct();
    void* getStruct();
}

template ImplementorT()
{
    void* getImplementorStruct()
    {
        return null;
    }
}

class Widget : ImplementorIF
{
    mixin ImplementorT;
    void* getStruct()
    {
        return null;
    }
}
