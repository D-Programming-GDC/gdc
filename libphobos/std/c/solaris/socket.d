/*
        Written by Christopher E. Miller
        Placed into public domain.
*/


module std.c.solaris.socket;

private import std.stdint;
private import std.c.solaris.solaris;

version (Solaris) { } else { static assert(0); }

extern(C):

enum: int
{
    // netinet/in.h
    IP_MULTICAST_LOOP =  0x12,
    IP_ADD_MEMBERSHIP =  0x13,
    IP_DROP_MEMBERSHIP = 0x14,
}

enum: int
{
    SHUT_RD   =  0,
    SHUT_WR   =  1,
    SHUT_RDWR =  2,
}

enum: int       // Not defined on Solaris, but that's okay.
{
    SD_RECEIVE =  SHUT_RD,
    SD_SEND =     SHUT_WR,
    SD_BOTH =     SHUT_RDWR,
}

int socket(int af, int type, int protocol);
int bind(int s, /*const*/ sockaddr* name, int namelen);
int connect(int s, /*const*/ sockaddr* name, int namelen);
int listen(int s, int backlog);
int accept(int s, sockaddr* addr, int* addrlen);
int shutdown(int s, int how);
int getpeername(int s, sockaddr* name, int* namelen);
int getsockname(int s, sockaddr* name, int* namelen);
int send(int s, void* buf, int len, int flags);
int sendto(int s, void* buf, int len, int flags, sockaddr* to, int tolen);
int recv(int s, void* buf, int len, int flags);
int recvfrom(int s, void* buf, int len, int flags, sockaddr* from, int* fromlen);
int getsockopt(int s, int level, int optname, void* optval, int* optlen);
int setsockopt(int s, int level, int optname, void* optval, int optlen);
uint inet_addr(char* cp);
char* inet_ntoa(in_addr);
hostent* gethostbyname(char* name);
int gethostbyname_r(char* name, hostent* ret, void* buf, size_t buflen, hostent** result, int* h_errnop);
hostent* gethostbyaddr(void* addr, int len, int type);
protoent* getprotobyname(char* name);
protoent* getprotobynumber(int number);
servent* getservbyname(char* name, char* proto);
servent* getservbyport(int port, char* proto);
int gethostname(char* name, int namelen);
int getaddrinfo(char* nodename, char* servname, addrinfo* hints, addrinfo** res);
void freeaddrinfo(addrinfo* ai);
int getnameinfo(sockaddr* sa, socklen_t salen, char* node, socklen_t nodelen, char* service, socklen_t servicelen, int flags);


version(BigEndian)
{
        ushort htons(ushort x)
        {
                return x;
        }


        uint htonl(uint x)
        {
                return x;
        }
}
else version(LittleEndian)
{
        private import std.intrinsic;


        ushort htons(ushort x)
        {
                return cast(ushort)((x >> 8) | (x << 8));
        }


        uint htonl(uint x)
        {
                return bswap(x);
        }
}
else
{
        static assert(0);
}


ushort ntohs(ushort x)
{
        return htons(x);
}


uint ntohl(uint x)
{
        return htonl(x);
}


// netdb.h
enum: int
{
        AI_PASSIVE = 0x8,
        AI_CANONNAME = 0x10,
        AI_NUMERICHOST = 0x20,
        AI_NUMERICSERV = 0x40,
}


const uint INET_ADDRSTRLEN = 16;
const uint INET6_ADDRSTRLEN = 46;

