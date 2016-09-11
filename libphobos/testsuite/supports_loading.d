version (linux) {}
else {static assert(false);}

extern(C) void* rt_loadLibrary(const char* name);

int main()
{
    return !rt_loadLibrary("foo.so");
}
