
/* Written by Walter Bright, Christopher E. Miller, and many others.
 * http://www.digitalmars.com/d/
 * Placed into public domain.
 */

module std.c.freebsd.freebsd;

version (FreeBSD) { } else { static assert(0); }

public import std.c.freebsd.pthread;
public import gcc.config.libc : tm, time_t;
public import gcc.config.unix;

private import std.c.stdio;

alias uint fflags_t;
alias long blkcnt_t;
alias uint blksize_t;
alias uint dev_t;
alias long id_t;
alias ushort nlink_t;
alias ulong fsblkcnt_t;
alias ulong fsfilcnt_t;

// sys/stat.h
extern (C)
{
    int access(in char*, int);
    int open(in char*, int, ...);
    int read(int, void*, int);
    int write(int, in void*, int);
    int close(int);
    int lseek(int, off_t, int);
    int fstat(int, struct_stat*);
    int lstat(in char*, struct_stat*);
    int stat(in char*, struct_stat*);
    int chdir(in char*);
    int mkdir(in char*, int);
    int rmdir(in char*);
    char* getcwd(char*, int);
    int chmod(in char*, mode_t);
    int fork();
    int dup(int);
    int dup2(int, int);
    int pipe(int[2]);
    pid_t wait(int*);
    int waitpid(pid_t, int*, int);

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

int madvise(void*, size_t, int);
int minherit(void*, size_t, int);
int munmap(void*, size_t);
int mprotect(void*, size_t, int);
int msync(void*, size_t, int);
int madvise(void*, size_t, int);
int mlock(void*, size_t);
int munlock(void*, size_t);
int mlockall(int);
int munlockall();
void* mremap(void*, size_t, size_t, int);
int mincore(/*const*/ void*, size_t, ubyte*);
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
		EINPROGRESS = 36,
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
     */

    const int RTLD_NOW = 2;

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
    const size_t _SIGSET_WORDS = 4;

    int sigismember(sigset_t*, int);
}

extern (C)
{
    /* from utime.h
     */

    int utime(in char* filename, in utimbuf* buf);
}

extern (C)
{
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

// sched.h

enum
{
    SCHED_FIFO = 1,
    SCHED_OTHER = 2,
    SCHED_RR = 3
}

struct sched_param
{
    int sched_priority;
}
