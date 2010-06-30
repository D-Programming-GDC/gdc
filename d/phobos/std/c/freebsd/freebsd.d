
/* Written by Walter Bright, Christopher E. Miller, and many others.
 * http://www.digitalmars.com/d/
 * Placed into public domain.
 */

module std.c.freebsd.freebsd;

version (FreeBSD) { } else { static assert(0); }

public import std.c.dirent;
private import std.c.stdio;

extern (C)
{
    /* From <dlfcn.h>
     * See http://www.opengroup.org/onlinepubs/007908799/xsh/dlsym.html
     */

    const int RTLD_NOW = 2;

    void* dlopen(in char* file, int mode);
    int   dlclose(void* handle);
    void* dlsym(void* handle, char* name);
    char* dlerror();
    
    extern
    {
	void* __libc_stack_end;
	int __data_start;
	int _end;
	int timezone;
	
	void *_deh_beg;
	void *_deh_end;
    }
}

