// D import file generated from 'src\core\sys\posix\sys\utsname.d'
module core.sys.posix.sys.utsname;
extern (C) version (linux)
{
    private enum utsNameLength = 65;

    struct utsname
{
    char[utsNameLength] sysname;
    char[utsNameLength] nodename;
    char[utsNameLength] release;
    char[utsNameLength] update;
    char[utsNameLength] machine;
    char[utsNameLength] __domainname;
}
    int uname(utsname* __name);
}
else
{
    version (OSX)
{
    private enum utsNameLength = 256;

    struct utsname
{
    char[utsNameLength] sysname;
    char[utsNameLength] nodename;
    char[utsNameLength] release;
    char[utsNameLength] update;
    char[utsNameLength] machine;
}
    int uname(utsname* __name);
}
else
{
    version (FreeBSD)
{
    private enum utsNameLength = 32;

    struct utsname
{
    char[utsNameLength] sysname;
    char[utsNameLength] nodename;
    char[utsNameLength] release;
    char[utsNameLength] update;
    char[utsNameLength] machine;
}
    int uname(utsname* __name);
}
}
}

