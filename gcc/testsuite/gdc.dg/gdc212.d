template hasElaborateAssign(S)
{
    enum hasElaborateAssign = is(typeof(S.init.opAssign(rvalueOf!S))) ||
        is(typeof(lvalueOf!S)) ;
}

T rvalueOf(T)();

T lvalueOf(T)();


template TypeTuple(TList...)
{
    alias TypeTuple = TList;
}

template Tuple()
{
    struct Tuple
    {
        void opAssign(R)(R)
        {
            if (hasElaborateAssign!R)
            {
            }
        }
    }
}

ref emplaceRef()
{
    static if (!hasElaborateAssign!(Tuple!()))
        chunk;
}

class TaskPool
{
    void reduce()
    {
        Tuple!() seed = void;
        Tuple!()[] results;
        foreach(i; TypeTuple!(0, 1))
            results[i] = seed;
    }
}
