import core.runtime, core.time : MonoTime;
import core.stdc.stdio;

ModuleInfo* getModuleInfo(string name)
{
    foreach (m; ModuleInfo)
        if (m.name == name) return m;
    assert(0, "module '"~name~"' not found");
}

// Call this program for every compiled in module
bool testAll()
{
    import core.stdc.stdlib, core.stdc.inttypes, core.stdc.string;
    bool success = true;
    uint num;
    foreach (m; ModuleInfo)
    {
        num++;
        string command = Runtime.args[0] ~ " " ~ m.name ~ "\0";
        if (system(command.ptr) != EXIT_SUCCESS)
            success = false;
    }
    printf(("%" ~ PRIu32[0 .. strlen(PRIu32)] ~ " modules tested\n\0").ptr, num);
    return success;
}

bool tester()
{
    assert(Runtime.args.length == 2);
    auto name = Runtime.args[1];
    immutable pkg = ".package";
    immutable pkgLen = pkg.length;

    debug string mode = "debug";
    else string mode =  "release";
    static if ((void*).sizeof == 4) mode ~= "32";
    else static if ((void*).sizeof == 8) mode ~= "64";
    else static assert(0, "You must be from the future!");

    if (name.length > pkgLen && name[$ - pkgLen .. $] == pkg)
        name = name[0 .. $ - pkgLen];

    if (auto fp = getModuleInfo(name).unitTest)
    {
        try
        {
            immutable t0 = MonoTime.currTime;
            fp();
            printf("%.3fs PASS %.*s %.*s\n",
                (MonoTime.currTime - t0).total!"msecs" / 1000.0,
                cast(uint)mode.length, mode.ptr,
                cast(uint)name.length, name.ptr);
        }
        catch (Throwable e)
        {
            auto msg = e.toString();
            printf("****** FAIL %.*s %.*s\n%.*s\n",
                cast(uint)mode.length, mode.ptr,
                cast(uint)name.length, name.ptr,
                cast(uint)msg.length, msg.ptr);
            return false;
        }
    }
    return true;
}

shared static this()
{
    if (Runtime.args.length == 1)
        Runtime.moduleUnitTester = &testAll;
    else
        Runtime.moduleUnitTester = &tester;
}

void main()
{
}

