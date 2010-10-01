/**
 * C's &lt;stdlib.h&gt;
 * D Programming Language runtime library
 * Authors: Walter Bright, Digital Mars, http://www.digitalmars.com
 * License: Public Domain
 * Macros:
 *	WIKI=Phobos/StdCStdlib
 */


/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.

   Modified by David Friedman, February 2007
*/

module std.c.stdlib;

private import std.c.stddef;
private import std.stdint;

extern (C):

enum
{
    _MAX_PATH   = 260,
    _MAX_DRIVE  = 3,
    _MAX_DIR    = 256,
    _MAX_FNAME  = 256,
    _MAX_EXT    = 256,
}

///
struct div_t { int  quot,rem; }
///
struct ldiv_t { Clong_t quot,rem; }
///
struct lldiv_t { long quot,rem; }

    div_t div(int,int);	///
    ldiv_t ldiv(Clong_t, Clong_t); /// ditto
    lldiv_t lldiv(long, long); /// ditto

    const int EXIT_SUCCESS = 0;	///
    const int EXIT_FAILURE = 1;	/// ditto

    int    atexit(void (*)());	///
    void   exit(int);	/// ditto
    void   _exit(int);	/// ditto

    int system(const char *);

    version (GNU)
    {
	private import gcc.builtins;
	alias gcc.builtins.__builtin_alloca alloca;	///
    } else {
	void *alloca(size_t);	///
    }

    void *calloc(size_t, size_t);	///
    void *malloc(size_t);	/// ditto
    void *realloc(void *, size_t);	/// ditto
    void free(void *);	/// ditto

    void *bsearch(in void *,in void *,size_t,size_t,
       int function(in void *,in void *));	///
    void qsort(void *base, size_t nelems, size_t elemsize,
	int (*compare)(in void *elem1, in void *elem2));	/// ditto

    char* getenv(const char*);	///
    int   setenv(const char*, const char*, int); /// extension to ISO C standard, not available on all platforms
    int unsetenv(const char*); /// extension to ISO C standard, not available on all platforms

    version (GNU)
    {
	static import gcc.config.libc;
	alias gcc.config.libc.RAND_MAX RAND_MAX;
    }

    int    rand();	///
    void   srand(uint);	/// ditto
    Clong_t random(int num);	/// ditto
    void   randomize();	/// ditto

    int getErrno();	/// ditto
    int setErrno(int);	/// ditto

    version (GNU)
    {
	private import gcc.config.errno;
	alias gcc.config.errno.ERANGE ERANGE;
    }
    else
	enum int ERANGE = 34;	// on Windows, linux and OSX

double atof(in char *);	///
int    atoi(in char *);	/// ditto
Clong_t atol(in char *);	/// ditto
float  strtof(in char *,char **);	/// ditto
double strtod(in char *,char **);	/// ditto

//real   strtold(char *,char **);
version (darwin)
    version (GNU_Have_strtold)
	version = darwin_strtold;
version(PPC)
    version(Linux)
	version=PPCLinux;
version (darwin_strtold)
{
    private import std.c.darwin.ldblcompat;
    real strtold(in char *, char **);	/// ditto
    pragma(GNU_asm,strtold,"strtold"~__DARWIN_LDBL_COMPAT);
}
else version (PPCLinux)
{
    private import std.c.linux.ldblcompat;
    static if (std.c.linux.ldblcompat.__No_Long_Double_Math)
	alias strtod strtold; 	/// ditto
    else
	version = Default_Strtold;
}
else
    version = Default_Strtold;
version (Default_Strtold)
{
    private import gcc.config.config;
    static if (gcc.config.config.Have_strtold)
	extern (C) real strtold(in char*, char**); 	/// ditto
    else
    {
	static import gcc.support;
	alias gcc.support.strtold strtold; /// ditto
    }
}

long   strtol(in char *,char **,int);	/// ditto
uint   strtoul(in char *,char **,int);	/// ditto
long   atoll(in char *);	/// ditto
long   strtoll(in char *,char **,int);	/// ditto
ulong  strtoull(in char *,char **,int);	/// ditto

char* itoa(int, char*, int);
char* ultoa(Culong_t, char*, int);

int mblen(in char *s, size_t n);	///
int mbtowc(wchar_t *pwc, char *s, size_t n);	/// ditto
int wctomb(char *s, wchar_t wc);	/// ditto
size_t mbstowcs(wchar_t *pwcs, in char *s, size_t n);	/// ditto
size_t wcstombs(in char *s, wchar_t *pwcs, size_t n);	/// ditto
