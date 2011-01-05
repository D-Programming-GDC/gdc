module std.c.freebsd.socket;

/**
 * The OSX implementation is the same as the FreeBSD implementation, so we'll
 * just use that one for now.  In reality, the OSX one should be including the
 * FreeBSD one to show the correct lineage of the operating systems, but as a
 * temp fix this is good enough.
 **/

public import std.c.osx.socket;
