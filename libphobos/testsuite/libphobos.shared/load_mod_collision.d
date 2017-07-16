module lib; // module collides with lib.so

import core.runtime, core.stdc.stdio, core.sys.posix.dlfcn;

void main(string[] args)
{
    auto name = args[0];
    assert(name[$-23 .. $] == "/load_mod_collision.exe");
    name = name[0 .. $-22] ~ "lib.so";
    auto lib = Runtime.loadLibrary(name);
}
