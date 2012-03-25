
struct Array
{
    size_t length;
    void * ptr;
}

extern (C) Array _adSort(Array a, TypeInfo ti)
{
    static const uint Qsort_Threshold = 7;
    
    struct StackEntry {
        byte *l;
        byte *r;
    }

    size_t elem_size = ti.tsize();
    size_t qsort_limit = elem_size * Qsort_Threshold;
    
    static assert(ubyte.sizeof == 1);
    static assert(ubyte.max == 255);
    
    StackEntry[size_t.sizeof * 8] stack; // log2( size_t.max )
    StackEntry * sp = stack.ptr;
    byte* lbound = cast(byte *) a.ptr;
    byte* rbound = cast(byte *) a.ptr + a.length * elem_size;
    byte* li = void;
    byte* ri = void;

    while (1)
    {
        if (rbound - lbound > qsort_limit)
        {
            ti.swap(lbound,
                lbound + (
                          ((rbound - lbound) >>> 1) -
                          (((rbound - lbound) >>> 1) % elem_size)
                          ));
            
            li = lbound + elem_size;
            ri = rbound - elem_size;

            if (ti.compare(li, ri) > 0)
                ti.swap(li, ri);
            if (ti.compare(lbound, ri) > 0)
                ti.swap(lbound, ri);
            if (ti.compare(li, lbound) > 0)
                ti.swap(li, lbound);

            while (1)
            {
                do
                    li += elem_size;
                while (ti.compare(li, lbound) < 0);
                do
                    ri -= elem_size;
                while (ti.compare(ri, lbound) > 0);
                if (li > ri)
                    break;
                ti.swap(li, ri);                    
            }
            ti.swap(lbound, ri);
            if (ri - lbound > rbound - li)
            {
                sp.l = lbound;
                sp.r = ri;
                lbound = li;
            }
            else
            {
                sp.l = li;
                sp.r = rbound;
                rbound = ri;
            }
            ++sp;
        } else {
            // Use insertion sort
            for (ri = lbound, li = lbound + elem_size;
                 li < rbound;
                 ri = li, li += elem_size)
            {
                for ( ; ti.compare(ri, ri + elem_size) > 0;
                      ri -= elem_size)
                {
                    ti.swap(ri, ri + elem_size);
                    if (ri == lbound)
                        break;
                }
            }       
            if (sp != stack.ptr)
            {
                --sp;
                lbound = sp.l;
                rbound = sp.r;
            }
            else
                return a;
        }
    }
}

unittest
{
    static void check(int[] a) {
        for (uint i = 1; i < a.length; i++)
            assert(a[i-1] <= a[i]);
    }
    
    static int[] t1 = [ 4, 3, 19, 7, 6, 20, 11, 1, 2, 5 ];
    int[] a;

    a = t1;
    a.sort;
    check(a);    
}
