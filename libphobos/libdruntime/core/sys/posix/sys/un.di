// D import file generated from 'src\core\sys\posix\sys\un.d'
module core.sys.posix.sys.un;
version (Posix)
{
    public import core.sys.posix.sys.socket;

}
extern (C) version (linux)
{
    struct sockaddr_un
{
    sa_family_t sun_family;
    byte[108] sun_path;
}
}
else
{
    version (OSX)
{
    struct sockaddr_un
{
    ubyte sun_len;
    sa_family_t sun_family;
    byte[104] sun_path;
}
}
else
{
    version (FreeBSD)
{
    struct sockaddr_un
{
    ubyte sun_len;
    sa_family_t sun_family;
    byte[104] sun_path;
}
}
}
}

