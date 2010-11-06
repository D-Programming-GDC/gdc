// D import file generated from 'src\core\atomic.d'
module core.atomic;
version (D_InlineAsm_X86)
{
    version (X86)
{
    version = AsmX86;
    version = AsmX86_32;
    has64BitCAS = true;
}
else
{
    version (X86_64)
{
    version = AsmX86;
    version = AsmX86_64;
    has64BitCAS = true;
}
}
}
private 
{
    template NakedType(T : shared(T))
{
alias T NakedType;
}
    template NakedType(T : shared(T*))
{
alias T* NakedType;
}
    template NakedType(T : const(T))
{
alias T NakedType;
}
    template NakedType(T : const(T*))
{
alias T* NakedType;
}
    template NamedType(T : T*)
{
alias T NakedType;
}
    template NakedType(T)
{
alias T NakedType;
}
}
version (AsmX86)
{
    private 
{
    template atomicValueIsProperlyAligned(T)
{
bool atomicValueIsProperlyAligned(size_t addr)
{
return addr % T.sizeof == 0;
}
}
}
}
version (ddoc)
{
    template cas(T,V1,V2) if (is(NakedType!(V1) == NakedType!(T)) && is(NakedType!(V2) == NakedType!(T)))
{
bool cas(shared(T)* here, const V1 ifThis, const V2 writeThis)
{
return false;
}
}
}
else
{
    version (AsmX86_32)
{
    template cas(T,V1,V2) if (is(NakedType!(V1) == NakedType!(T)) && is(NakedType!(V2) == NakedType!(T)))
{
bool cas(shared(T)* here, const V1 ifThis, const V2 writeThis)
in
{
static if(T.sizeof > size_t.sizeof)
{
assert(atomicValueIsProperlyAligned!(size_t)(cast(size_t)here));
}
else
{
assert(atomicValueIsProperlyAligned!(T)(cast(size_t)here));
}

}
body
{
static if(T.sizeof == (byte).sizeof)
{
asm { mov DL,writeThis; }
asm { mov AL,ifThis; }
asm { mov ECX,here; }
asm { lock; }
asm { cmpxchg[ECX],DL; }
asm { setz AL; }
}
else
{
static if(T.sizeof == (short).sizeof)
{
asm { mov DX,writeThis; }
asm { mov AX,ifThis; }
asm { mov ECX,here; }
asm { lock; }
asm { cmpxchg[ECX],DX; }
asm { setz AL; }
}
else
{
static if(T.sizeof == (int).sizeof)
{
asm { mov EDX,writeThis; }
asm { mov EAX,ifThis; }
asm { mov ECX,here; }
asm { lock; }
asm { cmpxchg[ECX],EDX; }
asm { setz AL; }
}
else
{
static if(T.sizeof == (long).sizeof && has64BitCAS)
{
asm { push EDI; }
asm { push EBX; }
asm { lea EDI,writeThis; }
asm { mov EBX,[EDI]; }
asm { mov ECX,4[EDI]; }
asm { lea EDI,ifThis; }
asm { mov EAX,[EDI]; }
asm { mov EDX,4[EDI]; }
asm { mov EDI,here; }
asm { lock; }
asm { cmpxch8b[EDI]; }
asm { setz AL; }
asm { pop EBX; }
asm { pop EDI; }
}
else
{
pragma (msg, "Invalid template type specified.");
static assert(false);
}

}

}

}

}
}
}
else
{
    version (AsmX86_64)
{
    template cas(T,V1,V2) if (is(NakedType!(V1) == NakedType!(T)) && is(NakedType!(V2) == NakedType!(T)))
{
bool cas(shared(T)* here, const V1 ifThis, const V2 writeThis)
in
{
static if(T.sizeof > size_t.sizeof)
{
assert(atomicValueIsProperlyAligned!(size_t)(cast(size_t)here));
}
else
{
assert(atomicValueIsProperlyAligned!(T)(cast(size_t)here));
}

}
body
{
static if(T.sizeof == (byte).sizeof)
{
asm { mov DL,writeThis; }
asm { mov AL,ifThis; }
asm { mov RCX,here; }
asm { lock; }
asm { cmpxchg[RCX],DL; }
asm { setz AL; }
}
else
{
static if(T.sizeof == (short).sizeof)
{
asm { mov DX,writeThis; }
asm { mov AX,ifThis; }
asm { mov RCX,here; }
asm { lock; }
asm { cmpxchg[RCX],DX; }
asm { setz AL; }
}
else
{
static if(T.sizeof == (int).sizeof)
{
asm { mov EDX,writeThis; }
asm { mov EAX,ifThis; }
asm { mov RCX,here; }
asm { lock; }
asm { cmpxchg[RCX],EDX; }
asm { setz AL; }
}
else
{
static if(T.sizeof == (long).sizeof)
{
asm { mov RDX,writeThis; }
asm { mov RAX,ifThis; }
asm { mov RCX,here; }
asm { lock; }
asm { cmpxchg[RCX],RDX; }
asm { setz AL; }
}
else
{
pragma (msg, "Invalid template type specified.");
static assert(false);
}

}

}

}

}
}
}
}
}
version (unittest)
{
    template testCAS(msyT)
{
template testCAS(T)
{
void testCAS(T val = T.init + 1)
{
T base;
shared(T) atom;
assert(base != val);
assert(atom == base);
assert(cas(&atom,base,val));
assert(atom == val);
assert(!cas(&atom,base,base));
assert(atom == val);
}
}
}
    template testType(T)
{
void testType(T val = T.init + 1)
{
testCAS!(T)(val);
}
}
    }
