import core.runtime, core.sys.posix.dlfcn;

extern(C) alias RunDepTests = int function();

void main(string[] args)
{
    auto name = args[0];
    assert(name[$-17 .. $] == "/load_linkdep.exe");
    name = name[0 .. $-16] ~ "liblinkdep.so";

    auto h = Runtime.loadLibrary(name);
    assert(h);
    auto runDepTests = cast(RunDepTests)dlsym(h, "runDepTests");
    assert(runDepTests());
    assert(Runtime.unloadLibrary(h));
}
