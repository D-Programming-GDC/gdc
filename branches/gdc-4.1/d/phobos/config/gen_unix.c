/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* eventually #ifdef HAVE_SYS_MMAN... */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
//#include <netinet6/in6.h>
#include <netdb.h>
#include <errno.h>
#include <pwd.h>

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

/* for AIX */
#ifndef NFDBITS
#include <sys/select.h>
#endif

#include "makestruct.h"

void c_types() {
    printf("const int NFDBITS = %d;\n", NFDBITS);
    printf("const int __FD_SET_SIZEOF = %d;\n", sizeof(fd_set));
    printf("const int FD_SETSIZE = %d;\n", FD_SETSIZE);
    INT_TYPE(mode_t);
    INT_TYPE(pid_t);
    INT_TYPE(uid_t);
    INT_TYPE(gid_t);
    INT_TYPE(ssize_t);
    printf("\n");
}

void c_dirent()
{
#ifdef DT_REG
    printf("enum {\n");
#ifdef DT_UNKNOWN
    CES( DT_UNKNOWN );
#endif
#ifdef DT_FIFO
    CES( DT_FIFO );
#endif
#ifdef DT_CHR
    CES( DT_CHR );
#endif
#ifdef DT_DIR
    CES( DT_DIR );
#endif
#ifdef DT_BLK
    CES( DT_BLK );
#endif
#ifdef DT_REG
    CES( DT_REG );
#endif
#ifdef DT_LNK
    CES( DT_LNK );
#endif
#ifdef DT_SOCK
    CES( DT_SOCK );
#endif
#ifdef DT_WHT
    CES( DT_WHT );
#endif
    printf("}\n");
#endif
}

void c_fcntl()
{
    printf("enum {\n");
    CES( O_RDONLY );
    CES( O_WRONLY );
    CES( O_RDWR );
    CES( O_NONBLOCK );
    CES( O_CREAT );
    CES( O_EXCL );
    CES( O_TRUNC );
    CES( O_APPEND );
#ifdef O_NOFOLLOW
    CES( O_NOFOLLOW );
#endif
    printf("}\n");
    printf("\n");
    printf("enum {\n");
    CES( F_DUPFD );
    CES( F_GETFD );
    CES( F_SETFD );
    CES( F_GETFL );
    CES( F_SETFL );
    // todo: locking, signaling, misc features
    printf("}\n");

    printf("enum {\n");
    CES( F_OK );
    CES( R_OK );
    CES( W_OK );
    CES( X_OK );
    printf("}\n");
    printf("\n");

    printf("\n");
}

void c_time() {
    time_t cool;
    INT_TYPE(time_t);

    {
	FieldInfo fi[2];
	struct timespec rec;
	INT_FIELD(fi[0], tv_sec);
	INT_FIELD(fi[1], tv_nsec);
	finish_struct(fi, 2, sizeof(rec), "timespec");
    }
    {
	FieldInfo fi[2];
	struct timeval rec;
	INT_FIELD(fi[0], tv_sec);
	INT_FIELD(fi[1], tv_usec);
	finish_struct(fi, 2, sizeof(rec), "timeval");
    }
    {
	FieldInfo fi[2];
	struct timezone rec;
	INT_FIELD(fi[0], tz_minuteswest);
	INT_FIELD(fi[1], tz_dsttime);
	finish_struct(fi, 2, sizeof(rec), "timezone");
    }
}

void c_utime()
{
    {
	FieldInfo fi[2];
	struct utimbuf rec;
	INT_FIELD(fi[0], actime);
	INT_FIELD(fi[1], modtime);
	finish_struct(fi, 2, sizeof(rec), "utimbuf");
    }
}

void c_stat()
{
    printf("enum {\n");
    CES( S_IFIFO );
    CES( S_IFCHR );
    CES( S_IFDIR );
    CES( S_IFBLK );
    CES( S_IFREG );
    CES( S_IFLNK );
    CES( S_IFSOCK );
    CES( S_IFMT );
    CES( S_IRUSR );
    CES( S_IWUSR );
    CES( S_IXUSR );
    CES( S_IRGRP );
    CES( S_IWGRP );
    CES( S_IXGRP );
    CES( S_IROTH );
    CES( S_IWOTH );
    CES( S_IXOTH );
    CES( S_IRWXG ); // standard?
    CES( S_IRWXO );
    printf("}\n\n");
    
    FieldInfo fi[13];
    struct stat rec;
    INT_FIELD(fi[0], st_dev); 
    INT_FIELD(fi[1], st_ino); 
    INT_FIELD(fi[2], st_mode); 
    INT_FIELD(fi[3], st_nlink); 
    INT_FIELD(fi[4], st_uid); 
    INT_FIELD(fi[5], st_gid); 
    INT_FIELD(fi[6], st_rdev); 
    INT_FIELD(fi[7], st_size); 
    INT_FIELD(fi[8], st_blksize); 
    INT_FIELD(fi[9], st_blocks); 
    INT_FIELD(fi[10], st_atime); 
    INT_FIELD(fi[11], st_mtime); 
    INT_FIELD(fi[12], st_ctime);
    finish_struct(fi, 13, sizeof(rec), "struct_stat");
}

void c_signal()
{
    printf("// from <sys/signal.h>\n");
    printf("enum {\n");
    CES( SIGHUP );
    CES( SIGINT );
    CES( SIGQUIT );
    CES( SIGILL );
    CES( SIGABRT );
#ifdef SIGIOT
    CES( SIGIOT );
#else
    CESZ( SIGIOT );
#endif
    CES( SIGBUS );
    CES( SIGFPE );
    CES( SIGKILL );
    CES( SIGUSR1 );
    CES( SIGSEGV );
    CES( SIGUSR2 );
    CES( SIGPIPE );
    CES( SIGALRM );
    CES( SIGTERM );
#ifdef SIGSTKFLT
    CES( SIGSTKFLT );
#else
    CESZ( SIGSTKFLT );
#endif
    CES( SIGCHLD );
    CES( SIGCONT );
    CES( SIGSTOP );
    CES( SIGTSTP );
    CES( SIGTTIN );
    CES( SIGTTOU );
    CES( SIGIO );
#ifdef SIGPOLL
    CES( SIGPOLL );
#else
    CESZ( SIGPOLL );
#endif    
#ifdef SIGPROF
    CES( SIGPROF );
#else
    CESZ( SIGPROF );
#endif    
#ifdef SIGWINCH
    CES( SIGWINCH );
#else
    CESZ( SIGWINCH );
#endif    
    CES( SIGURG );
#ifdef SIGXCPU
    CES( SIGXCPU );
#else
    CESZ( SIGXCPU );
#endif
#ifdef SIGXFSZ
    CES( SIGXFSZ );
#else
    CESZ( SIGXFSZ );
#endif
#ifdef SIGVTALRM
    CES( SIGVTALRM );
#else
    CESZ( SIGVTALRM );
#endif
#ifdef SIGTRAP
    CES( SIGTRAP );
#else
    CESZ( SIGTRAP );
#endif
#ifdef SIGPWR
    CES( SIGPWR );
#else
    CESZ( SIGPWR );
#endif
#ifdef SIGSYS
    CES( SIGSYS );
#else
    CESZ( SIGSYS );
#endif
    printf("}\n");

    printf("enum {\n");
    CES( SA_NOCLDSTOP );
#ifdef SA_NOCLDWAIT
    CES( SA_NOCLDWAIT );
#else
    CESZ( SA_NOCLDWAIT );
#endif
#ifdef SA_SIGINFO
    CES( SA_SIGINFO );
#else
    CESZ( SA_SIGINFO );
#endif

#ifndef SA_ONSTACK
#ifdef SA_STACK
#define SA_ONSTACK SA_STACK
#endif
#endif

#ifdef SA_ONSTACK
    CES( SA_ONSTACK );
#else
    CESZ( SA_ONSTACK );
#endif
    CES( SA_RESTART );
    CES( SA_NODEFER );
#ifdef SA_RESETHAND
    CES( SA_RESETHAND );
#else
    CESZ( SA_RESETHAND );
#endif
    printf("}\n");
    printf("\n");

    // configure test that it is integral?
    OPAQUE_TYPE(sigset_t);

    printf("alias  void function(int) __sighandler_t;\n");
    printf("const __sighandler_t SIG_DFL = cast(__sighandler_t) %d;\n", SIG_DFL);
    printf("const __sighandler_t SIG_IGN = cast(__sighandler_t) %d;\n", SIG_IGN);
    printf("const __sighandler_t SIG_ERR = cast(__sighandler_t) %d;\n", SIG_ERR);
    printf("\n");
#ifdef HAVE_SIGINFO_T
    {
	siginfo_t rec;
	FieldInfo fi[3];
	INT_FIELD(fi[0],si_signo);
	INT_FIELD(fi[1],si_errno);
	INT_FIELD(fi[2],si_code);
	printf("/* siginfo_t is not finished... see gen_unix.c */\n");
	finish_struct(fi, 3, sizeof(rec), "siginfo_t");
    }
#else
    printf("struct siginfo_t;\n");
#endif

    {
	struct sigaction rec;
	FieldInfo fi[4];
	int n = 0;
	ADD_FIELD(fi[n], "void function(int)",  sa_handler);
	n++;
#ifdef HAVE_SIGINFO_T
	ADD_FIELD(fi[n], "void function(int, siginfo_t *, void *)", sa_sigaction);
	n++;
#endif
	ADD_FIELD(fi[n], "sigset_t", sa_mask);
	n++;
	INT_FIELD(fi[n], sa_flags);
	n++;
	//FN_FIELD(fi[4], "void (*sa_restorer)(void)", sa_restorer);
	//ADD_FIELD(fi[4], "void function(void)", sa_restorer);
	finish_struct(fi, n, sizeof(rec), "sigaction_t");
    }
    // not sure about this

}

void c_mman()
{
    printf("// from <sys/mman.h>\n");
#ifdef MAP_FAILED
    printf("extern(D) const void * MAP_FAILED = cast(void *) %d;\n", (int) MAP_FAILED); // %% int/ptr issues
#endif
#ifdef PROT_NONE
    printf("enum { PROT_NONE = %d, PROT_READ = %d, PROT_WRITE = %d, PROT_EXEC = %d }\n",
	PROT_NONE, PROT_READ, PROT_WRITE, PROT_EXEC);
    // there are more flags, but this is all that is needed for GC and other modules
#endif
#ifdef MAP_SHARED
    printf(
	   "enum { MAP_SHARED = 0x%x, MAP_PRIVATE = 0x%x, MAP_ANON = 0x%x,"
	   "       MAP_ANONYMOUS = 0x%x,",
	   MAP_SHARED, MAP_PRIVATE,
	   // not sure if these are alway macros, if not, just add an autoconf test
#ifdef MAP_ANON
	   MAP_ANON, MAP_ANON
#else
	   MAP_ANONYMOUS, MAP_ANONYMOUS
#endif
	   );
#ifdef MAP_TYPE
    CES( MAP_TYPE );
#endif
#ifdef MAP_FIXED
    CES( MAP_FIXED );
#endif
#ifdef MAP_FILE
    CES( MAP_FILE );
#endif
#ifdef MAP_GROWSDOWN
    CES( MAP_GROWSDOWN );
#endif
#ifdef MAP_DENYWRITE
    CES( MAP_DENYWRITE );
#endif
#ifdef MAP_EXECUTABLE
    CES( MAP_EXECUTABLE );
#endif
#ifdef MAP_LOCKED
    CES( MAP_LOCKED );
#endif
#ifdef MAP_NORESERVE
    CES( MAP_NORESERVE );
#endif
#ifdef MAP_POPULATE
    CES( MAP_POPULATE );
#endif
#ifdef MAP_NONBLOCK
    CES( MAP_NONBLOCK );
#endif
    printf("}\n\n");
#endif

    printf("enum {\n");
#ifdef MS_ASYNC
    CES( MS_ASYNC );
#else
    CESZ( MS_ASYNC );
#endif
#ifdef MS_INVALIDATE
    CES( MS_INVALIDATE );
#else
    CESZ( MS_INVALIDATE );
#endif
#ifdef MS_SYNC
    CES( MS_SYNC );
#else
    CESZ( MS_SYNC );
#endif
    printf("}\n\n");

    printf("enum {\n");
#ifdef MCL_CURRENT
    CES( MCL_CURRENT );
#else
    CESZ( MCL_CURRENT );
#endif
#ifdef MCL_FUTURE
    CES( MCL_FUTURE );
#else
    CESZ( MCL_FUTURE );
#endif
    printf("}\n\n");

#ifdef MREMAP_MAYMOVE
    printf("enum { MREMAP_MAYMOVE = %d\n}\n\n", MREMAP_MAYMOVE);
#endif

    printf("enum {\n");
#ifdef MADV_NORMAL
    CES( MADV_NORMAL );
#else
    CESZ( MADV_NORMAL );
#endif
#ifdef MADV_RANDOM
    CES( MADV_RANDOM );
#else
    CESZ( MADV_RANDOM );
#endif 
#ifdef MADV_SEQUENTIAL
    CES( MADV_SEQUENTIAL );
#else
    CESZ( MADV_SEQUENTIAL );
#endif
#ifdef MADV_WILLNEED
    CES( MADV_WILLNEED );
#else
    CESZ( MADV_WILLNEED );
#endif
#ifdef MADV_DONTNEED
    CES( MADV_DONTNEED );
#else
    CESZ( MADV_DONTNEED );
#endif
    printf("}\n\n");
    
    printf("\n");
}

void c_errno()
{
    printf("enum {\n");
    CES( EPERM );
    CES( ENOENT );
    CES( ESRCH );
    CES( EINTR );
    CES( ENXIO );
    CES( E2BIG );
    CES( ENOEXEC );
    CES( EBADF );
    CES( ECHILD );
#ifdef EINPROGRESS
    CES( EINPROGRESS );
#endif
#ifdef EWOULDBLOCK
    CES( EWOULDBLOCK );
#endif
#ifdef EAGAIN
    CES( EAGAIN );
#endif
    
    /* etc... */
    printf("}\n");
    printf("\n");
}

void c_semaphore() {
    unsigned sem_t_size = 0;
#ifdef HAVE_SEMAPHORE_H
    sem_t_size = sizeof(sem_t);
#endif
    printf("struct sem_t { ubyte[%u] opaque; }\n", sem_t_size);
}

void c_pthread() {
#ifdef HAVE_PTHREAD_H
    // pthread_t is a pointer on some platforms... probably not too important
    // would have to write a config test; can't think of a way to detect
    // this in C code
    INT_TYPE(pthread_t);
    OPAQUE_TYPE(pthread_attr_t);
    OPAQUE_TYPE(pthread_cond_t);
    OPAQUE_TYPE(pthread_condattr_t);
    OPAQUE_TYPE(pthread_mutex_t);
    OPAQUE_TYPE(pthread_mutexattr_t);
    {
	struct sched_param rec;
	FieldInfo fi[1];
	INT_FIELD(fi[0],sched_priority);
	finish_struct(fi, 1, sizeof(rec), "sched_param");
    }
#if HAVE_PTHREAD_BARRIER_T
    OPAQUE_TYPE(pthread_barrier_t);
#else
    printf("struct pthread_barrier_t;\n");
#endif
#if HAVE_PTHREAD_BARRIERATTR_T
    OPAQUE_TYPE(pthread_barrierattr_t);
#else
    printf("struct pthread_barrierattr_t;\n");
#endif
#if HAVE_PTHREAD_RWLOCK_T
    OPAQUE_TYPE(pthread_rwlock_t);
#else
    printf("struct pthread_rwlock_t;\n");
#endif
#if HAVE_PTHREAD_RWLOCKATTR_T
    OPAQUE_TYPE(pthread_rwlockattr_t);
#else
    printf("struct pthread_rwlockattr_t;\n");
#endif
#if HAVE_PTHREAD_SPINLOCK_T
    OPAQUE_TYPE(pthread_spinlock_t);
#else
    printf("struct pthread_spinlock_t;\n");
#endif

    printf("\n");
    printf("\nenum : int {\n");
    CES( PTHREAD_CANCEL_ENABLE );
    CES( PTHREAD_CANCEL_DISABLE );
    CES( PTHREAD_CANCEL_DEFERRED );
    CES( PTHREAD_CANCEL_ASYNCHRONOUS );
    printf("}\n\n");

    // not pthreads, but currently only needed for a pthread func:
#if HAVE_CLOCKID_T
    INT_TYPE(clockid_t);
#else
    printf("alias Culong_t clockid_t;\n");
#endif
	
#endif
}

void c_socket() {
    //INT_TYPE(socket_t);
#if ! HAVE_SOCKLEN_T
#define socklen_t int
#endif
    INT_TYPE(socklen_t);
    
    printf("// from <sys/socket.h>\n");
    printf("const int SOL_SOCKET = %d\n\n;", SOL_SOCKET);
    printf("enum : int {\n");
    CES( SO_DEBUG );
#ifdef SO_ACCEPTCONN
    CES( SO_ACCEPTCONN );
#endif
    CES( SO_REUSEADDR );
    CES( SO_KEEPALIVE );
    CES( SO_DONTROUTE );
    CES( SO_BROADCAST );
#ifdef SO_USELOOPBACK
    CES( SO_USELOOPBACK );
#endif
    CES( SO_LINGER );
    CES( SO_OOBINLINE );
#ifdef SO_BSDCOMPAT
    CES( SO_BSDCOMPAT );
#endif
#ifdef SO_REUSEPORT
    CES( SO_REUSEPORT );
#endif
#ifdef SO_TIMESTAMP
    CES( SO_TIMESTAMP );
#endif
    CES( SO_SNDBUF );
    CES( SO_RCVBUF );
    CES( SO_SNDLOWAT );
    CES( SO_RCVLOWAT );
    CES( SO_SNDTIMEO );
    CES( SO_RCVTIMEO );
    CES( SO_ERROR );
    CES( SO_TYPE );
    printf("}\n");
    printf("\n");
    {
	struct linger rec;
	FieldInfo fi[2];
	INT_FIELD(fi[0],l_onoff);
	INT_FIELD(fi[1],l_linger);
	finish_struct(fi, 2, sizeof(rec), "linger");
    }
    printf("\n");
    printf("enum : int {\n");
    CES( SOCK_STREAM );
    CES( SOCK_DGRAM );
    CES( SOCK_RAW );
#ifdef SOCK_RDM
    CES( SOCK_RDM );
#else
    CESZ( SOCK_RDM );
#endif
#ifdef SOCK_SEQPACKET
    CES( SOCK_SEQPACKET );
#else
    CESZ( SOCK_SEQPACKET );
#endif
    printf("}\n");
    printf("\n");
    printf("enum : int {\n");
    CES( MSG_OOB );
    CES( MSG_PEEK );
    CES( MSG_DONTROUTE );
    printf("}\n");
    printf("\n");
}

void c_afpf() {
    printf("enum : int {\n");
    CES( AF_UNSPEC );
    CES( AF_UNIX );
    CES( AF_INET );
#ifndef AF_IPX
#define AF_IPX AF_UNSPEC
#endif
    CES( AF_IPX );
#ifndef AF_APPLETALK
#define AF_APPLETALK AF_UNSPEC
#endif
    CES( AF_APPLETALK );
#ifndef AF_INET6
#define AF_INET6 AF_UNSPEC
#endif
    CES( AF_INET6 );
#ifndef AF_BOGOSITY
#define AF_BOGOSITY AF_UNSPEC
#endif
    CES( AF_BOGOSITY );
    printf("}\n");
    printf("\n");
    printf("enum : int {\n");
    CES( PF_UNSPEC );
    CES( PF_UNIX );
    CES( PF_INET );
#ifndef PF_IPX
#define PF_IPX PF_UNSPEC
#endif
    CES( PF_IPX );
#ifndef PF_APPLETALK
#define PF_APPLETALK PF_UNSPEC
#endif
    CES( PF_APPLETALK );
#ifndef PF_INET6
#define PF_INET6 PF_UNSPEC
#endif
    printf("}\n");
    printf("\n");

    // Anyone seen a unix where PF_xxx!=AF_xxx?
    
}

void c_ipproto() {
    printf("// from <netinet/in.h>\n");
    printf("enum : int {\n");
    CES( IPPROTO_IP );
    CES( IPPROTO_ICMP );
    CES( IPPROTO_IGMP );
#ifndef IPPROTO_GGP
#define IPPROTO_GGP (-1)
#endif
    CES( IPPROTO_GGP );
    CES( IPPROTO_TCP );
    CES( IPPROTO_PUP );
    CES( IPPROTO_UDP );
    CES( IPPROTO_IDP );
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 (-1)
#endif    
    CES( IPPROTO_IPV6 );
    printf("}\n");
    printf("\n");
    printf("enum : uint {\n");
    CESX( INADDR_ANY );
    CESX( INADDR_LOOPBACK );
    CESX( INADDR_BROADCAST );
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
    CESX( INADDR_NONE );
    CESA( ADDR_ANY, INADDR_ANY );
    printf("}\n");
    
    printf("// from <netinet/tcp.h>\n");
    printf("enum : int {\n");
    CES( TCP_NODELAY );
    printf("}\n");
    printf("\n");
    
    printf("// from <netinet6/in6.h>\n");
    printf("enum : int {\n");
    //CES( IPV6_ADDRFORM );
    //CES( IPV6_PKTINFO );
    //CES( IPV6_HOPOPTS );
    //CES( IPV6_DSTOPTS );
    //CES( IPV6_RTHDR );
    //CES( IPV6_PKTOPTIONS );
    //CES( IPV6_CHECKSUM );
    //CES( IPV6_HOPLIMIT );
    //CES( IPV6_NEXTHOP );
    //CES( IPV6_AUTHHDR );
#ifndef IPV6_UNICAST_HOPS
#define IPV6_UNICAST_HOPS  (-1)
#define IPV6_MULTICAST_IF  (-1)
#define IPV6_MULTICAST_HOPS  (-1)
#define IPV6_MULTICAST_LOOP  (-1)
#define IPV6_JOIN_GROUP  (-1)
#define IPV6_LEAVE_GROUP  (-1)
#endif
    CES( IPV6_UNICAST_HOPS );
    CES( IPV6_MULTICAST_IF );
    CES( IPV6_MULTICAST_HOPS );
    CES( IPV6_MULTICAST_LOOP );
    CES( IPV6_JOIN_GROUP );
    CES( IPV6_LEAVE_GROUP );
    //CES( IPV6_ROUTER_ALERT );
    //CES( IPV6_MTU_DISCOVER );
    //CES( IPV6_MTU );
    //CES( IPV6_RECVERR );
    //CES( IPV6_V6ONLY );
    //CES( IPV6_JOIN_ANYCAST );
    //CES( IPV6_LEAVE_ANYCAST );
    //CES( IPV6_IPSEC_POLICY );
    //CES( IPV6_XFRM_POLICY );
    printf("}\n");
    printf("\n");
}

void c_netdb() {
    printf("// from <netdb.h>\n");
    {
	FieldInfo fi[3];
	struct protoent rec;
	ADD_FIELD(fi[0], "char *", p_name);
	ADD_FIELD(fi[1], "char **", p_aliases);
	INT_FIELD(fi[2], p_proto);
	finish_struct(fi, 3, sizeof(rec), "protoent");
    }
    {
	FieldInfo fi[4];
	struct servent rec;
	ADD_FIELD(fi[0], "char *", s_name);
	ADD_FIELD(fi[1], "char **", s_aliases);
	INT_FIELD(fi[2], s_port);
	ADD_FIELD(fi[3], "char *", s_proto);
	finish_struct(fi, 4, sizeof(rec), "servent");
    }
    {
	FieldInfo fi[5];
	struct hostent rec;
	ADD_FIELD(fi[0], "char *", h_name);
	ADD_FIELD(fi[1], "char **", h_aliases);
	INT_FIELD(fi[2], h_addrtype);
	INT_FIELD(fi[3], h_length);
	ADD_FIELD(fi[4], "char **", h_addr_list);
	finish_struct_ex(fi, 5, sizeof(rec), "hostent",
	"char* h_addr()\n"
	"{\n"
	"	return h_addr_list[0];\n"
	"}");
    }
    /*//not required for std/socket.d yet
    {
	FieldInfo fi[8];
	struct addrinfo rec;
	INT_FIELD(fi[0], ai_flags);
	INT_FIELD(fi[1], ai_family);
	INT_FIELD(fi[2], ai_socktype);
	INT_FIELD(fi[3], ai_protocol);
	INT_FIELD(fi[4], ai_addrlen);
	ADD_FIELD(fi[5], "sockaddr *", ai_addr);
	ADD_FIELD(fi[6], "char *", ai_canonname);
	ADD_FIELD(fi[7], "addrinfo *", ai_next);
	finish_struct(fi, 8, "addrinfo");
    }
    */
    printf("struct addrinfo { }\n");
}

void c_pwd()
{
    {
	FieldInfo fi[7];
	struct passwd rec;
	ADD_FIELD(fi[0], "char *", pw_name);
	ADD_FIELD(fi[1], "char *", pw_passwd);
	INT_FIELD(fi[2], pw_uid);
	INT_FIELD(fi[3], pw_gid);
	ADD_FIELD(fi[4], "char *", pw_gecos);
	ADD_FIELD(fi[5], "char *", pw_dir);
	ADD_FIELD(fi[6], "char *", pw_shell);
	finish_struct(fi, 7, sizeof(rec), "passwd");
    }	
}

int main()
{
    printf("extern(C) {\n\n");
    c_types();
    c_dirent();
    c_fcntl();
    c_time();
    c_utime();
    c_stat();
    c_signal();
    c_mman();
    c_errno();
    c_semaphore();
    c_pthread();
    c_socket();
    c_afpf();
    c_ipproto();
    c_netdb();
    c_pwd();
    printf("}\n\n");
    
    return 0;
}
