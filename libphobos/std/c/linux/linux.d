
/* Written by Walter Bright, Christopher E. Miller, and many others.
 * http://www.digitalmars.com/d/
 * Placed into public domain.
 * Linux(R) is the registered trademark of Linus Torvalds in the U.S. and other
 * countries.
 */

module std.c.linux.linux;

version (FreeBSD)
{
    public import std.c.freebsd.freebsd;
}
else version (OpenBSD)
{
    public import std.c.openbsd.openbsd;
}
else version (Solaris)
{
    public import std.c.solaris.solaris;
}
else version (OSX)
{
    public import std.c.osx.osx;
}
else
{
public import std.c.linux.linuxextern;
public import std.c.linux.pthread;
public import gcc.config.libc : tm, time_t;
public import gcc.config.unix;

private import std.c.stdio;

extern (C)
{
    int access(in char*, int);
    int open(in char*, int, ...);
    ssize_t read(int, void*, size_t);
    ssize_t write(int, in void*, size_t);
    int close(int);
    int lseek(int, off_t, int);
    int fstat(int, struct_stat*);
    int lstat(in char*, struct_stat*);
    int stat(in char*, struct_stat*);
    int chdir(in char*);
    int mkdir(in char*, int);
    int rmdir(in char*);
    char* getcwd(char*, size_t);
    int chmod(in char*, mode_t);
    int fork();
    int dup(int);
    int dup2(int, int);
    int pipe(int[2]);
    pid_t wait(int*);
    int waitpid(pid_t, int*, int);
    int raise(int);

    uint alarm(uint);
    char* basename(char*);
    //wint_t btowc(int);
    int chown(in char*, uid_t, gid_t);
    int chroot(in char*);
    size_t confstr(int, char*, size_t);
    int creat(in char*, mode_t);
    char* ctermid(char*);
    int dirfd(DIR*);
    char* dirname(char*);
    int fattach(int, char*);
    int fchmod(int, mode_t);
    int fdatasync(int);
    int ffs(int);
    int fmtmsg(int, char*, int, char*, char*, char*);
    int fpathconf(int, int);
    int fseeko(FILE*, off_t, int);
    off_t ftello(FILE*);

    extern char** environ;
}

extern (C)
{
    time_t time(time_t*);
    char* asctime(in tm*);
    char* ctime(in time_t*);
    tm* gmtime(in time_t*);
    tm* localtime(in time_t*);
    time_t mktime(tm*);
    char* asctime_r(in tm* t, char* buf);
    char* ctime_r(in time_t* timep, char* buf);
    tm* gmtime_r(in time_t* timep, tm* result);
    tm* localtime_r(in time_t* timep, tm* result);
}

/**************************************************************/
extern (C)
{
void* mmap(void*, size_t, int, int, int, off_t);

int munmap(void*, size_t);
int mprotect(void*, size_t, int);
int msync(void*, size_t, int);
int madvise(void*, size_t, int);
int mlock(void*, size_t);
int munlock(void*, size_t);
int mlockall(int);
int munlockall();
void* mremap(void*, size_t, size_t, int);
int mincore(void*, size_t, ubyte*);
int remap_file_pages(void*, size_t, int, size_t, int);
int shm_open(in char*, int, int);
int shm_unlink(in char*);
}

extern(C)
{
    DIR* opendir(in char* name);
    int closedir(DIR* dir);
    dirent* readdir(DIR* dir);
    void rewinddir(DIR* dir);
    off_t telldir(DIR* dir);
    void seekdir(DIR* dir, off_t offset);
}


extern(C)
{
	private import std.intrinsic;
	
	
	int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* errorfds, timeval* timeout);
	int fcntl(int s, int f, ...);
	
	
	enum
	{
		EINTR = 4,
		EINPROGRESS = 115,
	}
	
	
	int FDELT(int d)
	{
		return d / NFDBITS;
	}
	
	
	int FDMASK(int d)
	{
		return 1 << (d % NFDBITS);
	}
}

extern (C)
{
    /* From <dlfcn.h>
     * See http://www.opengroup.org/onlinepubs/007908799/xsh/dlsym.html
     * To use these functions, you'll need to link in /usr/lib/libdl.a
     * (compile/link with -L-ldl)
     */

    const int RTLD_NOW = 0x00002;	// Correct for Red Hat 8

    void* dlopen(in char* file, int mode);
    int   dlclose(void* handle);
    void* dlsym(void* handle, char* name);
    char* dlerror();
}

extern (C)
{
    /* from <pwd.h>
     */

    int getpwnam_r(char*, passwd*, void*, size_t, passwd**);
    passwd* getpwnam(in char*);
    passwd* getpwuid(uid_t);
    int getpwuid_r(uid_t, passwd*, char*, size_t, passwd**);
    int kill(pid_t, int);
}

extern (C)
{
    /* from signal.h
     */
    version (linux)
    {
	const size_t _SIGSET_NWORDS = 1024 / (8 * uint.sizeof);

        int sigismember(sigset_t*, int);
    }
}

extern (C)
{
    /* from utime.h
     */

    int utime(char* filename, utimbuf* buf);
}
}
