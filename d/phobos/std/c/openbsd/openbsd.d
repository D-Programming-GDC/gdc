
module std.c.openbsd.openbsd;

version (OpenBSD) { } else { static assert(0); }

public import std.c.openbsd.pthread;
public import gcc.config.libc : tm, time_t;
public import gcc.config.libc;

private import std.c.stdio;


//alias uint fflags_t;
//alias long blkcnt_t;
//alias uint blksize_t;
alias int dev_t;
alias uint id_t;
alias uint nlink_t;
alias ulong fsblkcnt_t;
alias ulong fsfilcnt_t;

