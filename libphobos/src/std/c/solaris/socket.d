// Written in the D programming language.

/*
 * This module is just for making std.socket work under Solaris, and these
 * definitions should actually be in druntime. (core.sys.posix.netdb or sth)
 */
module std.c.solaris.socket;

public import core.sys.posix.netdb;
import core.sys.posix.sys.socket;

extern(C):

enum
{
    AF_APPLETALK    = 16,
    AF_IPX          = 23,
}

//enum // <sys/socket.h>
//{
//    SOCK_RDM        = 4,
//}

//enum
//{
//    MSG_NOSIGNAL    = 0x20000,
//}

enum
{
    IPPROTO_IGMP    = 2,
    IPPROTO_GGP     = 3,
    IPPROTO_PUP     = 12,
    IPPROTO_IDP     = 22,
    IPPROTO_ND      = 77,
    IPPROTO_MAX     = 256,
}

enum // <netinet/in.h>
{
    INADDR_LOOPBACK = 0x7f000001,
    INADDR_NONE     = 0xffffffff,
}
