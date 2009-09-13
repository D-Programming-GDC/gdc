
/**
 * C's &lt;string.h&gt;
 * Authors: Walter Bright, Digital Mars, www.digitalmars.com
 * License: Public Domain
 * Macros:
 *	WIKI=Phobos/StdCString
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.

   Modified by David Friedman, May 2006
*/


module std.c.string;

extern (C):

version (GNU)
{
    private import gcc.builtins;
    alias __builtin_memcpy memcpy;	///
    alias __builtin_strcpy strcpy;	///
    alias __builtin_strncpy strncpy;	///
    alias __builtin_strncat strncat;	///
    alias __builtin_strncmp strncmp;	///
    alias __builtin_strchr strchr;	///
    alias __builtin_strcspn strcspn;	///
    alias __builtin_strpbrk strpbrk;	///
    alias __builtin_strrchr strrchr;	///
    alias __builtin_strspn strspn;	///
    alias __builtin_strstr strstr;	///
    alias __builtin_memset memset;	///
    alias __builtin_strlen strlen;	///
    alias __builtin_strcmp strcmp;	///
    alias __builtin_strcat strcat;	///
    alias __builtin_memcmp memcmp;	///
}
else
{
void* memcpy(void* s1, in void* s2, size_t n);	///
char* strcpy(char* s1, in char* s2);		///
char* strncpy(char* s1, in char* s2, size_t n);	///
char* strncat(char*  s1, in char*  s2, size_t n);	///
int strncmp(in char* s1, in char* s2, size_t n);	///
char* strchr(in char* s, int c);			///
size_t strcspn(in char* s1, in char* s2);		///
char* strpbrk(in char* s1, in char* s2);		///
char* strrchr(char* s, int c);			///
size_t strspn(in char* s1, in char* s2);		///
char* strstr(in char* s1, in char* s2);		///
void* memset(void* s, int c, size_t n);		///
size_t strlen(in char* s);				///
int strcmp(in char* s1, in char* s2);			///
char* strcat(char* s1, in char* s2);		///
int memcmp(in void* s1, in void* s2, size_t n);	///
}
void* memmove(void* s1, void* s2, size_t n);	///
size_t strxfrm(char*  s1, in char*  s2, size_t n);	///
int strcoll(in char* s1, in char* s2);		///
void* memchr(in void* s, int c, size_t n);		///
char* strtok(char*  s1, in char*  s2);		///
const(char)* strerror(int errnum);			///
const(char)* strerror_r(int errnum, char* buf, size_t buflen);	///

version (Windows)
{
    int memicmp(in char* s1, in char* s2, size_t n);	///
}

// Original DMD strerror_r is non-portable glibc version
const(char*) _d_gnu_cbridge_strerror(int errnum, char* buf, size_t buflen);
