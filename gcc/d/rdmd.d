// rdmd - a program to compile, cache and execute D programming
// language source files via either the command-line or as a
// 'pseudo shell script' on POSIX conforming Linux/Unix systems.
//
// Written by Dave Fladebo and released into the public domain as
// explained by http://creativecommons.org/licenses/publicdomain
// Windows version by Roberto Mariottini, GDC/Unix version by afb.
//
// This software is provided "AS IS" and without any express or
// implied warranties, including and without limitation to, any
// warranty of merchantability or fitness for any purpose.
//
// version 1.2

version (Windows)
{
  import std.c.windows.windows;

  string fileSeparator = "\\";
  string pathSeparator = ";";
  string objectExtension = ".obj";
  string exeExtension = ".exe";
}
else version (linux)
{
  import std.c.linux.linux;

  string fileSeparator = "/";
  string pathSeparator = ":";
  string objectExtension = ".o";
  string exeExtension = "";
}
else version (Unix)
{
  import std.c.unix.unix;

  string fileSeparator = "/";
  string pathSeparator = ":";
  string objectExtension = ".o";
  string exeExtension = "";
}
else static assert(0);

version (DigitalMars)
    version (linux)
        version = Unix;

bool verbose = false;

import std.c.stdlib, std.file, std.md5, std.process, std.stdio, std.string;

version (Unix)
{
  extern(C) ushort getuid(); // moved to top, because of mac linker issues
}

int main(string[] args)
{
    int retval = -1;
    bool havefile = false, force = false;
    string[] cmpv, argv;    // cmpv = compiler arguments, argv = program arguments
    string exepath, dfilepath, compiler = "dmd", tmpdir = "/tmp";

    version (GNU)
    {
        compiler = "gdmd";
    }
    version (Windows)
    {
        tmpdir = toString(getenv("TEMP"));
    }

    .myname = args[0];
    .defcmp = compiler;

    foreach(int i, string arg; args)
    {
        if(i == 0)
            continue;

        if(find(arg,".d") >= 0  ||  find(arg,".ds") >= 0)
        {
            havefile = true;
            dfilepath = arg;
        }
        else
        {
            if(havefile == false)
            {
                bool skip = false;
                if(arg == "--help")
                    usage;
                else if(arg == "--force")
                    skip = force = true;
                else if(arg == "--verbose")
                    skip = verbose = true;
                else
                {
                    const string cs = "--compiler=";
                    if(arg.length > cs.length && arg[0..cs.length] == cs)
                    {
                        compiler = split(arg,"=")[1];
                        skip = true;
                    }
                    const string td = "--tmpdir=";
                    if(arg.length > td.length && arg[0..td.length] == td)
                    {
                        tmpdir = split(arg,"=")[1];
                        skip = true;
                    }
                }

                if(!skip)
                    cmpv ~= arg;
            }
            else
                argv ~= arg;
        }
    }

    if(!havefile)
        error("Couldn't find any D source code file to compile or execute.", retval);

    if(compile(tmpdir,compiler,force,dfilepath,cmpv,exepath))
    {
        string[] exeargv;
        version (Windows)
        {
        exeargv ~= "\"" ~ exepath ~ "\"";
        foreach(string arg; argv) exeargv ~= "\"" ~ arg ~ "\"";
        }
        else
        {
        exeargv ~= exepath;
        foreach(string arg; argv) exeargv ~= arg;
        }

        if(verbose)
        {
            fwritef(stderr,"running: ");
            foreach(string arg; exeargv)
            {
                fwritef(stderr,arg," ");
            }
            fwritefln(stderr);
        }

        // execute
        version (Windows)
        {
            retval = spawnvp(std.c.process._P_WAIT, exepath, exeargv);
        }
        else
        {
            retval = spawnapp(exepath,exeargv);
        }
    }
    else
    {
        try { std.file.remove(exepath); } catch {}
        error("Couldn't compile or execute " ~ dfilepath ~ ".", retval);
    }

    return retval;
}

string myname;
string defcmp;
void error(string errmsg, int errno)
{
    fwritefln(stderr,myname,": ",errmsg);
    exit(errno);
}

void usage()
{
    fwritefln(stderr,"Usage:");
    fwritefln(stderr,"  ",myname," [D compiler arguments] [",myname," arguments] progfile.d [program arguments]");
    fwritefln(stderr);
    fwritefln(stderr,myname," arguments:");
    fwritefln(stderr,"  --help\t\tThis message");
    fwritefln(stderr,"  --force\t\tForce re-compilation of source code [default = do not force]");
    fwritefln(stderr,"  --verbose\t\tShow detailed info of operations [default = do not show]");
    fwritefln(stderr,"  --compiler=(dmd|gdmd)\tSpecify compiler [default = "~ .defcmp ~"]");
    fwritefln(stderr,"  --tmpdir=tmp_dir_path\tSpecify directory to store cached program and other temporaries [default = /tmp]");
    fwritefln(stderr);
    fwritefln(stderr,"Notes:");
    fwritefln(stderr,"  dmd or gdmd must be in the current user context $PATH");
    fwritefln(stderr,"  ",myname," does not support execution of D source code via stdin");
    fwritefln(stderr,"  ",myname," will only compile and execute files with a '.d' file extension");
    exit(EXIT_SUCCESS);
}

bool compile(string tmpdir, string compiler, bool force, string dfilepath, string[] cmpv, ref string exepath)
{
    int retval = 0;

    struct_stat dfilestat;  // D source code file status info.
    int filrv = stat(toStringz(dfilepath),&dfilestat);

    string[] pathcomps = split(dfilepath,fileSeparator);
    string exefile = split(pathcomps[$-1],".")[0];

    string cmdline = compiler ~ " -quiet";
    foreach(string str; cmpv)
        if(str != "")
            cmdline ~= " " ~ str;

    // MD5 sum of compiler arguments
    ubyte[16] digest;
    sum(digest,cast(void[])cmdline);

    // directory for temp. files
    if(!tmpdir.length)
        tmpdir = "/tmp/";
    else
        if(tmpdir[$-1] != fileSeparator[0])
            tmpdir = tmpdir ~ fileSeparator;

    // exe filename format is basename-uid-filesysdev-inode-MD5
    // append MD5 sum of the compiler arguments onto the file name to force recompile if they have changed
    string uid_str;
    version(Windows)
        uid_str = getuid();
    else
        uid_str = toString(getuid());
    exepath = tmpdir ~ exefile ~ "-" ~ uid_str ~ "-" ~ toString(dfilestat.st_dev) ~ "-" ~ toString(dfilestat.st_ino) ~ "-" ~ digestToString(digest) ~ exeExtension;

    struct_stat exestat;    // temp. executable status info.
    int exerv = stat(toStringz(exepath),&exestat);
    if(force ||                                 // force compilation
       exerv ||                                 // stat returned an error (e.g.: no exefile)
       dfilestat.st_mtime > exestat.st_mtime || // source code file is newer than executable
       progstat(.myname).st_mtime > exestat.st_mtime || // this program is newer than executable
       progstat(compiler).st_mtime > exestat.st_mtime)  // compiler is newer than executable
    {
        cmdline ~= " " ~ dfilepath ~ " -of" ~ exepath ~ " -od" ~ tmpdir;
        if(verbose)
        {
            fwritefln(stderr,"running: ",cmdline);
        }
        retval = std.process.system(cmdline);   // compile ("system" is also in std.c.stdlib)
        chmod(toStringz(exepath),0700);
    }

    // remove object file
    try { std.file.remove(tmpdir ~ exefile ~ objectExtension); } catch {}

    return cast(bool)(retval == 0);
}

struct_stat progstat(string program)
{
    struct_stat progstat;  // D source code file status info.

    try
    {
        int prgrv;
        if(find(program,fileSeparator) >= 0)
            prgrv = stat(toStringz(program), &progstat);
        else
        {
            // There's got to be a better way...
            string[] pathdirs = split(toString(getenv("PATH")),pathSeparator);
            foreach(string dir; pathdirs)
            {
                prgrv = stat(toStringz(dir ~ fileSeparator ~ program), &progstat);
                if (prgrv == 0)
                {
                    break;
                }
            }
        }
    }
    catch {}

    return progstat;
}

version (Unix)
{
extern(C) char* strerror(int);

int spawnapp(string pathname, string[] argv)
{
    int retval = 0;
    pid_t pid = fork();

    if(pid != -1)
    {
        if(pid == 0)
        {
            execv(pathname,argv);
            goto Lerror;
        }

        while(1)
        {
            int status;
            pid_t wpid = waitpid(pid, &status, 0);
            if(exited(status))
            {
                retval = exitstatus(status);
                break;
            }
            else if(signaled(status))
            {
                retval = -termsig(status);
                break;
            }
            else if(stopped(status)) // ptrace support
                continue;
            else
                goto Lerror;
        }

        return retval;
    }

Lerror:
    retval = getErrno;
    error("Cannot spawn " ~ pathname ~ "; " ~ toString(strerror(retval)) ~ " [errno " ~ toString(retval) ~ "]", retval);
    return retval;
}

bool stopped(int status)    { return cast(bool)((status & 0xff) == 0x7f); }
bool signaled(int status)   { return cast(bool)((cast(char)((status & 0x7f) + 1) >> 1) > 0); }
int  termsig(int status)    { return status & 0x7f; }
bool exited(int status)     { return cast(bool)((status & 0x7f) == 0); }
int  exitstatus(int status) { return (status & 0xff00) >> 8; }

} // version (Unix)

version (Windows)
{
  extern (Windows)
  {
    BOOL GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime,
                     LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
    BOOL GetUserNameA(LPTSTR lpBuffer, LPDWORD nSize);
  }

  // fake struct stat
  struct struct_stat
  {
    ulong st_dev;
    uint st_ino;
    ulong st_mtime;
  }

  // fake stat function
  int stat(char* name, struct_stat* st)
  {
    int retval = -1;
    HANDLE h = CreateFileA(name, FILE_GENERIC_READ, 0, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
    if (h != INVALID_HANDLE_VALUE)
    {
      FILETIME lastWriteTime;
      if (GetFileTime(h, null, null, &lastWriteTime))
      {
        st.st_mtime = lastWriteTime.dwHighDateTime;
        st.st_mtime <<= 32;
        st.st_mtime |= lastWriteTime.dwLowDateTime;
        retval = 0;
      }
      CloseHandle(h);
    }
    if(verbose)
    {
      fwritefln(stderr,"stat: ",toString(name)," : ",retval);
    }
    return retval;
  }

  // fake getuid function
  char[] getuid()
  {
    char[] buffer;
    DWORD size = buffer.length = 64;
    if(GetUserNameA(buffer.ptr, &size))
    {
      buffer.length = size;
      return buffer;
    }
    return "";
  }

  // fake chmod function
  void chmod(...)
  {
  }
}
