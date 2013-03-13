// PERMUTE_ARGS:
// REQUIRED_ARGS: -Jrunnable/extra-files
// EXTRA_FILES: extra-files/foo37.txt

import std.stdio;

void main()
{
    writefln(import("foo37.txt"));
}
